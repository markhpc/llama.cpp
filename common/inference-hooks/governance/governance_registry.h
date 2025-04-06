// governance_registry.h
#pragma once

#include "json.hpp"

#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include <optional>
#include <mutex>
#include <functional>

// Forward declaration for nlohmann::json
using json = nlohmann::ordered_json;

// Rule structure definition
struct GovernanceRule {
    int id;
    std::string name;
    std::string description;
    std::string category;
    std::function<std::optional<std::string>(const std::string&)> finalize_response;
    std::function<std::optional<std::string>(const std::string&)> streaming_check;

    // For debugging and logging
    std::string to_string() const;
};

// Singleton registry for governance rules
class GovernanceRegistry {
private:
    // Singleton instance
    static GovernanceRegistry* instance;
    static std::mutex instance_mutex;
    
    // Rule storage
    std::unordered_map<int, std::shared_ptr<GovernanceRule>> rules_by_id;
    std::unordered_map<std::string, std::vector<std::shared_ptr<GovernanceRule>>> rules_by_category;
    
    // Private constructor for singleton
    GovernanceRegistry() = default;
    
public:
    // Prevent copy/move operations
    GovernanceRegistry(const GovernanceRegistry&) = delete;
    GovernanceRegistry& operator=(const GovernanceRegistry&) = delete;
    GovernanceRegistry(GovernanceRegistry&&) = delete;
    GovernanceRegistry& operator=(GovernanceRegistry&&) = delete;
    
    // Singleton access
    static GovernanceRegistry& get_instance();
    
    // Rule management
    void register_rule(std::shared_ptr<GovernanceRule> rule);
    void unregister_rule(int rule_id);
    void clear_rules();
    
    // Rule access
    std::shared_ptr<GovernanceRule> get_rule(int rule_id) const;
    std::vector<std::shared_ptr<GovernanceRule>> get_rules_by_category(const std::string& category) const;
    std::vector<std::shared_ptr<GovernanceRule>> get_all_rules() const;
    size_t rule_count() const;
    
    // Rule evaluation
    std::optional<std::string> evaluate_rules(const std::string& input, const std::string& category = "") const;
    
    // Rule status reporting
    std::string get_rules_status() const;
    
    // Serialization support
    void to_json(json& j) const;
    void from_json(const json& j,
                   const std::function<std::function<std::optional<std::string>(const std::string&)>(int, bool)>& rule_factory);
};
