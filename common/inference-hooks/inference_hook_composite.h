#pragma once

#include "inference_hook.h"
#include <iostream>
#include <vector>
#include <memory>

using json = nlohmann::ordered_json;

class InferenceHookComposite : public InferenceHook {
public:
    void add_hook(std::shared_ptr<InferenceHook> hook);

    std::string get_id() const override;
    void on_cycle_start(const llama_context& ctx);
    std::string format_injection_prompt() const;
    void process_response(json& response, bool is_final, const WriteCallback& write_callback);
    void handle_json_command(json &j);
    std::string handle_text_command(const std::string& cmd);
    std::string finalize_response(const std::string& response_text) override;
    StreamingCheckResult check_streaming_content(const std::string& current_content) override;
    std::string get_feedback() const override;
    bool has_feedback() const override;

private:
    std::vector<std::shared_ptr<InferenceHook>> hooks;
};

