// governance_registry.cpp
#include "governance_registry.h"
#include <sstream>
#include <iostream>

// Initialize static members
GovernanceRegistry* GovernanceRegistry::instance = nullptr;
std::mutex GovernanceRegistry::instance_mutex;

// String representation of a rule
std::string GovernanceRule::to_string() const {
    std::stringstream ss;
    ss << "Rule " << id << ": " << name << " (" << category << ")\n";
    ss << "  " << description;
    return ss.str();
}

// Singleton access
GovernanceRegistry& GovernanceRegistry::get_instance() {
    if (instance == nullptr) {
        std::lock_guard<std::mutex> lock(instance_mutex);
        if (instance == nullptr) {
            instance = new GovernanceRegistry();
        }
    }
    return *instance;
}

// Rule management
void GovernanceRegistry::register_rule(std::shared_ptr<GovernanceRule> rule) {
    if (!rule) return;
    
    rules_by_id[rule->id] = rule;
    rules_by_category[rule->category].push_back(rule);
}

void GovernanceRegistry::unregister_rule(int rule_id) {
    auto it = rules_by_id.find(rule_id);
    if (it == rules_by_id.end()) return;
    
    std::shared_ptr<GovernanceRule> rule = it->second;
    
    // Remove from rules_by_id
    rules_by_id.erase(it);
    
    // Remove from rules_by_category
    auto& category_rules = rules_by_category[rule->category];
    category_rules.erase(
        std::remove_if(category_rules.begin(), category_rules.end(),
                      [rule_id](const std::shared_ptr<GovernanceRule>& r) { return r->id == rule_id; }),
        category_rules.end()
    );
    
    // Remove empty categories
    if (category_rules.empty()) {
        rules_by_category.erase(rule->category);
    }
}

void GovernanceRegistry::clear_rules() {
    rules_by_id.clear();
    rules_by_category.clear();
}

// Rule access
std::shared_ptr<GovernanceRule> GovernanceRegistry::get_rule(int rule_id) const {
    auto it = rules_by_id.find(rule_id);
    if (it != rules_by_id.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::shared_ptr<GovernanceRule>> GovernanceRegistry::get_rules_by_category(const std::string& category) const {
    auto it = rules_by_category.find(category);
    if (it != rules_by_category.end()) {
        return it->second;
    }
    return {};
}

std::vector<std::shared_ptr<GovernanceRule>> GovernanceRegistry::get_all_rules() const {
    std::vector<std::shared_ptr<GovernanceRule>> all_rules;
    all_rules.reserve(rules_by_id.size());
    
    for (const auto& [id, rule] : rules_by_id) {
        all_rules.push_back(rule);
    }
    
    // Sort by rule ID for consistent ordering
    std::sort(all_rules.begin(), all_rules.end(), 
              [](const std::shared_ptr<GovernanceRule>& a, const std::shared_ptr<GovernanceRule>& b) {
                  return a->id < b->id;
              });
    
    return all_rules;
}

size_t GovernanceRegistry::rule_count() const {
    return rules_by_id.size();
}

// Rule evaluation
std::optional<std::string> GovernanceRegistry::evaluate_rules(const std::string& input, const std::string& category) const {
    std::vector<std::shared_ptr<GovernanceRule>> rules_to_check;
    
    if (category.empty()) {
        // Check all rules
        rules_to_check = get_all_rules();
    } else {
        // Check only rules in the specified category
        rules_to_check = get_rules_by_category(category);
    }
    
    // Apply each rule and return the first rule violation
    for (const auto& rule : rules_to_check) {
        if (rule->finalize_response) {  // Check if finalize_response is not null
            auto result = rule->finalize_response(input);
            if (result.has_value()) {
                return result;
            }
        }
    }
    
    // No rule violations found
    return std::nullopt;
}

// Rule status reporting
std::string GovernanceRegistry::get_rules_status() const {
    std::stringstream ss;
    ss << "## Governance Rules Status\n\n";
    
    std::vector<std::string> categories;
    for (const auto& [category, _] : rules_by_category) {
        categories.push_back(category);
    }
    
    // Sort categories for consistent output
    std::sort(categories.begin(), categories.end());
    
    // Group rules by category
    for (const auto& category : categories) {
        auto rules = get_rules_by_category(category);
        
        ss << "### Category: " << category << "\n\n";
        
        // Sort rules by ID
        std::sort(rules.begin(), rules.end(), 
                 [](const std::shared_ptr<GovernanceRule>& a, const std::shared_ptr<GovernanceRule>& b) {
                     return a->id < b->id;
                 });
        
        for (const auto& rule : rules) {
            ss << "- **Rule " << rule->id << "**: " << rule->name << "\n";
            ss << "  " << rule->description << "\n\n";
        }
    }
    
    return ss.str();
}

// Serialization support
void GovernanceRegistry::to_json(json& j) const {
    // Create an array of rules
    json rules_array = json::array();
    
    for (const auto& [id, rule] : rules_by_id) {
        json rule_json = {
            {"id", rule->id},
            {"name", rule->name},
            {"description", rule->description},
            {"category", rule->category},
            {"has_finalize_response", rule->finalize_response ? true : false},
        };
        rules_array.push_back(rule_json);
    }
    
    j["rules"] = rules_array;
}

void GovernanceRegistry::from_json(const json& j) {
    // Clear existing rules
    clear_rules();
    
    // Load rules from JSON
    if (j.contains("rules") && j["rules"].is_array()) {
        for (const auto& rule_json : j["rules"]) {
            // Get basic rule info
            int rule_id = rule_json["id"];
            std::string name = rule_json["name"];
            std::string description = rule_json["description"];
            std::string category = rule_json["category"];
            
            // Just create a reference to an existing rule or a placeholder
            auto rule = std::make_shared<GovernanceRule>();
            rule->id = rule_id;
            rule->name = name;
            rule->description = description;
            rule->category = category;
            
            // Rules will be reinitialized with the correct functions
            // when GovernanceHook is created
            
            register_rule(rule);
        }
    }
}
