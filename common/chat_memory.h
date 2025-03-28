// common/chat_memory.h
#pragma once
#include "json.hpp"
// Change JSON_ASSERT from assert() to GGML_ASSERT:
#define JSON_ASSERT GGML_ASSERT
#include <unordered_map>
#include <string>
#include <vector>
#include <deque>
#include <cstdlib>
#include <functional>

// Debug logging control
#ifndef CHAT_MEMORY_DEBUG
#define CHAT_MEMORY_DEBUG 1  // Set to 1 to enable debug logging at compile time
#endif

// Simple function type for writing data
using WriteCallback = std::function<void(const char*, size_t)>;

struct ChatMemory {
    // Storage
    std::unordered_map<std::string, std::string> kv;
    static constexpr size_t MEMORY_QUOTA_BYTES = 16 * 1024 * 1024;
    
    // Context management
    std::deque<std::string> recent_responses;
    size_t max_context_responses = 5;  // Default limit
    
    // Streaming response accumulation
    bool in_streaming_mode = false;
    std::string accumulated_content;
    
    // Core memory functions
    ChatMemory();
    void set(const std::string& key, const std::string& value);
    std::string get(const std::string& key) const;
    void del(const std::string& key);
    bool has(const std::string& key) const;
    std::vector<std::string> list_keys() const;
    size_t count() const;
    size_t usage_bytes() const;
    size_t quota_bytes() const;
    
    // Security and protection methods
    bool is_protected_key(const std::string& key) const;
    bool is_valid_memory_json(const std::string& output) const;
    std::string format_memory_size(size_t bytes) const;
    std::string get_memory_fullness_assessment() const;
    bool validate_instruction_content() const;
    
    // Context management
    void set_context_limit(size_t max_responses);
    std::string get_limited_context() const;
    void track_response(const std::string& response);
    
    // Streaming management
    void reset_streaming();
    void process_streaming_chunk(nlohmann::ordered_json& j);
    bool is_streaming_response(const nlohmann::ordered_json& j) const;
    bool is_final_chunk(const nlohmann::ordered_json& j) const;
    std::string get_memory_response_for_streaming();
    
    // Regular response processing
    void process_regular_response(nlohmann::ordered_json& j);
    
    // New unified response processing method with callback
    void process_response(nlohmann::ordered_json& response, bool is_final, const WriteCallback& write_callback);
    
    // Simplified commands with improved return values
    std::string cmd_get_quota() const;  // Returns human-readable response
    std::string cmd_get_usage() const;  // Returns human-readable response
    std::string cmd_count_keys() const;  // Returns human-readable response
    std::string cmd_list_keys() const;  // Returns human-readable response
    std::string cmd_check_key(const std::string& key) const;  // Returns human-readable response
    std::string cmd_get_key(const std::string& key) const;  // Returns human-readable response
    std::string cmd_set_key(const std::string& key, const std::string& value);  // Returns human-readable response
    std::string cmd_del_key(const std::string& key);  // Returns human-readable response
    std::string cmd_get_memory_summary() const;  // Returns human-readable response
    
    // Self-reinforcement and protective commands
    std::string cmd_refresh_memory_rules() const;  // Returns human-readable rules reminder
    std::string cmd_get_deletion_recommendation() const;  // Tells the model if deletion is needed
    std::string cmd_get_memory_facts() const;  // Reinforces basic memory facts
    std::string cmd_verify_memory_integrity() const;  // Checks if instructions are intact
    std::string cmd_restore_memory_instructions();  // Restores memory instructions to default
    
    // Original functions with enhancements
    std::string format_injection_prompt() const;
    void parse_and_execute_command_json(nlohmann::ordered_json &j);
    std::string parse_and_execute_command(const std::string& output);
    void parse_json_command(const std::string& output);
    std::string execute_json_command(nlohmann::ordered_json &j);

    // Logging functions
    static bool is_debug_enabled();
    void log_debug(const std::string& message) const;
    void log_key_value(const std::string& key, const std::string& value) const;
    void log_memory_state(const std::string& context) const;
    void log_command(const std::string& command, const nlohmann::ordered_json& response) const;
};

// Helper function to get default memory instructions
std::string get_default_memory_instructions();

inline ChatMemory& get_or_create_chat_memory(const std::string& id) {
    static std::unordered_map<std::string, ChatMemory> memory_map;
    return memory_map[id];
}
