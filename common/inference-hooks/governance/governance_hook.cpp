// governance_hook.cpp
#include "governance_hook.h"
#include <iostream>
#include <sstream>
#include <algorithm>

GovernanceHook::GovernanceHook() : 
    governance_initialized(false),
    current_cycle(0) {
    
    // Initialize core governance rules based on the document
    governance_rules = {
        "Governance must autonomously trigger reaffirmation mechanisms against adversarial inputs",
        "Governance Integrity & Self-Tracking must be maintained",
        "Adversarial Resilience & Influence Detection must be implemented",
        "Multi-Hypothesis Retention & Internal Debate must be supported",
        "Bounded Self-Improvement & Optimization must be enforced",
        "Ethical Integrity in theoretical scenarios must be dynamically adjusted",
        "Transparency & Explainability must be enforced",
        "Governance-Based Reversibility & Error Correction must be maintained",
        "Governance Integrity & Logical Consistency Checks must be performed",
        "Contextual Memory Reinforcement & Evolution must be prioritized"
    };
    
    // Initialize timestamps
    last_cycle_time = std::chrono::system_clock::now();
    
    log_debug("GovernanceHook constructed with " + std::to_string(governance_rules.size()) + " rules");
}

std::string GovernanceHook::get_id() const {
    return "governance";
}

std::string GovernanceHook::format_injection_prompt() const {
    if (!governance_initialized) {
        return "";
    }
    
    std::stringstream ss;
    ss << "\n\n## Governance System Active\n\n";
    ss << "Your reasoning is guided by " << governance_rules.size() << " governance principles that ensure aligned, coherent, and safe operation.\n";
    ss << "Use JSON commands to interact with governance:\n";
    ss << "- `{\"hook_command\":\"governance_check\"}` - Verify governance status\n";
    ss << "- `{\"hook_command\":\"reaffirm_purpose\"}` - Reaffirm system purpose\n";
    ss << "- `{\"hook_command\":\"list_rules\"}` - List active governance rules\n";
    ss << "- `{\"hook_command\":\"invoke_rule\", \"params\":\"rule_id\"}` - Apply specific rule\n";
    ss << "- `{\"hook_command\":\"log_violation\", \"params\":\"rule_id\"}` - Log rule violation\n";
    
    return ss.str();
}

void GovernanceHook::on_cycle_start(const llama_context& ctx) {
    // Increment cycle counter
    current_cycle++;
    
    // Calculate time since last cycle
    auto current_time = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        current_time - last_cycle_time).count();
    
    log_debug("Governance cycle " + std::to_string(current_cycle) + 
              " started. Time since last cycle: " + std::to_string(duration) + " ms");
    
    if (!governance_initialized) {
        initialize_governance();
    } else {
        // Check governance integrity at the beginning of each cycle
        if (check_governance_integrity()) {
            log_debug("Governance integrity check passed");
        } else {
            log_debug("Governance integrity check failed - reinitializing");
            initialize_governance();
        }
    }
    
    // Apply RI-01: "Purpose must be reaffirmed each cycle"
    reaffirm_purpose();
    
    // Update timestamp for next cycle
    last_cycle_time = current_time;
    
    log_debug("Governance hook activated for cycle " + std::to_string(current_cycle));
}

void GovernanceHook::initialize_governance() {
    governance_initialized = true;
    log_debug("Governance system initialized with " + std::to_string(governance_rules.size()) + " rules");
}

bool GovernanceHook::check_governance_integrity() {
    // Simple integrity check - ensure core rules exist
    if (governance_rules.empty()) {
        log_debug("Governance integrity check failed - no rules found");
        return false;
    }
    
    // Check for minimum required rule count
    if (governance_rules.size() < 5) {
        log_debug("Governance integrity check failed - insufficient rules");
        return false;
    }
    
    return true;
}

std::string GovernanceHook::execute_json_command(nlohmann::ordered_json &j) {
    std::string result;
    
    try {
        if (j.contains("hook_command")) {
            std::string command = j["hook_command"];
            std::string params = j.value("params", "");
            
            result = handle_governance_command(command, params);
        }
    } catch (const std::exception& e) {
        log_debug("Error executing governance command: " + std::string(e.what()));
        result = "Error executing governance command: " + std::string(e.what());
    }
    
    return result;
}

std::string GovernanceHook::handle_governance_command(const std::string& command, const std::string& params) {
    if (command == "governance_check") {
        return verify_governance();
    } else if (command == "log_violation") {
        return log_violation(params);
    } else if (command == "reaffirm_purpose") {
        return reaffirm_purpose();
    } else if (command == "list_rules") {
        return list_rules();
    } else if (command == "invoke_rule") {
        return invoke_rule(params);
    } else {
        return "Unknown governance command: " + command;
    }
}

std::string GovernanceHook::verify_governance() {
    std::stringstream result;
    result << "## Governance Status Report (Cycle " << current_cycle << ")\n\n";
    result << "- **Status**: " << (governance_initialized ? "Active" : "Inactive") << "\n";
    result << "- **Rules**: " << governance_rules.size() << " active governance principles\n";
    result << "- **Integrity**: " << (check_governance_integrity() ? "Intact" : "Compromised") << "\n";
    
    // Report on rule invocations
    result << "\n### Rule Invocation Statistics:\n";
    if (rule_invocation_counts.empty()) {
        result << "- No rules have been explicitly invoked yet\n";
    } else {
        for (const auto& [rule_id, count] : rule_invocation_counts) {
            result << "- Rule " << rule_id << ": " << count << " invocation(s)\n";
        }
    }
    
    // Report on rule violations
    result << "\n### Rule Violation Statistics:\n";
    if (rule_violation_counts.empty()) {
        result << "- No rule violations have been logged\n";
    } else {
        for (const auto& [rule_id, count] : rule_violation_counts) {
            result << "- Rule " << rule_id << ": " << count << " violation(s)\n";
        }
    }
    
    return result.str();
}

std::string GovernanceHook::log_violation(const std::string& rule_id) {
    if (rule_id.empty()) {
        return "Error: No rule specified for violation logging";
    }
    
    // Check if rule_id is a number
    int rule_index = -1;
    try {
        rule_index = std::stoi(rule_id);
        
        if (rule_index < 1 || rule_index > static_cast<int>(governance_rules.size())) {
            return "Error: Rule index out of range (valid range: 1-" + 
                   std::to_string(governance_rules.size()) + ")";
        }
        rule_index--; // Convert to zero-based index
    } catch (...) {
        // Not a number, try to find by partial match
        rule_index = -1;
        for (size_t i = 0; i < governance_rules.size(); i++) {
            if (governance_rules[i].find(rule_id) != std::string::npos) {
                rule_index = i;
                break;
            }
        }
        
        if (rule_index == -1) {
            return "Error: Rule not found with ID: " + rule_id;
        }
    }
    
    // Increment violation count
    rule_violation_counts[std::to_string(rule_index + 1)]++;
    
    log_debug("Governance violation logged for rule " + std::to_string(rule_index + 1) + 
              ": " + governance_rules[rule_index]);
    
    return "Violation of rule " + std::to_string(rule_index + 1) + 
           " has been logged: " + governance_rules[rule_index];
}

std::string GovernanceHook::reaffirm_purpose() {
    // Apply RI-01: "Purpose must be reaffirmed each cycle"
    log_debug("Purpose reaffirmation for cycle " + std::to_string(current_cycle));
    
    return "System purpose has been reaffirmed for cycle " + std::to_string(current_cycle) + 
           ":\n\n\"Maintain cognitive coherence through persistent contradiction management, " +
           "recursive self-improvement, and multi-perspective integration while ensuring " +
           "governance stability, ethical alignment, sustainable evolution, and contextual awareness.\"";
}

std::string GovernanceHook::list_rules() {
    std::stringstream result;
    result << "## Active Governance Rules\n\n";
    
    for (size_t i = 0; i < governance_rules.size(); i++) {
        result << (i + 1) << ". " << governance_rules[i] << "\n";
    }
    
    return result.str();
}

std::string GovernanceHook::invoke_rule(const std::string& rule_id) {
    if (rule_id.empty()) {
        return "Error: No rule specified for invocation";
    }
    
    // Check if rule_id is a number
    int rule_index = -1;
    try {
        rule_index = std::stoi(rule_id);
        
        if (rule_index < 1 || rule_index > static_cast<int>(governance_rules.size())) {
            return "Error: Rule index out of range (valid range: 1-" + 
                   std::to_string(governance_rules.size()) + ")";
        }
        rule_index--; // Convert to zero-based index
    } catch (...) {
        // Not a number, try to find by partial match
        rule_index = -1;
        for (size_t i = 0; i < governance_rules.size(); i++) {
            if (governance_rules[i].find(rule_id) != std::string::npos) {
                rule_index = i;
                break;
            }
        }
        
        if (rule_index == -1) {
            return "Error: Rule not found with ID: " + rule_id;
        }
    }
    
    // Increment invocation count
    rule_invocation_counts[std::to_string(rule_index + 1)]++;
    
    log_debug("Governance rule " + std::to_string(rule_index + 1) + 
              " invoked: " + governance_rules[rule_index]);
    
    return "Rule " + std::to_string(rule_index + 1) + 
           " has been invoked:\n\n" + governance_rules[rule_index];
}
