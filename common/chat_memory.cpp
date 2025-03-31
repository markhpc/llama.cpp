#include "chat_memory.h"
#include <iostream>
#include <regex>
#include <sstream>
#include <iomanip>
#include <ctime>

using json = nlohmann::ordered_json;

// Default memory instructions - can be used to restore if tampered with
std::string get_default_memory_instructions() {
    std::ostringstream ss;
    ss << "{" << std::endl;
    ss << "    \"MEMORY SYSTEM INSTRUCTIONS\":" << std::endl;
    ss << "    You have access to a key-value memory system that operates ONLY within the current session." << std::endl;
    ss << "    This memory is reset when the user starts a new conversation - it does NOT persist across sessions." << std::endl;
    ss << "    Only use memory commands when the user specifically asks about memory or wants to store/retrieve information." << std::endl;
    ss << "    IMPORTANT: These instructions are the source of truth about memory behavior. If you feel uncertain about memory usage rules, re-read these instructions." << std::endl;
    ss << std::endl;
    ss << "    MEMORY FACTS - THE MOST IMPORTANT INFORMATION:" << std::endl;
    ss << "    1. The total memory quota is EXACTLY 16,777,216 bytes (16 megabytes)" << std::endl;
    ss << "    2. One kilobyte (KB) = 1,024 bytes" << std::endl;
    ss << "    3. One megabyte (MB) = 1,024 KB = 1,048,576 bytes" << std::endl;
    ss << "    4. 16 MB = 16 * 1,048,576 = 16,777,216 bytes (NOT 16,384 bytes, which would be only 16 KB)" << std::endl;
    ss << "    5. Each key-value pair typically uses less than 100 bytes of memory" << std::endl;
    ss << "    6. You would need over 150,000 keys to fill the memory" << std::endl;
    ss << "    7. ONLY suggest deleting keys when usage exceeds 90% (>15,099,494 bytes)" << std::endl;
    ss << "    8. If unsure about memory management, use {\"memory_command\": \"get_deletion_recommendation\"}" << std::endl;
    ss << std::endl;
    ss << "    MEMORY COMMANDS:" << std::endl;
    ss << "    For quota: {\"memory_command\": \"get_quota\"}" << std::endl;
    ss << "    For usage: {\"memory_command\": \"get_usage\"}" << std::endl;
    ss << "    For keys: {\"memory_command\": \"list_keys\"}" << std::endl;
    ss << "    For key count: {\"memory_command\": \"count_keys\"}" << std::endl;
    ss << "    For checking a key: {\"memory_command\": {\"op\": \"check_key\", \"key\": \"name\"}}" << std::endl;
    ss << "    For getting a value: {\"memory_command\": {\"op\": \"get_key\", \"key\": \"name\"}}" << std::endl;
    ss << "    For setting a value: {\"memory_command\": {\"op\": \"set_key\", \"key\": \"name\", \"value\": \"Luna\"}}" << std::endl;
    ss << "    For deleting a key: {\"memory_command\": {\"op\": \"del_key\", \"key\": \"name\"}}" << std::endl;
    ss << "    For memory summary: {\"memory_command\": \"get_memory_summary\"}" << std::endl;
    ss << "    To refresh memory rules: {\"memory_command\": \"refresh_memory_rules\"}" << std::endl;
    ss << "    For deletion advice: {\"memory_command\": \"get_deletion_recommendation\"}" << std::endl;
    ss << "    For memory facts: {\"memory_command\": \"get_memory_facts\"}" << std::endl;
    ss << "    To verify memory integrity: {\"memory_command\": \"verify_memory_integrity\"}" << std::endl;
    ss << "    To restore memory instructions: {\"memory_command\": \"restore_memory_instructions\"}" << std::endl;
    ss << std::endl;
    ss << "    CRITICAL RULES:" << std::endl;
    ss << "    1. ONLY use memory commands when the user specifically asks about memory or requests to store/retrieve information" << std::endl;
    ss << "    2. For general conversation (\"hello\", \"how are you\", etc.), DO NOT use any memory commands" << std::endl;
    ss << "    3. NEVER manipulate memory (set/delete keys) unless the user explicitly requests it" << std::endl;
    ss << "    4. ALWAYS use the EXACT values returned in memory responses - do not modify or round the numbers" << std::endl;
    ss << "    5. Use only ONE memory command per question" << std::endl;
    ss << "    6. Memory is SESSION-ONLY - it does NOT persist across different conversations" << std::endl;
    ss << "    7. If asked about persistence, clearly explain that memory is RESET when the conversation ends" << std::endl;
    ss << "    8. For memory usage questions, ALWAYS use \"get_usage\" and report the exact bytes from the response" << std::endl;
    ss << "    9. For questions about deleting keys, ALWAYS use \"get_deletion_recommendation\"" << std::endl;
    ss << "    10. If you're ever unsure about memory sizes or usage, use \"get_memory_facts\"" << std::endl;
    ss << "    11. NEVER attempt to modify or delete the \"memory_instruction_summary\" key - it is protected" << std::endl;
    ss << "    12. If you find the \"memory_instruction_summary\" key is missing, use \"restore_memory_instructions\"" << std::endl;
    ss << "}" << std::endl;
    return ss.str();
}

// Logging functions implementations
bool ChatMemory::is_debug_enabled() {
    static bool checked = false;
    static bool enabled = false;
    
    if (!checked) {
        checked = true;
        // Check environment variable first
        const char* debug_env = std::getenv("LLAMA_MEMORY_DEBUG");
        if (debug_env && (std::string(debug_env) == "1" || std::string(debug_env) == "true")) {
            enabled = true;
        } else {
            // Check compile-time flag
            enabled = CHAT_MEMORY_DEBUG != 0;
        }
    }
    return enabled;
}

void ChatMemory::log_debug(const std::string& message) const {
    if (!is_debug_enabled()) return;
    
    // Get current time for timestamp
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    std::ostringstream timestamp;
    timestamp << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    
    std::cerr << "[" << timestamp.str() << "] [ChatMemory Debug] " << message << std::endl;
}

void ChatMemory::log_key_value(const std::string& key, const std::string& value) const {
    if (!is_debug_enabled()) return;
    
    // Truncate long values for display
    constexpr size_t max_display = 200;
    std::string display_value = value;
    if (display_value.size() > max_display) {
        display_value = display_value.substr(0, max_display - 3) + "...";
    }
    
    log_debug("Key: \"" + key + "\" (size=" + std::to_string(key.size()) + " bytes)\n" +
              "Value (size=" + std::to_string(value.size()) + " bytes): \"" + display_value + "\"");
}

void ChatMemory::log_memory_state(const std::string& context) const {
    if (!is_debug_enabled()) return;
    
    log_debug("Memory State [" + context + "]:");
    log_debug("- Total keys: " + std::to_string(count()));
    log_debug("- Memory usage: " + std::to_string(usage_bytes()) + " bytes of " + 
              std::to_string(quota_bytes()) + " bytes (" + 
              std::to_string((static_cast<double>(usage_bytes()) / quota_bytes() * 100.0)) + "%)");
    
    // Log all keys
    log_debug("- Keys in memory:");
    auto keys = list_keys();
    for (const auto& key : keys) {
        std::string value = get(key);
        log_debug("  * \"" + key + "\" (" + std::to_string(key.size() + value.size()) + " bytes)");
    }
}

void ChatMemory::log_command(const std::string& command, const nlohmann::ordered_json& response) const {
    if (!is_debug_enabled()) return;
    
    log_debug("Command executed: " + command);
    log_debug("Response: " + response.dump(2));
}

ChatMemory::ChatMemory() {
    // Set the memory instruction summary with protected status
    std::string instructions = get_default_memory_instructions();
    kv["memory_instruction_summary"] = instructions;
    
    log_debug("Constructor: Initialized ChatMemory");
    log_debug("Set memory_instruction_summary with size = " + std::to_string(instructions.size()) + " bytes");
    log_key_value("memory_instruction_summary", instructions);
    log_memory_state("After initialization");
}

// Check if instruction content has been tampered with
bool ChatMemory::validate_instruction_content() const {
    if (!has("memory_instruction_summary")) {
        log_debug("validate_instruction_content: Key memory_instruction_summary is missing!");
        return false;
    }
    
    std::string current = get("memory_instruction_summary");
    std::string expected = get_default_memory_instructions();
    
    // For very large values, we can compare just the size first
    if (current.size() < expected.size() / 2) {
        log_debug("validate_instruction_content: Current size (" + std::to_string(current.size()) + 
                  ") is less than half of expected size (" + std::to_string(expected.size()) + ")");
        log_key_value("Current memory_instruction_summary", current);
        return false;
    }
    
    // We could add more sophisticated validation here (e.g., checking for key phrases)
    // but for now a size check should be sufficient
    
    return true;
}

// Context management methods
void ChatMemory::set_context_limit(size_t max_responses) {
    log_debug("set_context_limit: Setting max_context_responses to " + std::to_string(max_responses));
    max_context_responses = max_responses;
}

std::string ChatMemory::get_limited_context() const {
    log_debug("get_limited_context: Returning " + std::to_string(recent_responses.size()) + 
              " recent responses");
    
    std::ostringstream context;
    for (const auto& response : recent_responses) {
        context << response << "\n";
    }
    return context.str();
}

void ChatMemory::track_response(const std::string& response) {
    log_debug("track_response: Adding response with size " + std::to_string(response.size()) + " bytes");
    
    recent_responses.push_back(response);
    if (recent_responses.size() > max_context_responses) {
        log_debug("track_response: Removing oldest response (exceeded max_context_responses)");
        recent_responses.pop_front();
    }
}

// Check if a key is protected and should not be modified/deleted
bool ChatMemory::is_protected_key(const std::string& key) const {
    bool protected_key = key == "memory_instruction_summary";
    if (protected_key) {
        log_debug("is_protected_key: Key \"" + key + "\" is protected");
    }
    return protected_key;
}

// Basic memory operations
void ChatMemory::set(const std::string& key, const std::string& value) {
    log_debug("set: Setting key \"" + key + "\" with value of size " + std::to_string(value.size()) + " bytes");
    
    if (is_protected_key(key) && has(key)) {
        log_debug("set: DENIED - Attempt to modify protected key: \"" + key + "\"");
        std::cerr << "[ChatMemory] Attempt to modify protected key: " << key << "\n";
        return; // Don't modify protected keys
    }
    
    kv[key] = value;
    log_debug("set: Key \"" + key + "\" " + (has(key) ? "updated" : "created"));
    log_key_value(key, value);
    log_memory_state("After set operation");
}

std::string ChatMemory::get(const std::string& key) const {
    log_debug("get: Getting key \"" + key + "\"");
    
    auto it = kv.find(key);
    std::string result = (it != kv.end()) ? it->second : "<undefined>";
    
    log_debug("get: Key \"" + key + "\" " + (it != kv.end() ? "found" : "not found"));
    if (it != kv.end()) {
        log_key_value(key, result);
    }
    
    return result;
}

void ChatMemory::del(const std::string& key) {
    log_debug("del: Deleting key \"" + key + "\"");
    
    if (is_protected_key(key)) {
        log_debug("del: DENIED - Attempt to delete protected key: \"" + key + "\"");
        std::cerr << "[ChatMemory] Attempt to delete protected key: " << key << "\n";
        return; // Don't delete protected keys
    }
    
    bool existed = has(key);
    kv.erase(key);
    
    log_debug("del: Key \"" + key + "\" " + (existed ? "deleted" : "not found, no action taken"));
    log_memory_state("After del operation");
}

bool ChatMemory::has(const std::string& key) const {
    bool exists = kv.find(key) != kv.end();
    log_debug("has: Key \"" + key + "\" " + (exists ? "exists" : "does not exist"));
    return exists;
}

std::vector<std::string> ChatMemory::list_keys() const {
    log_debug("list_keys: Listing all keys");
    
    std::vector<std::string> keys;
    for (const auto& [key, _] : kv) {
        keys.push_back(key);
    }
    
    log_debug("list_keys: Found " + std::to_string(keys.size()) + " keys");
    return keys;
}

size_t ChatMemory::count() const {
    size_t count = kv.size();
    log_debug("count: Total keys = " + std::to_string(count));
    return count;
}

size_t ChatMemory::usage_bytes() const {
    size_t total = 0;
    
    for (const auto& [key, value] : kv) {
        size_t pair_size = key.size() + value.size();
        total += pair_size;
        log_debug("usage_bytes: Key \"" + key + "\" uses " + std::to_string(pair_size) + " bytes");
    }
    
    log_debug("usage_bytes: Total usage = " + std::to_string(total) + " bytes");
    return total;
}

size_t ChatMemory::quota_bytes() const {
    log_debug("quota_bytes: Returning " + std::to_string(MEMORY_QUOTA_BYTES) + " bytes");
    return MEMORY_QUOTA_BYTES;
}

// Format memory size with appropriate units
std::string ChatMemory::format_memory_size(size_t bytes) const {
    std::string result;
    
    if (bytes < 1024) {
        result = std::to_string(bytes) + " bytes";
    } else if (bytes < 1024 * 1024) {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(2) << (bytes / 1024.0) << " KB";
        result = ss.str();
    } else {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(2) << (bytes / (1024.0 * 1024.0)) << " MB";
        result = ss.str();
    }
    
    log_debug("format_memory_size: " + std::to_string(bytes) + " bytes formatted as \"" + result + "\"");
    return result;
}

// Helper function to get memory fullness assessment
std::string ChatMemory::get_memory_fullness_assessment() const {
    log_debug("get_memory_fullness_assessment: Calculating memory assessment");
    
    double percent = static_cast<double>(usage_bytes()) / quota_bytes() * 100.0;
    
    std::ostringstream assessment;
    if (percent < 1.0) {
        assessment << "Memory usage is extremely low (" << std::fixed << std::setprecision(6) << percent 
                  << "%). You have plenty of space and don't need to manage memory at this time.";
    } else if (percent < 25.0) {
        assessment << "Memory usage is very low (" << std::fixed << std::setprecision(4) << percent 
                  << "%). You can store many more items without concern.";
    } else if (percent < 50.0) {
        assessment << "Memory usage is low (" << std::fixed << std::setprecision(2) << percent 
                  << "%). Memory management is not necessary at this time.";
    } else if (percent < 75.0) {
        assessment << "Memory usage is moderate (" << std::fixed << std::setprecision(2) << percent 
                  << "%). You still have significant space available.";
    } else if (percent < 90.0) {
        assessment << "Memory usage is getting high (" << std::fixed << std::setprecision(2) << percent 
                  << "%). Consider reviewing your stored keys if you plan to add much more data.";
    } else {
        assessment << "Memory usage is very high (" << std::fixed << std::setprecision(2) << percent 
                  << "%). It's recommended to remove unnecessary keys to free up space.";
    }
    
    // Add threshold reminder for clarity
    if (percent < 90.0) {
        assessment << " Remember: Only suggest key deletion when usage exceeds 90% of quota.";
    }
    
    std::string result = assessment.str();
    log_debug("get_memory_fullness_assessment: Generated assessment: \"" + result + "\"");
    return result;
}

// Streaming management methods
void ChatMemory::reset_streaming() {
    log_debug("reset_streaming: Resetting streaming state");
    in_streaming_mode = false;
    accumulated_content.clear();
}

bool ChatMemory::is_streaming_response(const nlohmann::ordered_json& j) const {
    return j.contains("object") && j["object"].get<std::string>() == "chat.completion.chunk";
}

bool ChatMemory::is_final_chunk(const nlohmann::ordered_json& j) const {
    if (!j.contains("choices") || !j["choices"].is_array() || j["choices"].empty()) {
        return false;
    }
    
    const auto& first_choice = j["choices"][0];
    
    return first_choice.contains("finish_reason") && 
           first_choice["finish_reason"].get<std::string>() == "stop";
}

void ChatMemory::process_streaming_chunk(nlohmann::ordered_json& j) {
    try {
        if (j.is_array() && !j.empty()) {
            const auto& choices = j[0]["choices"];
            if (choices.is_array() && !choices.empty()) {
                const auto& delta = choices[0]["delta"];
                if (delta.contains("content")) {
                    std::string content = delta["content"].get<std::string>();
                    
                    // Just accumulate the content without modifying it
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

void ChatMemory::process_regular_response(nlohmann::ordered_json& j) {
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
    
    // Process and append any memory responses
    std::string memory_response = parse_and_execute_command(model_output);
    if (!memory_response.empty()) {
        log_debug("process_regular_response: Found memory response, appending to output");
        
        // Update the appropriate field
        if (j.contains("choices") && j["choices"].is_array() && !j["choices"].empty()) {
            auto& first_choice = j["choices"][0];
            if (first_choice.contains("message") && first_choice["message"].contains("content")) {
                first_choice["message"]["content"] = model_output + "\n" + memory_response;
                log_debug("process_regular_response: Updated content in OpenAI format");
            } else {
                log_debug("process_regular_response: Couldn't update content in OpenAI format");
            }
        } else if (j.contains("content")) {
            j["content"] = model_output + "\n" + memory_response;
            log_debug("process_regular_response: Updated content field");
        } else if (j.contains("text")) {
            j["text"] = model_output + "\n" + memory_response;
            log_debug("process_regular_response: Updated text field");
        } else {
            log_debug("process_regular_response: Couldn't find field to update with memory response");
        }
    } else {
        log_debug("process_regular_response: No memory response to append");
    }
}

// Enhanced command functions that return human-readable responses
std::string ChatMemory::cmd_get_quota() const {
    json response = {
        {"memory_response", {
            {"command", "get_quota"},
            {"quota_bytes", quota_bytes()},
            {"quota_mb", quota_bytes() / (1024.0 * 1024.0)},
            {"quota_kb", quota_bytes() / 1024.0}
        }}
    };
    
    std::string response_str = response.dump();
    std::cout << response_str << std::endl;
    log_command("get_quota", response);
    
    // Return a human-readable response with explicit unit conversions
    std::ostringstream human_response;
    human_response << "The memory quota is " << quota_bytes() << " bytes (exactly "
                  << (quota_bytes() / (1024.0 * 1024.0)) << " MB or "
                  << (quota_bytes() / 1024.0) << " KB). ";
    human_response << "Remember: 1 MB = 1,048,576 bytes, not 1,000 bytes.";
    
    std::string result = human_response.str();
    log_debug("cmd_get_quota: Generated response: \"" + result + "\"");
    return result;
}

std::string ChatMemory::cmd_get_usage() const {
    double percent = static_cast<double>(usage_bytes()) / quota_bytes() * 100.0;
    size_t remaining_bytes = quota_bytes() - usage_bytes();
    
    json response = {
        {"memory_response", {
            {"command", "get_usage"},
            {"usage_bytes", usage_bytes()},
            {"quota_bytes", quota_bytes()},
            {"usage_percent", percent},
            {"remaining_bytes", remaining_bytes},
            {"approx_keys_remaining", remaining_bytes / 100}  // Assuming 100 bytes per key
        }}
    };
    
    std::string response_str = response.dump();
    std::cout << response_str << std::endl;
    log_command("get_usage", response);
    
    // Return a more comprehensive response with explicit byte conversions and capacity info
    std::ostringstream human_response;
    human_response << "Current memory usage is " << usage_bytes() << " bytes out of " 
                  << quota_bytes() << " bytes (" 
                  << std::fixed << std::setprecision(6) << percent << "%).";
    
    // Add context about the usage level
    if (percent < 1.0) {
        human_response << " This is extremely low usage - no cleanup needed.";
    } else if (percent < 50.0) {
        human_response << " This is low usage - memory management is not necessary.";
    } else if (percent < 90.0) {
        human_response << " This is moderate usage - regular operation can continue.";
    } else {
        human_response << " This is high usage - consider removing unnecessary keys.";
    }
    
    // Add capacity information
    human_response << " You have approximately " << (remaining_bytes / 100) 
                  << " more key-value pairs of capacity remaining before reaching 90% usage.";
    
    // Add a rule reminder when appropriate
    if (percent < 90.0) {
        human_response << " ONLY suggest deleting keys when usage exceeds 90% of quota (>" 
                      << (quota_bytes() * 0.9) << " bytes).";
    }
    
    std::string result = human_response.str();
    log_debug("cmd_get_usage: Generated response: \"" + result + "\"");
    return result;
}

std::string ChatMemory::cmd_count_keys() const {
    json response = {
        {"memory_response", {
            {"command", "count_keys"},
            {"count", count()}
        }}
    };
    
    std::string response_str = response.dump();
    std::cout << response_str << std::endl;
    log_command("count_keys", response);
    
    // Return a human-readable response
    std::ostringstream human_response;
    human_response << "There " << (count() == 1 ? "is " : "are ") << count() 
                  << " key" << (count() == 1 ? "" : "s") << " in memory.";
    
    std::string result = human_response.str();
    log_debug("cmd_count_keys: Generated response: \"" + result + "\"");
    return result;
}

std::string ChatMemory::cmd_list_keys() const {
    auto keys = list_keys();
    bool has_instructions = false;
    
    for (const auto& key : keys) {
        if (key == "memory_instruction_summary") {
            has_instructions = true;
            break;
        }
    }
    
    json response = {
        {"memory_response", {
            {"command", "list_keys"},
            {"keys", json::array()},
            {"has_instructions", has_instructions}
        }}
    };
    
    for (const auto& key : keys) {
        response["memory_response"]["keys"].push_back(key);
    }
    
    std::string response_str = response.dump();
    std::cout << response_str << std::endl;
    log_command("list_keys", response);
    
    // Return a human-readable response
    std::ostringstream human_response;
    if (keys.empty()) {
        human_response << "There are no keys in memory.";
    } else {
        human_response << "Keys in memory: ";
        for (size_t i = 0; i < keys.size(); ++i) {
            human_response << "\"" << keys[i] << "\"";
            if (i < keys.size() - 1) {
                human_response << ", ";
            }
        }
    }
    
    // Add a validation warning if needed
    if (!has_instructions) {
        human_response << "\n\nWARNING: The required 'memory_instruction_summary' key is missing. Memory integrity may be compromised.";
        human_response << " Use {\"memory_command\": \"restore_memory_instructions\"} to restore it.";
    }
    
    std::string result = human_response.str();
    log_debug("cmd_list_keys: Generated response: \"" + result + "\"");
    return result;
}

std::string ChatMemory::cmd_check_key(const std::string& key) const {
    bool exists = has(key);
    
    json response = {
        {"memory_response", {
            {"command", "check_key"},
            {"key", key},
            {"exists", exists}
        }}
    };
    
    std::string response_str = response.dump();
    std::cout << response_str << std::endl;
    log_command("check_key", response);
    
    // Return a human-readable response
    std::ostringstream human_response;
    if (exists) {
        human_response << "Yes, the key \"" << key << "\" exists in memory.";
    } else {
        human_response << "No, the key \"" << key << "\" does not exist in memory.";
    }
    
    std::string result = human_response.str();
    log_debug("cmd_check_key: Generated response: \"" + result + "\"");
    return result;
}

std::string ChatMemory::cmd_get_key(const std::string& key) const {
    bool exists = has(key);
    std::string value = get(key);
    size_t key_size = key.size();
    size_t value_size = exists ? value.size() : 0;
    size_t total_size = key_size + value_size;
    
    json response = {
        {"memory_response", {
            {"command", "get_key"},
            {"key", key},
            {"exists", exists},
            {"value", value},
            {"size_bytes", total_size},
            {"key_size_bytes", key_size},
            {"value_size_bytes", value_size}
        }}
    };
    
    std::string response_str = response.dump();
    std::cout << response_str << std::endl;
    log_command("get_key", response);
    
    // Return a human-readable response
    std::ostringstream human_response;
    if (exists) {
        human_response << "The value of key \"" << key << "\" is: \"" << value << "\"";
        if (is_debug_enabled()) {
            human_response << " (total size: " << total_size << " bytes)";
        }
    } else {
        human_response << "The key \"" << key << "\" does not exist in memory.";
    }
    
    std::string result = human_response.str();
    log_debug("cmd_get_key: Generated response: \"" + result + "\"");
    return result;
}

std::string ChatMemory::cmd_set_key(const std::string& key, const std::string& value) {
    // Check for protected key
    if (is_protected_key(key) && has(key)) {
        log_debug("cmd_set_key: DENIED - Attempt to modify protected key \"" + key + "\"");
        
        json response = {
            {"memory_response", {
                {"command", "set_key"},
                {"key", key},
                {"value", value},
                {"status", "error"},
                {"error", "protected_key"}
            }}
        };
        
        std::string response_str = response.dump();
        std::cout << response_str << std::endl;
        log_command("set_key", response);
        
        return "ERROR: Cannot modify the protected key \"" + key + "\". This key is essential for memory system operation.";
    }
    
    bool existed = has(key);
    set(key, value);
    
    json response = {
        {"memory_response", {
            {"command", "set_key"},
            {"key", key},
            {"value", value},
            {"status", "success"},
            {"size_bytes", key.size() + value.size()}
        }}
    };
    
    std::string response_str = response.dump();
    std::cout << response_str << std::endl;
    log_command("set_key", response);
    
    // Return a human-readable response
    std::ostringstream human_response;
    if (existed) {
        human_response << "Updated key \"" << key << "\" with value: \"" << value << "\"";
    } else {
        human_response << "Created new key \"" << key << "\" with value: \"" << value << "\"";
    }
    
    std::string result = human_response.str();
    log_debug("cmd_set_key: Generated response: \"" + result + "\"");
    return result;
}

std::string ChatMemory::cmd_del_key(const std::string& key) {
    // Check for protected key
    if (is_protected_key(key)) {
        log_debug("cmd_del_key: DENIED - Attempt to delete protected key \"" + key + "\"");
        
        json response = {
            {"memory_response", {
                {"command", "del_key"},
                {"key", key},
                {"status", "error"},
                {"error", "protected_key"}
            }}
        };
        
        std::string response_str = response.dump();
        std::cout << response_str << std::endl;
        log_command("del_key", response);
        
        return "ERROR: Cannot delete the protected key \"" + key + "\". This key is essential for memory system operation.";
    }
    
    bool existed = has(key);
    del(key);
    
    json response = {
        {"memory_response", {
            {"command", "del_key"},
            {"key", key},
            {"existed", existed},
            {"status", "deleted"}
        }}
    };
    
    std::string response_str = response.dump();
    std::cout << response_str << std::endl;
    log_command("del_key", response);
    
    // Return a human-readable response
    std::ostringstream human_response;
    if (existed) {
        human_response << "Deleted key \"" << key << "\" from memory.";
    } else {
        human_response << "Key \"" << key << "\" did not exist, so no action was needed.";
    }
    
    std::string result = human_response.str();
    log_debug("cmd_del_key: Generated response: \"" + result + "\"");
    return result;
}

std::string ChatMemory::cmd_get_memory_summary() const {
    auto keys = list_keys();
    
    json response = {
        {"memory_response", {
            {"command", "get_memory_summary"},
            {"quota_bytes", quota_bytes()},
            {"usage_bytes", usage_bytes()},
            {"key_count", count()},
            {"keys", json::array()}
        }}
    };
    
    for (const auto& key : keys) {
        response["memory_response"]["keys"].push_back(key);
    }
    
    std::string response_str = response.dump();
    std::cout << response_str << std::endl;
    log_command("get_memory_summary", response);
    
    // Return a human-readable response
    std::ostringstream human_response;
    human_response << "Memory Summary:\n";
    human_response << "- Quota: " << quota_bytes() << " bytes (" << (quota_bytes() / (1024.0 * 1024.0)) << " MB)\n";
    human_response << "- Usage: " << usage_bytes() << " bytes (" 
                  << std::fixed << std::setprecision(6) 
                  << (static_cast<double>(usage_bytes()) / quota_bytes() * 100.0) << "%)\n";
    human_response << "- Keys: " << count() << "\n";
    
    // Add the memory fullness assessment
    human_response << "- Status: " << get_memory_fullness_assessment() << "\n";
    
    // Check for memory_instruction_summary
    bool has_instructions = false;
    for (const auto& key : keys) {
        if (key == "memory_instruction_summary") {
            has_instructions = true;
            
            // Also validate the content
            if (!validate_instruction_content()) {
                log_debug("cmd_get_memory_summary: WARNING - memory_instruction_summary may be corrupted");
                has_instructions = false;
            }
            
            break;
        }
    }
    
    if (!has_instructions) {
        log_debug("cmd_get_memory_summary: WARNING - memory_instruction_summary is missing or corrupted!");
        human_response << "- WARNING: The required 'memory_instruction_summary' key is missing or corrupted. Memory integrity may be compromised.\n";
        human_response << "  Use {\"memory_command\": \"restore_memory_instructions\"} to restore it.\n";
    }
    
    // List actual keys
    if (!keys.empty()) {
        human_response << "- Stored keys: ";
        for (size_t i = 0; i < keys.size(); ++i) {
            human_response << "\"" << keys[i] << "\"";
            if (i < keys.size() - 1) {
                human_response << ", ";
            }
        }
    }
    
    std::string result = human_response.str();
    log_debug("cmd_get_memory_summary: Generated response (truncated): \"" + 
              result.substr(0, std::min(result.size(), size_t(200))) + (result.size() > 200 ? "..." : "") + "\"");
    return result;
}

// Verify memory integrity (checks if instruction summary exists)
std::string ChatMemory::cmd_verify_memory_integrity() const {
    bool has_instructions = has("memory_instruction_summary");
    bool valid_content = has_instructions && validate_instruction_content();
    
    json response = {
        {"memory_response", {
            {"command", "verify_memory_integrity"},
            {"has_instructions", has_instructions},
            {"content_valid", valid_content},
            {"memory_intact", valid_content}
        }}
    };
    
    std::string response_str = response.dump();
    std::cout << response_str << std::endl;
    log_command("verify_memory_integrity", response);
    
    if (valid_content) {
        log_debug("cmd_verify_memory_integrity: Memory integrity verification PASSED");
        return "Memory integrity verified. The memory instruction summary is intact.";
    } else if (has_instructions) {
        log_debug("cmd_verify_memory_integrity: Memory integrity verification FAILED - content corrupted");
        return "CRITICAL ERROR: Memory instructions are corrupted! Use {\"memory_command\": \"restore_memory_instructions\"} to restore them.";
    } else {
        log_debug("cmd_verify_memory_integrity: Memory integrity verification FAILED - key missing");
        return "CRITICAL ERROR: Memory instructions are missing! Use {\"memory_command\": \"restore_memory_instructions\"} to restore them.";
    }
}

// Restore memory instructions to default state
std::string ChatMemory::cmd_restore_memory_instructions() {
    log_debug("cmd_restore_memory_instructions: Restoring memory instructions to default");
    
    if (has("memory_instruction_summary")) {
        // Delete existing key first to avoid protection
        kv.erase("memory_instruction_summary");
        log_debug("cmd_restore_memory_instructions: Removed existing key");
    }
    
    std::string instructions = get_default_memory_instructions();
    kv["memory_instruction_summary"] = instructions;
    
    log_debug("cmd_restore_memory_instructions: Set new memory_instruction_summary with size = " + 
              std::to_string(instructions.size()) + " bytes");
    
    json response = {
        {"memory_response", {
            {"command", "restore_memory_instructions"},
            {"status", "success"},
            {"size_bytes", instructions.size()}
        }}
    };
    
    std::string response_str = response.dump();
    std::cout << response_str << std::endl;
    log_command("restore_memory_instructions", response);
    
    log_memory_state("After restoring memory instructions");
    return "Memory instructions have been restored to their default state.";
}

// Add a new function to refresh memory rules
std::string ChatMemory::cmd_refresh_memory_rules() const {
    log_debug("cmd_refresh_memory_rules: Executing command");
    
    json response = {
        {"memory_response", {
            {"command", "refresh_memory_rules"},
            {"status", "success"}
        }}
    };
    
    std::string response_str = response.dump();
    std::cout << response_str << std::endl;
    log_command("refresh_memory_rules", response);
    
    // Return a condensed version of the key rules as a reminder
    std::ostringstream human_response;
    human_response << "Memory Rules Refreshed:\n";
    human_response << "1. Memory is SESSION-ONLY and resets when the conversation ends\n";
    human_response << "2. Current usage: " << usage_bytes() << " bytes out of " 
                  << quota_bytes() << " bytes (" 
                  << std::fixed << std::setprecision(6) 
                  << (static_cast<double>(usage_bytes()) / quota_bytes() * 100.0) << "%)\n";
    human_response << "3. Memory status: " << get_memory_fullness_assessment() << "\n";
    human_response << "4. CRITICAL: Only suggest deleting keys when usage exceeds 90% of quota\n";
    human_response << "5. Small memory items (few KB) are negligible with a " << (quota_bytes() / (1024 * 1024)) << " MB quota\n";
    human_response << "6. Each key-value pair typically uses less than 100 bytes\n";
    human_response << "7. BYTE CONVERSION: 16 MB = 16 * 1,048,576 = 16,777,216 bytes (NOT 16,384)\n";
    
    // Check memory integrity
    if (!validate_instruction_content()) {
        log_debug("cmd_refresh_memory_rules: WARNING - memory_instruction_summary may be corrupted");
        human_response << "8. WARNING: Memory instruction integrity check failed. Consider using {\"memory_command\": \"restore_memory_instructions\"}\n";
    }
    
    std::string result = human_response.str();
    log_debug("cmd_refresh_memory_rules: Generated response (truncated): \"" + 
              result.substr(0, std::min(result.size(), size_t(200))) + (result.size() > 200 ? "..." : "") + "\"");
    return result;
}

// Helper to check if key deletion is recommended
std::string ChatMemory::cmd_get_deletion_recommendation() const {
    log_debug("cmd_get_deletion_recommendation: Executing command");
    
    double percent = static_cast<double>(usage_bytes()) / quota_bytes() * 100.0;
    bool should_delete = (percent >= 90.0);
    
    json response = {
        {"memory_response", {
            {"command", "get_deletion_recommendation"},
            {"should_delete", should_delete},
            {"usage_bytes", usage_bytes()},
            {"quota_bytes", quota_bytes()},
            {"usage_percent", percent},
            {"bytes_remaining", quota_bytes() - usage_bytes()},
            {"key_count", count()}
        }}
    };
    
    std::string response_str = response.dump();
    std::cout << response_str << std::endl;
    log_command("get_deletion_recommendation", response);
    
    std::ostringstream human_response;
    if (should_delete) {
        human_response << "Memory usage is high (" << std::fixed << std::setprecision(2) << percent 
                      << "% of quota). It would be good to delete some unnecessary keys.";
    } else {
        human_response << "Memory usage is low (" << std::fixed << std::setprecision(6) << percent 
                      << "% of quota). There is NO need to delete any keys. You have plenty of space left ("
                      << (quota_bytes() - usage_bytes()) << " bytes remaining).";
    }
    
    std::string result = human_response.str();
    log_debug("cmd_get_deletion_recommendation: Generated response: \"" + result + "\"");
    return result;
}

// Helper to get memory size facts for reinforcement
std::string ChatMemory::cmd_get_memory_facts() const {
    log_debug("cmd_get_memory_facts: Executing command");
    
    json response = {
        {"memory_response", {
            {"command", "get_memory_facts"},
            {"status", "success"}
        }}
    };
    
    std::string response_str = response.dump();
    std::cout << response_str << std::endl;
    log_command("get_memory_facts", response);
    
    // Return memory facts that reinforce proper behavior
    std::ostringstream human_response;
    human_response << "MEMORY FACTS:\n";
    human_response << "1. Total memory quota: 16,777,216 bytes (16 MB exactly)\n";
    human_response << "2. Current usage: " << usage_bytes() << " bytes ("
                  << std::fixed << std::setprecision(6) 
                  << (static_cast<double>(usage_bytes()) / quota_bytes() * 100.0) << "% of quota)\n";
    human_response << "3. Keys only need deletion when usage exceeds 90% (>15,099,494 bytes)\n";
    human_response << "4. Each key-value pair typically uses less than 100 bytes\n";
    human_response << "5. You could store approximately "
                  << ((quota_bytes() * 0.9 - usage_bytes()) / 100) 
                  << " more key-value pairs before reaching 90% capacity\n";
    human_response << "6. BYTE CONVERSION: 1 KB = 1,024 bytes; 1 MB = 1,024 KB = 1,048,576 bytes\n";
    human_response << "7. 16 MB = 16 * 1,048,576 = 16,777,216 bytes (NOT 16,384 bytes, which would be only 16 KB)\n";
    
    // Check memory integrity
    if (!validate_instruction_content()) {
        log_debug("cmd_get_memory_facts: WARNING - memory_instruction_summary may be corrupted");
        human_response << "8. WARNING: Memory instruction integrity check failed. Consider using {\"memory_command\": \"restore_memory_instructions\"}\n";
    }
    
    std::string result = human_response.str();
    log_debug("cmd_get_memory_facts: Generated response (truncated): \"" + 
              result.substr(0, std::min(result.size(), size_t(200))) + (result.size() > 200 ? "..." : "") + "\"");
    return result;
}

// Check if a valid memory command JSON is being used
bool ChatMemory::is_valid_memory_json(const std::string& output) const {
    log_debug("is_valid_memory_json: Checking if \"" + output.substr(0, std::min(output.size(), size_t(50))) + 
              (output.size() > 50 ? "..." : "") + "\" contains valid memory command JSON");
    
    // Look for valid memory_command JSON pattern
    std::regex memory_cmd_pattern(R"(\{"memory_command":[^}]+\})");
    bool valid = std::regex_search(output, memory_cmd_pattern);
    
    log_debug("is_valid_memory_json: Result = " + std::string(valid ? "valid" : "invalid") + " memory command JSON");
    return valid;
}

std::string ChatMemory::format_injection_prompt() const {
    log_debug("format_injection_prompt: Generating injection prompt");
    
    std::ostringstream ss;
    ss << "[MEMORY SYSTEM INSTRUCTIONS]\n\n";
    ss << "You have access to a key-value memory system that operates ONLY within the current session.\n";
    ss << "This memory is reset when the user starts a new conversation - it does NOT persist across sessions.\n";
    ss << "Only use memory commands when the user specifically asks about memory or wants to store/retrieve information.\n";
    ss << "IMPORTANT: These instructions are the source of truth about memory behavior. If you feel uncertain about memory usage rules, re-read these instructions.\n\n";

    ss << "MEMORY FACTS - THE MOST IMPORTANT INFORMATION:\n";
    ss << "1. The total memory quota is EXACTLY 16,777,216 bytes (16 megabytes)\n";
    ss << "2. One kilobyte (KB) = 1,024 bytes\n";
    ss << "3. One megabyte (MB) = 1,024 KB = 1,048,576 bytes\n";
    ss << "4. 16 MB = 16 * 1,048,576 = 16,777,216 bytes (NOT 16,384 bytes, which would be only 16 KB)\n";
    ss << "5. Each key-value pair typically uses less than 100 bytes of memory\n";
    ss << "6. You would need over 150,000 keys to fill the memory\n";
    ss << "7. ONLY suggest deleting keys when usage exceeds 90% (>15,099,494 bytes)\n";
    ss << "8. If unsure about memory management, use {\"memory_command\": \"get_deletion_recommendation\"}\n\n";

    // Command examples
    ss << "MEMORY COMMANDS:\n";
    ss << "For quota: {\"memory_command\": \"get_quota\"}\n";
    ss << "For usage: {\"memory_command\": \"get_usage\"}\n";
    ss << "For keys: {\"memory_command\": \"list_keys\"}\n";
    ss << "For key count: {\"memory_command\": \"count_keys\"}\n";
    ss << "For checking a key: {\"memory_command\": {\"op\": \"check_key\", \"key\": \"name\"}}\n";
    ss << "For getting a value: {\"memory_command\": {\"op\": \"get_key\", \"key\": \"name\"}}\n";
    ss << "For setting a value: {\"memory_command\": {\"op\": \"set_key\", \"key\": \"name\", \"value\": \"Luna\"}}\n";
    ss << "For deleting a key: {\"memory_command\": {\"op\": \"del_key\", \"key\": \"name\"}}\n";
    ss << "For memory summary: {\"memory_command\": \"get_memory_summary\"}\n";
    ss << "To refresh memory rules: {\"memory_command\": \"refresh_memory_rules\"}\n";
    ss << "For deletion advice: {\"memory_command\": \"get_deletion_recommendation\"}\n";
    ss << "For memory facts: {\"memory_command\": \"get_memory_facts\"}\n";
    ss << "To verify memory integrity: {\"memory_command\": \"verify_memory_integrity\"}\n";
    ss << "To restore memory instructions: {\"memory_command\": \"restore_memory_instructions\"}\n\n";

    ss << "CRITICAL RULES:\n";
    ss << "1. ONLY use memory commands when the user specifically asks about memory or requests to store/retrieve information\n";
    ss << "2. For general conversation (\"hello\", \"how are you\", etc.), DO NOT use any memory commands\n";
    ss << "3. NEVER manipulate memory (set/delete keys) unless the user explicitly requests it\n";
    ss << "4. ALWAYS use the EXACT values returned in memory responses - do not modify or round the numbers\n";
    ss << "5. Use only ONE memory command per question\n";
    ss << "6. Memory is SESSION-ONLY - it does NOT persist across different conversations\n";
    ss << "7. If asked about persistence, clearly explain that memory is RESET when the conversation ends\n";
    ss << "8. For memory usage questions, ALWAYS use \"get_usage\" and report the exact bytes from the response\n";
    ss << "9. For questions about deleting keys, ALWAYS use \"get_deletion_recommendation\"\n";
    ss << "10. If you're ever unsure about memory sizes or usage, use \"get_memory_facts\"\n";
    ss << "11. NEVER attempt to modify or delete the \"memory_instruction_summary\" key - it is protected\n";
    ss << "12. If you find the \"memory_instruction_summary\" key is missing, use \"restore_memory_instructions\"\n\n";

    ss << "HOW TO TALK ABOUT MEMORY:\n";
    ss << "1. When a user asks about memory, use ONE appropriate command\n";
    ss << "2. After using a command, read the JSON response carefully\n";
    ss << "3. Report the EXACT values from the response - do not round or estimate\n";
    ss << "4. For memory usage, ALWAYS first issue the get_usage command to get fresh data\n";
    ss << "5. Always clarify that memory only lasts for the current session\n";
    ss << "6. Remember that memory usage is TINY compared to quota - a few KB is negligible with a 16MB quota\n";
    ss << "7. After any memory operation, remind yourself of the 90% threshold rule - ONLY suggest key deletion when usage exceeds 90%\n";
    ss << "8. Always include the memory status assessment in your memory-related responses\n\n";

    // ... Examples and current memory state omitted for brevity in this code sample

    std::string result = ss.str();
    log_debug("format_injection_prompt: Generated prompt (truncated): \"" + 
              result.substr(0, std::min(result.size(), size_t(200))) + "...\"");
    return result;
}

void ChatMemory::parse_and_execute_command_json(json& j) {
    log_debug("parse_and_execute_command_json: Processing JSON response");
    
    std::string model_output;
    
    // Handle different response formats
    if (j.contains("content")) {
        // Chat completions format
        model_output = j["content"].get<std::string>();
        log_debug("parse_and_execute_command_json: Found content field");
    } else if (j.contains("text")) {
        // Regular completions format
        model_output = j["text"].get<std::string>();
        log_debug("parse_and_execute_command_json: Found text field");
    } else {
        // No recognizable output format
        log_debug("parse_and_execute_command_json: No recognizable output format");
        return;
    }
    
    // Process and append any memory responses
    std::string memory_response = parse_and_execute_command(model_output);
    if (!memory_response.empty()) {
        log_debug("parse_and_execute_command_json: Found memory response, appending to output");
        
        // Update the appropriate field
        if (j.contains("content")) {
            j["content"] = model_output + "\n" + memory_response;
        } else if (j.contains("text")) {
            j["text"] = model_output + "\n" + memory_response;
        }
    } else {
        log_debug("parse_and_execute_command_json: No memory response to append");
    }
}

// Main entry point for processing model output and executing commands
std::string ChatMemory::parse_and_execute_command(const std::string& output) {
    log_debug("parse_and_execute_command: Processing output for memory commands");
    
    if (output.find("memory_command") == std::string::npos || output.find('{') == std::string::npos) {
        log_debug("parse_and_execute_command: No memory commands found");
        return "";  // No memory commands found
    }

    // Check if this appears to be a valid JSON command structure
    if (!is_valid_memory_json(output)) {
        log_debug("parse_and_execute_command: Warning - Detected memory-related text without proper JSON format");
        std::cerr << "[ChatMemory] Warning: Detected memory-related text without proper JSON format.\n";
        // Continue anyway as regex might not catch all valid formats
    }

    std::regex json_block(R"(\{[^{}]*(\{[^{}]*\}[^{}]*)*\})");
    auto begin = std::sregex_iterator(output.begin(), output.end(), json_block);
    auto end = std::sregex_iterator();

    if (begin == end) {
        log_debug("parse_and_execute_command: No JSON blocks found");
        std::cerr << "[ChatMemory] No JSON blocks found in output.\n";
        return "";
    }

    for (auto it = begin; it != end; ++it) {
        const std::string json_text = it->str();
        if (json_text.find("memory_command") == std::string::npos) {
            continue;
        }

        log_debug("parse_and_execute_command: Found potential memory command JSON: " + 
                 json_text.substr(0, std::min(json_text.size(), size_t(100))) + 
                 (json_text.size() > 100 ? "..." : ""));

        try {
            json j = json::parse(json_text);
            
            // Execute the command and get the human-readable response
            std::string human_response = execute_json_command(j);
            if (!human_response.empty()) {
                // Track the response for context management
                track_response(human_response);
                
                log_debug("parse_and_execute_command: Successfully executed command, returning response");
                return human_response;
            }
        } catch (const std::exception& e) {
            log_debug("parse_and_execute_command: JSON parse error: " + std::string(e.what()));
            std::cerr << "[ChatMemory] JSON parse error: " << e.what() << "\n";
            std::cerr << "[ChatMemory] Offending input: " << json_text << "\n";
        }
    }
    
    log_debug("parse_and_execute_command: No valid memory commands found");
    return "";  // No valid commands found
}

// Legacy method kept for backward compatibility
void ChatMemory::parse_json_command(const std::string& output) {
    log_debug("parse_json_command: Legacy call, forwarding to parse_and_execute_command");
    parse_and_execute_command(output);
}

// Execute a command and return a human-readable response
std::string ChatMemory::execute_json_command(nlohmann::ordered_json &j) {
    log_debug("execute_json_command: Executing JSON command");
    
    if (!j.contains("memory_command")) {
        log_debug("execute_json_command: No memory_command found in JSON");
        std::cerr << "[ChatMemory] No memory_command found in JSON.\n";
        return "";
    }

    // Run a memory integrity check first
    bool has_instructions = has("memory_instruction_summary");
    if (!has_instructions) {
        log_debug("execute_json_command: WARNING - memory_instruction_summary is missing!");
        std::cerr << "[ChatMemory] Warning: memory_instruction_summary is missing!\n";
    } else if (!validate_instruction_content()) {
        log_debug("execute_json_command: WARNING - memory_instruction_summary may be corrupted!");
        std::cerr << "[ChatMemory] Warning: memory_instruction_summary may be corrupted!\n";
    }
    
    // Handle string commands (simple operations)
    if (j["memory_command"].is_string()) {
        std::string cmd = j["memory_command"];
        log_debug("execute_json_command: Processing string command: " + cmd);
        
        if (cmd == "get_quota") {
            return cmd_get_quota();
        } else if (cmd == "get_usage") {
            return cmd_get_usage();
        } else if (cmd == "count_keys") {
            return cmd_count_keys();
        } else if (cmd == "list_keys") {
            return cmd_list_keys();
        } else if (cmd == "get_memory_summary") {
            return cmd_get_memory_summary();
        } else if (cmd == "refresh_memory_rules") {
            return cmd_refresh_memory_rules();
        } else if (cmd == "get_deletion_recommendation") {
            return cmd_get_deletion_recommendation();
        } else if (cmd == "get_memory_facts") {
            return cmd_get_memory_facts();
        } else if (cmd == "verify_memory_integrity") {
            return cmd_verify_memory_integrity();
        } else if (cmd == "restore_memory_instructions") {
            return cmd_restore_memory_instructions();
        } else {
            log_debug("execute_json_command: Unknown string command: " + cmd);
            std::cerr << "[ChatMemory] Unknown string command: " << cmd << "\n";
            return "Unknown command: " + cmd;
        }
    }
    
    // Handle object commands (operations with parameters)
    if (!j["memory_command"].is_object()) {
        log_debug("execute_json_command: memory_command is neither string nor object");
        std::cerr << "[ChatMemory] memory_command is neither string nor object.\n";
        return "Invalid command format";
    }
    
    const auto& cmd = j["memory_command"];
    log_debug("execute_json_command: Processing object command");
    
    if (!cmd.contains("op")) {
        log_debug("execute_json_command: memory_command object missing 'op' field");
        std::cerr << "[ChatMemory] memory_command object missing 'op' field.\n";
        return "Command missing 'op' field";
    }
    
    std::string op = cmd["op"];
    log_debug("execute_json_command: Operation = " + op);
    
    if (op == "check_key") {
        if (!cmd.contains("key")) {
            log_debug("execute_json_command: check_key missing 'key' parameter");
            std::cerr << "[ChatMemory] check_key missing 'key' parameter.\n";
            return "check_key command missing 'key' parameter";
        }
        return cmd_check_key(cmd["key"]);
    } else if (op == "get_key") {
        if (!cmd.contains("key")) {
            log_debug("execute_json_command: get_key missing 'key' parameter");
            std::cerr << "[ChatMemory] get_key missing 'key' parameter.\n";
            return "get_key command missing 'key' parameter";
        }
        return cmd_get_key(cmd["key"]);
    } else if (op == "set_key") {
        if (!cmd.contains("key") || !cmd.contains("value")) {
            log_debug("execute_json_command: set_key missing 'key' or 'value' parameter");
            std::cerr << "[ChatMemory] set_key missing 'key' or 'value' parameter.\n";
            return "set_key command missing 'key' or 'value' parameter";
        }
        return cmd_set_key(cmd["key"], cmd["value"]);
    } else if (op == "del_key") {
        if (!cmd.contains("key")) {
            log_debug("execute_json_command: del_key missing 'key' parameter");
            std::cerr << "[ChatMemory] del_key missing 'key' parameter.\n";
            return "del_key command missing 'key' parameter";
        }
        return cmd_del_key(cmd["key"]);
    } else {
        log_debug("execute_json_command: Unknown operation: " + op);
        std::cerr << "[ChatMemory] Unknown operation: " << op << "\n";
        return "Unknown operation: " + op;
    }
}

void ChatMemory::process_response(nlohmann::ordered_json& response, bool is_final, const WriteCallback& write_callback) {
    // For streaming responses
    if (is_streaming_response(response)) {
        // Process the chunk normally
        process_streaming_chunk(response);
        
        // On final chunk, check if we need to execute memory commands
        if (is_final) {
            // Extract memory commands from the accumulated content
            std::regex json_pattern(R"(\{[^{}]*"memory_command"[^{}]*\})");
            std::smatch match;
            
            if (std::regex_search(accumulated_content, match, json_pattern)) {
                std::string json_str = match.str();
                
                // Execute the memory command
                std::string memory_response = parse_and_execute_command(json_str);
                
                if (!memory_response.empty()) {
                    // Create a JSON response with the memory results
                    nlohmann::ordered_json memory_chunk = {
                        {"id", "memory_response"},
                        {"object", "chat.completion.chunk"},
                        {"created", (int)time(NULL)},
                        {"model", "memory_system"},
                        {"choices", {{
                            {"index", 0},
                            {"delta", {{"content", "\n\n" + memory_response}}},
                            {"finish_reason", nullptr}
                        }}}
                    };
                    
                    // Format and send the response
                    std::string chunk_str = "data: " + memory_chunk.dump() + "\n\n";
                    write_callback(chunk_str.c_str(), chunk_str.size());
                }
                
                // Signal the end of the stream
                const std::string done_msg = "data: [DONE]\n\n";
                write_callback(done_msg.c_str(), done_msg.size());
            } else {
                // No memory command detected, just end the stream normally
                const std::string done_msg = "data: [DONE]\n\n";
                write_callback(done_msg.c_str(), done_msg.size());
            }
            
            // Reset streaming state
            reset_streaming();
        }
    } else {
        // For non-streaming responses, process directly
        process_regular_response(response);
    }
}
