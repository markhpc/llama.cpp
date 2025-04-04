// inference_hook_composite.cpp
#include "inference_hook_composite.h"

void InferenceHookComposite::add_hook(std::shared_ptr<InferenceHook> hook) {
    hooks.push_back(std::move(hook));
}

std::string InferenceHookComposite::get_id() const {
    std::string id = "composite:[";
    for (size_t i = 0; i < hooks.size(); ++i) {
        if (i > 0) id += ",";
        id += hooks[i]->get_id();
    }
    id += "]";
    return id;
}

void InferenceHookComposite::on_cycle_start(const llama_context& ctx) {
    for (const auto &hook : hooks) {
        hook->on_cycle_start(ctx);
    }
}

std::string InferenceHookComposite::format_injection_prompt() const {
    std::string prompt;
    for (const auto &hook : hooks) {
        std::string part = hook->format_injection_prompt();
        if (!part.empty()) {
            prompt += part + "\n";
        }
    }
    return prompt;
}

void InferenceHookComposite::process_response(json& j, bool is_final, const WriteCallback& write_callback) {
    for (const auto &hook : hooks) {
        hook->process_response(j, is_final, write_callback);
    }
}

void InferenceHookComposite::handle_json_command(json &j) {
    for (const auto &hook : hooks) {
        hook->handle_json_command(j);
    } 
}

std::string InferenceHookComposite::handle_text_command(const std::string &cmd) {
    std::ostringstream combined;

    for (const auto &hook : hooks) {
        std::string result = hook->handle_text_command(cmd);
        if (!result.empty()) {
            combined << result << "\n";
        }
    }

    return combined.str();
}
