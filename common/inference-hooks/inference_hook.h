// inference_hook.h
#pragma once

#include "json.hpp"
#include "llama.h"

#include <deque>
#include <functional>
#include <string>

// Debug logging control
#ifndef INFERENCE_HOOK_DEBUG
#define INFERENCE_HOOK_DEBUG 1  // Set to 1 to enable debug logging at compile time
#endif

// Forward declarations
using json = nlohmann::ordered_json;
using WriteCallback = std::function<void(const char*, size_t)>;

// Abstract base class defining the interface
class InferenceHook {
public:
    virtual ~InferenceHook() = default;
    
    // Core interface required by server.cpp
    virtual std::string get_id() const = 0;
    virtual void process_response(json& response, bool is_final, const WriteCallback& write_callback) = 0;
    virtual std::string handle_text_command(const std::string& output) = 0;
    virtual void handle_json_command(json& j) = 0;
    virtual std::string format_injection_prompt() const = 0;
    virtual void on_cycle_start(const llama_context& ctx) = 0; 
    virtual std::string finalize_response(const std::string& response_text) = 0;
    
    // New streaming interface
    struct StreamingCheckResult {
        bool should_inject_message;
        std::string message;
        bool is_feedback_only; // New flag to indicate feedback-only messages
    
        StreamingCheckResult() : should_inject_message(false), is_feedback_only(false) {}
        StreamingCheckResult(const std::string& msg, bool feedback_only = false) 
            : should_inject_message(true), message(msg), is_feedback_only(feedback_only) {}

        operator bool() const { return should_inject_message; }
    };
    virtual StreamingCheckResult check_streaming_content(const std::string& current_content) = 0;

    // Feedback
    virtual std::string get_feedback() const = 0;
    virtual bool has_feedback() const = 0;

};

// A common implementation base class that can be reused
class InferenceHookCommon : public InferenceHook {
public:
    // Implement the interface from InferenceHook 
    std::string get_id() const override = 0;
    void process_response(json& response, bool is_final, const WriteCallback& write_callback) override;
    std::string handle_text_command(const std::string& output) override;
    void handle_json_command(json& j) override;
    std::string format_injection_prompt() const override = 0;
    virtual void on_cycle_start(const llama_context& ctx) {
        (void) ctx;
    }
    virtual std::string finalize_response(const std::string& response_text) {
        return response_text;
    }

    StreamingCheckResult check_streaming_content(const std::string& current_content) override {
       return StreamingCheckResult(); // Default implementation returns no issues
    }

    virtual std::string get_feedback() const override {
        return ""; // Default is no feedback
    }
    
    virtual bool has_feedback() const override {
        return false; // Default is no feedback
    }

    protected:
    // Thresholds for streaming content checks
    bool streaming_checks_enabled = true;
    size_t min_streaming_check_length = 50;  // Only check once we have enough content
    size_t streaming_check_interval = 30;     // Check every N tokens/chunks
    size_t streaming_check_counter = 0;      // Counter for tracking when to check
    bool in_streaming_mode = false;

    // Context Management
    std::deque<std::string> recent_responses;
    size_t max_context_responses = 5;  // Default limit

    // Default logging methods
    static bool is_debug_enabled();
    void log_debug(const std::string& message) const;
    void log_command(const std::string& command, const nlohmann::ordered_json& response) const;

    // Default implementations of helper methods
    virtual std::string execute_json_command(nlohmann::ordered_json &j);
    bool is_streaming_response(const json& j) const;
    bool is_valid_hook_json(const std::string& output) const;
    void process_streaming_chunk(json& j);
    void process_regular_response(json& j);
    void reset_streaming();
    void track_response(const std::string& response);

    // Shared state
    std::string accumulated_content;
};

// Global function to get a InferenceHook object by ID
InferenceHook& get_or_create_inference_hook(const std::string& id);
