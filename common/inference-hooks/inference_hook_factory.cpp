// inference_hook_factory.cpp
#include "inference_hook.h"
#include "inference-hooks/inference_hook_composite.h"
#include "inference-hooks/governance/governance_hook.h"
#include <unordered_map>
#include <memory>

namespace {
    std::unordered_map<std::string, std::unique_ptr<InferenceHook>> hooks;
}

InferenceHook& get_or_create_inference_hook(const std::string& id) {
    auto it = hooks.find(id);
    
    if (it == hooks.end()) {
        // Create a new composite hook
        auto composite = std::make_unique<InferenceHookComposite>();
        
        // Add the governance hook to the composite
        composite->add_hook(std::make_shared<GovernanceHook>());
        
        // Insert into the map and get iterator
        it = hooks.emplace(id, std::move(composite)).first;
    }
    
    return *it->second;
}
