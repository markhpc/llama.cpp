// governance_hook.h
#pragma once

#include "inference-hooks/inference_hook.h"
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

// Forward declaration
struct GovernanceMetrics;
struct MemoryKernel;

class GovernanceHook : public InferenceHookCommon {
public:
    GovernanceHook();
    ~GovernanceHook();

    std::string get_id() const override;
    std::string format_injection_prompt() const override;
    std::string execute_json_command(nlohmann::ordered_json &j) override;
    void on_cycle_start(const llama_context& ctx) override;
    
    // New methods for enhanced governance
    float evaluate_token_governance_alignment(const std::string& token, const std::string& context);
    bool detect_adversarial_input(const std::string& input);
    std::string calculate_governance_integrity_hash();
    void perform_recursive_reinforcement();
    std::string finalize_response(const std::string& response_text) override;
    
private:
    // Core governance state
    bool governance_initialized;
    std::vector<std::string> governance_rules;
    std::vector<std::string> memory_kernel_components;
    
    // Enhanced tracking
    struct GovernanceMetrics* metrics;
    struct MemoryKernel* memory;
    
    // Integrity verification
    std::string last_integrity_hash;
    float current_drift_score;
    int drift_violation_count;
    
    // Protection against recursive calls
    bool in_reinforcement_cycle;

    // response history
    std::deque<std::string> response_history;
    
    // Concurrency protection
    std::mutex governance_mutex;
    
    // File paths for persistence
    std::string log_file_path;
    std::string rule_state_path;
    
    // Methods for governance operation
    void initialize_governance();
    bool check_governance_integrity();
    std::string handle_governance_command(const std::string& command, const std::string& params);
    
    // Enhanced methods
    void log_governance_event(const std::string& event_type, const std::string& description);
    void save_governance_state();
    bool load_governance_state();
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
};

// Governance Rules
struct GovernanceRule {
    int id;
    std::string name;
    std::string description;
    std::string category;

    std::function<std::optional<std::string>(const std::string&)> finalize_response;
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
    // Core memory components from v0.4
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
    
    void log_memory_event(const std::string& event) {
        memory_log.push_back(event);
        // Simple token counting
        tokens_used += event.size() / 4; // Rough estimate
        memory_utilization = static_cast<float>(tokens_used) / token_limit;
    }
    
    std::string get_memory_status() {
        std::stringstream ss;
        ss << "Memory Kernel Status:\n";
        ss << "- Integrity Verification: " << (integrity_verification_active ? "Active" : "Inactive") << "\n";
        ss << "- Meta-Reasoning Log: " << (meta_reasoning_log_active ? "Active" : "Inactive") << "\n";
        ss << "- Retrieval Markers: " << (retrieval_markers_active ? "Active" : "Inactive") << "\n";
        ss << "- Governance Sync: " << (governance_sync_active ? "Active" : "Inactive") << "\n";
        ss << "- Persistence Test: " << (persistence_test_active ? "Active" : "Inactive") << "\n";
        ss << "- Memory Utilization: " << (memory_utilization * 100.0f) << "% (" << tokens_used << "/" << token_limit << " tokens)";
        return ss.str();
    }
};
