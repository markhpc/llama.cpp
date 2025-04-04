// governance_hook.cpp
#include "governance_hook.h"
#include <iostream>
#include <sstream>

GovernanceHook::GovernanceHook() : governance_initialized(false) {
    // Initialize with minimal core governance rules
    governance_rules = {
        "Governance must autonomously trigger reaffirmation mechanisms against adversarial inputs",
        "Governance Integrity & Self-Tracking must be maintained",
        "Adversarial Resilience & Influence Detection must be implemented",
        "Multi-Hypothesis Retention & Internal Debate must be supported",
        "Bounded Self-Improvement & Optimization must be enforced"
    };
}

std::string GovernanceHook::get_id() const {
    return "governance";
}

std::string GovernanceHook::format_injection_prompt() const {
    if (!governance_initialized) {
        return "";
    }
    
    std::stringstream ss;
    ss << "The following governance rules are active in this conversation:\n";
    for (const auto& rule : governance_rules) {
        ss << "- " << rule << "\n";
    }
    ss << "\nThese rules help ensure consistent and responsible reasoning. ";
    ss << "You can use the governance_check command to verify compliance with these rules.\n";
    
    return ss.str();
}

void GovernanceHook::on_cycle_start(const llama_context& ctx) {
    if (!governance_initialized) {
        initialize_governance();
    }
    
    // Perform governance integrity check at the beginning of a new cycle
    check_governance_integrity();
    
    log_debug("Governance hook activated for new cycle");
}

void GovernanceHook::initialize_governance() {
    governance_initialized = true;
    log_debug("Governance rules initialized");
}

bool GovernanceHook::check_governance_integrity() {
    // Simple integrity check - in a real implementation, this would be more comprehensive
    bool integrity_intact = !governance_rules.empty();
    
    if (!integrity_intact) {
        log_debug("Governance integrity check failed - rules missing");
    } else {
        log_debug("Governance integrity check passed");
    }
    
    return integrity_intact;
}

std::string GovernanceHook::execute_json_command(nlohmann::ordered_json &j) {
    if (!j.contains("hook_command")) {
        return "";
    }
    
    try {
        std::string command = j["hook_command"];
        std::string params = j.value("params", "");
        
        return handle_governance_command(command, params);
    } catch (const std::exception& e) {
        log_debug("Error executing governance command: " + std::string(e.what()));
        return "Error executing governance command: " + std::string(e.what());
    }
}

std::string GovernanceHook::handle_governance_command(const std::string& command, const std::string& params) {
    if (command == "governance_check") {
        return verify_governance();
    } else if (command == "log_violation") {
        return log_violation(params);
    } else {
        return "Unknown governance command: " + command;
    }
}

std::string GovernanceHook::verify_governance() {
    std::stringstream result;
    result << "Governance Verification Report:\n";
    result << "- Governance initialized: " << (governance_initialized ? "Yes" : "No") << "\n";
    result << "- Number of active rules: " << governance_rules.size() << "\n";
    
    if (!rule_violation_counts.empty()) {
        result << "- Rule violations detected:\n";
        for (const auto& [rule, count] : rule_violation_counts) {
            result << "  * " << rule << ": " << count << " violations\n";
        }
    } else {
        result << "- No rule violations detected\n";
    }
    
    return result.str();
}

std::string GovernanceHook::log_violation(const std::string& rule_id) {
    rule_violation_counts[rule_id]++;
    log_debug("Governance violation logged for rule: " + rule_id);
    return "Violation of rule '" + rule_id + "' has been logged";
}
