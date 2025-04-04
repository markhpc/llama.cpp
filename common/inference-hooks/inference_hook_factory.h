#pragma once

#include "inference_hook.h"
#include <string>

InferenceHook& get_or_create_inference_hook(const std::string& id);
