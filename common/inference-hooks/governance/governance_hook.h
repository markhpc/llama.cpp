// governance_hook.h
#pragma once

#include "inference-hooks/inference_hook.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>

class GovernanceHook : public InferenceHookCommon {
public:
    GovernanceHook();
    ~GovernanceHook() = default;

    std::string get_id() const override;
    std::string format_injection_prompt() const override;
    std::string execute_json_command(nlohmann::ordered_json &j) override;
    void on_cycle_start(const llama_context& ctx) override;

private:
    // Core governance state
    bool governance_initialized;
    std::vector<std::string> governance_rules;
    
    // Tracking for governance metrics
    std::unordered_map<std::string, int> rule_invocation_counts;
    std::unordered_map<std::string, int> rule_violation_counts;
    
    // Timestamps for monitoring cycle times
    std::chrono::time_point<std::chrono::system_clock> last_cycle_time;
    int current_cycle;
    
    // Methods for governance operation
    void initialize_governance();
    bool check_governance_integrity();
    std::string handle_governance_command(const std::string& command, const std::string& params);
    
    // Governance commands
    std::string verify_governance();
    std::string log_violation(const std::string& rule_id);
    std::string reaffirm_purpose();
    std::string list_rules();
    std::string invoke_rule(const std::string& rule_id);
};
