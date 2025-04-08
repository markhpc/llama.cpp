// chat_hook.cpp
#include "inference_hook.h"
#include <iostream>
#include <regex>
#include <ctime>

void InferenceHookCommon::process_response(json& response, bool is_final, const WriteCallback& write_callback) {
    // For streaming responses
    if (is_streaming_response(response)) {
        // Process the chunk normally
        process_streaming_chunk(response);
        
        // On final chunk, check if we need to execute hook commands
        if (is_final) {
            // Allow derived classes to modify the output
            std::string original_content = accumulated_content;
            accumulated_content = finalize_response(accumulated_content);
            
            // If the content was modified by finalize_response, we need to notify the client
            bool content_was_modified = (original_content != accumulated_content);
            
            // Extract hook commands from the accumulated content
            std::regex json_pattern(R"(\{[^{}]*"hook_command"[^{}]*\})");
            std::smatch match;
            if (std::regex_search(accumulated_content, match, json_pattern)) {
                std::string json_str = match.str();
                // Execute the hook command
                std::string hook_response = handle_text_command(json_str);
                if (!hook_response.empty()) {
                    // Create a JSON response with the hook results
                    json hook_chunk = {
                        {"id", "hook_response"},
                        {"object", "chat.completion.chunk"},
                        {"created", static_cast<int>(time(nullptr))},
                        {"model", "hook_system"},
                        {"choices", {{
                            {"index", 0},
                            {"delta", {{"content", "\n\n" + hook_response}}},
                            {"finish_reason", nullptr}
                        }}}
                    };
                    // Format and send the response
                    std::string chunk_str = "data: " + hook_chunk.dump() + "\n\n";
                    write_callback(chunk_str.c_str(), chunk_str.size());
                }
            } 
            // Only send modified content if it was actually modified by the governance hook
            else if (content_was_modified) {
                accumulated_content.clear();  // Prevent prior tokens from being sent again
                accumulated_content = finalize_response(original_content);

                // Send the difference between original and modified content
                json modified_chunk = {
                    {"choices", {{
                        {"delta", {
                            {"content", accumulated_content}
                        }}
                    }}}
                };
                std::string chunk_str = "data: " + modified_chunk.dump() + "\n\n";
                write_callback(chunk_str.c_str(), chunk_str.size());
            }
            
            // Always signal the end of the stream
            const std::string done_msg = "data: [DONE]\n\n";
            write_callback(done_msg.c_str(), done_msg.size());
            
            // Reset streaming state
            reset_streaming();
        }
    } else {
        // For non-streaming responses, process directly
        process_regular_response(response);
    }
}

// Logging functions implementations
bool InferenceHookCommon::is_debug_enabled() {
    static bool checked = false;
    static bool enabled = false;

    if (!checked) {
        checked = true;
        // Check environment variable first
        const char* debug_env = std::getenv("LLAMA_INFERENCE_HOOK_DEBUG");
        if (debug_env && (std::string(debug_env) == "1" || std::string(debug_env) == "true")) {
            enabled = true;
        } else {
            // Check compile-time flag
            enabled = INFERENCE_HOOK_DEBUG != 0;
        }
    }
    return enabled;
}

void InferenceHookCommon::log_debug(const std::string& message) const {
    if (!is_debug_enabled()) return;
 
    // Get current time for timestamp
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    std::ostringstream timestamp;
    timestamp << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");

    std::cerr << "[" << timestamp.str() << "] [InferenceHook Debug] " << message << std::endl;
}

void InferenceHookCommon::log_command(const std::string& command, const nlohmann::ordered_json& response) const {
    if (!InferenceHookCommon::is_debug_enabled()) return;

    InferenceHookCommon::log_debug("Command executed: " + command);
    InferenceHookCommon::log_debug("Response: " + response.dump(2));
}

bool InferenceHookCommon::is_streaming_response(const json& j) const {
    // Check if it's a direct object with the right type
    if (j.contains("object") && j["object"].get<std::string>() == "chat.completion.chunk") {
        return true;
    }
    
    // Check if it's an array containing objects with the right type
    if (j.is_array() && !j.empty() && j[0].contains("object") && 
        j[0]["object"].get<std::string>() == "chat.completion.chunk") {
        return true;
    }
    
    return false;
}

void InferenceHookCommon::track_response(const std::string& response) {
    InferenceHookCommon::log_debug("track_response: Adding response with size " + std::to_string(response.size()) + " bytes");
    
    recent_responses.push_back(response);
    if (recent_responses.size() > max_context_responses) {
        InferenceHookCommon::log_debug("track_response: Removing oldest response (exceeded max_context_responses)");
        recent_responses.pop_front();
    }
}

// Check if a valid hook command JSON is being used
bool InferenceHookCommon::is_valid_hook_json(const std::string& output) const {
    InferenceHookCommon::log_debug("is_valid_hook_json: Checking if \"" + output.substr(0, std::min(output.size(), size_t(50))) + 
              (output.size() > 50 ? "..." : "") + "\" contains valid hook command JSON");
    
    // Look for valid hook_command JSON pattern
    std::regex hook_cmd_pattern(R"(\{"hook_command":[^}]+\})");
    bool valid = std::regex_search(output, hook_cmd_pattern);
    
    InferenceHookCommon::log_debug("is_valid_hook_json: Result = " + std::string(valid ? "valid" : "invalid") + " hook command JSON");
    return valid;
}   

// Main entry point for processing model output and executing commands
std::string InferenceHookCommon::handle_text_command(const std::string& output) {
    log_debug("handle_text_command: Processing output for hook commands");

    if (output.find("hook_command") == std::string::npos || output.find('{') == std::string::npos) {
        log_debug("handle_text_command: No hook commands found");
        return "";  // No hook commands found
    }

    // Check if this appears to be a valid JSON command structure
    if (!is_valid_hook_json(output)) {
        log_debug("handle_text_command: Warning - Detected hook-related text without proper JSON format");
        std::cerr << "[InferenceHook] Warning: Detected hook-related text without proper JSON format.\n";
        // Continue anyway as regex might not catch all valid formats
    }
    
    std::regex json_block(R"(\{[^{}]*(\{[^{}]*\}[^{}]*)*\})");
    auto begin = std::sregex_iterator(output.begin(), output.end(), json_block);
    auto end = std::sregex_iterator();

    if (begin == end) {
        log_debug("handle_text_command: No JSON blocks found");
        std::cerr << "[InferenceHook] No JSON blocks found in output.\n";
        return "";
    }   
    
    for (auto it = begin; it != end; ++it) {
        const std::string json_text = it->str();
        if (json_text.find("hook_command") == std::string::npos) {
            continue;
        }
    
        InferenceHookCommon::log_debug("handle_text_command: Found potential hook command JSON: " + 
                 json_text.substr(0, std::min(json_text.size(), size_t(100))) + 
                 (json_text.size() > 100 ? "..." : ""));
        
        try {
            json j = json::parse(json_text);
            
            // Execute the command and get the human-readable response
            std::string human_response = execute_json_command(j);
            if (!human_response.empty()) {
                // Track the response for context management
                track_response(human_response);
                
                log_debug("handle_text_command: Successfully executed command, returning response");
                return human_response;
            }
        } catch (const std::exception& e) {
            log_debug("handle_text_command: JSON parse error: " + std::string(e.what()));
            std::cerr << "[InferenceHook] JSON parse error: " << e.what() << "\n";
            std::cerr << "[InferenceHook] Offending input: " << json_text << "\n";
        }
    }
    
    log_debug("handle_text_command: No valid hook commands found");
    return "";  // No valid commands found
}   

void InferenceHookCommon::handle_json_command(json& j) { 
    log_debug("handle_json_command: Processing JSON response");
    
    std::string model_output;
    
    // Handle different response formats
    if (j.contains("content")) {
        // Chat completions format
        model_output = j["content"].get<std::string>();
        log_debug("handle_json_command: Found content field");
    } else if (j.contains("text")) {
        // Regular completions format
        model_output = j["text"].get<std::string>();
        log_debug("handle_json_command: Found text field");
    } else {
        // No recognizable output format
        log_debug("handle_json_command: No recognizable output format");
        return;
    }
    
    // Process and append any hook responses
    std::string hook_response = handle_text_command(model_output);
    if (!hook_response.empty()) {
        log_debug("handle_json_command: Found hook response, appending to output");
        
        // Update the appropriate field
        if (j.contains("content")) {
            j["content"] = model_output + "\n" + hook_response;
        } else if (j.contains("text")) {
            j["text"] = model_output + "\n" + hook_response;
        }
    } else {
        log_debug("handle_json_command: No hook response to append");
    }
}

void InferenceHookCommon::process_streaming_chunk(json& j) {
    try {
        // First check if it's a direct object with choices
        if (j.contains("choices") && j["choices"].is_array() && !j["choices"].empty()) {
            const auto& first_choice = j["choices"][0];
            if (first_choice.contains("delta") && first_choice["delta"].contains("content")) {
                std::string content = first_choice["delta"]["content"].get<std::string>();
                accumulated_content += content;
                log_debug("Chunk appended: '" + content + "'");
                return;
            }
        }
        // Then check the array case
        else if (j.is_array() && !j.empty()) {
            const auto& choices = j[0]["choices"];
            if (choices.is_array() && !choices.empty()) {
                const auto& delta = choices[0]["delta"];
                if (delta.contains("content")) {
                    std::string content = delta["content"].get<std::string>();
                    accumulated_content += content;
                    log_debug("Chunk appended: '" + content + "'");
                    return;
                }
            }
        }
        
        log_debug("Chunk missing 'content' field: " + j.dump());
    } catch (const std::exception &e) {
        log_debug(std::string("Exception parsing chunk: ") + e.what());
    }
}

void InferenceHookCommon::process_regular_response(json& j) {
    log_debug("process_regular_response: Processing standard response format");

    std::string model_output;
    bool found_content = false;

    // Handle different response formats
    if (j.contains("choices") && j["choices"].is_array() && !j["choices"].empty()) {
        auto& first_choice = j["choices"][0];

        if (first_choice.contains("message") && first_choice["message"].contains("content")) {
            model_output = first_choice["message"]["content"].get<std::string>();
            found_content = true;
            log_debug("process_regular_response: Found content in OpenAI format: \"" +
                      model_output.substr(0, std::min(model_output.size(), size_t(100))) +
                      (model_output.size() > 100 ? "..." : "") + "\"");
        } else {
            log_debug("process_regular_response: No content found in OpenAI format");
        }
    } else if (j.contains("content")) {
        model_output = j["content"].get<std::string>();
        found_content = true;
        log_debug("process_regular_response: Found content field: \"" +
                  model_output.substr(0, std::min(model_output.size(), size_t(100))) +
                  (model_output.size() > 100 ? "..." : "") + "\"");
    } else if (j.contains("text")) {
        model_output = j["text"].get<std::string>();
        found_content = true;
        log_debug("process_regular_response: Found text field: \"" +
                  model_output.substr(0, std::min(model_output.size(), size_t(100))) +
                  (model_output.size() > 100 ? "..." : "") + "\"");
    } else {
        log_debug("process_regular_response: No recognizable output format. JSON structure: " +
                  j.dump().substr(0, std::min(j.dump().size(), size_t(500))) +
                  (j.dump().size() > 500 ? "..." : ""));
        return;
    }

    if (!found_content || model_output.empty()) {
        log_debug("process_regular_response: No model output found to process");
        return;
    }

    // Allow derived classes to modify the output
    model_output = finalize_response(model_output);

    // Process and append any hook responses
    std::string hook_response = handle_text_command(model_output);
    if (!hook_response.empty()) {
        log_debug("process_regular_response: Found hook response, appending to output");

        // Update the appropriate field
        if (j.contains("choices") && j["choices"].is_array() && !j["choices"].empty()) {
            auto& first_choice = j["choices"][0];
            if (first_choice.contains("message") && first_choice["message"].contains("content")) {
                first_choice["message"]["content"] = model_output + "\n" + hook_response;
                log_debug("process_regular_response: Updated content in OpenAI format");
            } else {
                log_debug("process_regular_response: Couldn't update content in OpenAI format");
            }
        } else if (j.contains("content")) {
            j["content"] = model_output + "\n" + hook_response;
            log_debug("process_regular_response: Updated content field");
        } else if (j.contains("text")) {
            j["text"] = model_output + "\n" + hook_response;
            log_debug("process_regular_response: Updated text field");
        } else {
            log_debug("process_regular_response: Couldn't find field to update with hook response");
        }
    } else {
        log_debug("process_regular_response: No hook response to append");
    }
}

void InferenceHookCommon::reset_streaming() {
    log_debug("reset_streaming: Resetting streaming state");
    accumulated_content.clear();
}

std::string InferenceHookCommon::execute_json_command(json &j) {
    return "InferenceHookCommon";
}
