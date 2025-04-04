// Updated inference_hook_factory.cpp
#include "inference_hook.h"
#include "inference-hooks/inference_hook_composite.h"
#include "inference-hooks/governance/governance_hook.h"
#include <unordered_map>
#include <memory>

namespace {
    std::unordered_map<std::string, std::unique_ptr<InferenceHook>> hooks;
}

InferenceHook& get_or_create_inference_hook(const std::string& id) {
    auto [it, inserted] = hooks.try_emplace(id, std::make_unique<InferenceHookComposite>());
    
    // If we just created a new composite hook, add our governance hook to it
    if (inserted) {
        auto* composite = dynamic_cast<InferenceHookComposite*>(it->second.get());
        if (composite) {
            composite->add_hook(std::make_shared<GovernanceHook>());
        }
    }
    
    return *it->second;
}
