// governance_hook.h
#pragma once

#include "inference-hooks/inference_hook.h"
#include <string>
#include <vector>
#include <unordered_map>

class GovernanceHook : public InferenceHookCommon {
public:
    GovernanceHook();
    ~GovernanceHook() = default;

    std::string get_id() const override;
    std::string format_injection_prompt() const override;
    std::string execute_json_command(nlohmann::ordered_json &j) override;
    void on_cycle_start(const llama_context& ctx) override;

private:
    // Governance state tracking
    bool governance_initialized;
    std::vector<std::string> governance_rules;
    
    // Simple memory for tracking rule adherence
    std::unordered_map<std::string, int> rule_violation_counts;
    
    // Methods for governance enforcement
    void initialize_governance();
    bool check_governance_integrity();
    std::string handle_governance_command(const std::string& command, const std::string& params);
    std::string verify_governance();
    std::string log_violation(const std::string& rule_id);
};
