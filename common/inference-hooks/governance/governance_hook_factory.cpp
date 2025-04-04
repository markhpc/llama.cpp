// governance_hook_factory.cpp
#include "inference-hooks/inference_hook_factory.h"
#include "inference-hooks/inference_hook_composite.h"
#include "governance_hook.h"

#include <iostream>

// Custom factory implementation to add our hook
class GovernanceHookFactory {
public:
    static void add_governance_hook_to_composite(const std::string& id) {
        // Get or create the composite hook for this ID
        auto& hook = get_or_create_inference_hook(id);
        
        // If this is a composite hook, add our governance hook to it
        auto* composite = dynamic_cast<InferenceHookComposite*>(&hook);
        if (composite) {
            composite->add_hook(std::make_shared<GovernanceHook>());
        } else {
            // If it's not a composite hook, we can't add to it
            std::cerr << "Warning: Cannot add governance hook to non-composite hook with ID: " << id << std::endl;
        }
    }
};
