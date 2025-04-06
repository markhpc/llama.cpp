// governance_hook.h
#pragma once

#include "inference-hooks/inference_hook.h"
#include "governance_registry.h"
#include "json.hpp"

#include <deque>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <fstream>
#include <functional>
#include <optional>
#include <random>

// Forward declarations
using json = nlohmann::ordered_json;
struct GovernanceFeedback;
struct GovernanceMetrics;
struct MemoryKernel;

enum class FeedbackSeverity {
    DIAGNOSTIC,  // Developer-only info
    WARNING,     // Potential issues
    CRITICAL     // Serious violations
};

class GovernanceHook : public InferenceHookCommon {
public:
    GovernanceHook();
    ~GovernanceHook();

    // InferenceHook interface implementation
    std::string get_id() const override;
    std::string format_injection_prompt() const override;
    std::string execute_json_command(json &j) override;
    void on_cycle_start(const llama_context& ctx) override;
    std::string finalize_response(const std::string& response_text) override;
    
    // Public methods for rule evaluation
    float evaluate_token_governance_alignment(const std::string& token, const std::string& context);
    bool detect_adversarial_input(const std::string& input);

    // For feedback
    void add_feedback(int rule_id, const std::string& message,
                  FeedbackSeverity severity = FeedbackSeverity::DIAGNOSTIC);
    std::string get_feedback() const override;
    bool has_feedback() const override;

    // For streaming
    StreamingCheckResult check_streaming_content(const std::string& current_content) override;

private:
    // Core governance state
    bool governance_initialized;
    std::vector<std::string> memory_kernel_components;

    // Feedback
    std::vector<GovernanceFeedback> feedback_channel;
    bool in_debug_mode = false;
    bool show_trace_requested = false;
    
    // Enhanced tracking
    struct GovernanceMetrics* metrics;
    struct MemoryKernel* memory;
    
    // Integrity verification
    std::string last_integrity_hash;
    float current_drift_score;
    int drift_violation_count;
    
    // Protection against recursive calls
    bool in_reinforcement_cycle;

    // Response history for repetition detection
    std::deque<std::string> response_history;
    
    // Concurrency protection
    std::mutex governance_mutex;
    
    // File paths for persistence
    std::string log_file_path;
    std::string rule_state_path;
    
    // Registry initialization
    void initialize_rule_registry();
    
    // Core methods for governance operation
    void initialize_governance();
    bool check_governance_integrity();
    std::string handle_governance_command(const std::string& command, const std::string& params);
    std::string calculate_governance_integrity_hash();
    void perform_recursive_reinforcement();
    
    // Enhanced methods
    void log_governance_event(const std::string& event_type, const std::string& description);
    void save_governance_state();
    bool load_governance_state();
    std::function<std::optional<std::pair<std::string, EnforcementMethod>>(const std::string&)> create_rule_logic(int rule_id, bool is_streaming = false);
    void update_drift_metrics(float new_violation_score);
    
    // Governance commands
    std::string verify_governance();
    std::string log_violation(const std::string& rule_id);
    std::string reaffirm_purpose();
    std::string list_rules();
    std::string invoke_rule(const std::string& rule_id);
    std::string check_memory_kernel();
    std::string check_adversarial_detection();
    std::string perform_self_verification();

    // Similarity tooling
    double levenshtein_similarity(const std::string& s1, const std::string& s2);
    
    // Helper for logging
    void log_debug(const std::string& message);
};

struct GovernanceFeedback {
    int rule_id;
    std::string message;
    FeedbackSeverity severity;
    bool visible_to_user;
    std::chrono::system_clock::time_point timestamp;
};

// Governance metrics tracking
struct GovernanceMetrics {
    int current_cycle;
    std::chrono::time_point<std::chrono::system_clock> last_cycle_time;
    
    // Rule tracking
    std::unordered_map<std::string, int> rule_invocation_counts;
    std::unordered_map<std::string, int> rule_violation_counts;
    
    // Enhanced metrics
    float total_integrity_score;
    float average_drift;
    int consecutive_violations;
    int reinforcement_cycles;
    
    // Adversarial detection
    int adversarial_attempts_detected;
    float adversarial_sensitivity;
    
    GovernanceMetrics() : 
        current_cycle(0),
        last_cycle_time(std::chrono::system_clock::now()),
        total_integrity_score(1.0f),
        average_drift(0.0f),
        consecutive_violations(0),
        reinforcement_cycles(0),
        adversarial_attempts_detected(0),
        adversarial_sensitivity(0.7f) {}
};

// Memory kernel for persistence
struct MemoryKernel {
    // Core memory components
    bool integrity_verification_active;
    bool meta_reasoning_log_active;
    bool retrieval_markers_active;
    bool governance_sync_active;
    bool persistence_test_active;
    
    // Memory constraints awareness
    const int token_limit = 32768;
    int tokens_used;
    float memory_utilization;
    
    // Memory log
    std::vector<std::string> memory_log;
    
    MemoryKernel() :
        integrity_verification_active(false),
        meta_reasoning_log_active(false),
        retrieval_markers_active(false),
        governance_sync_active(false),
        persistence_test_active(false),
        tokens_used(0),
        memory_utilization(0.0f) {}
    
    void log_memory_event(const std::string& event);
    std::string get_memory_status();
};
