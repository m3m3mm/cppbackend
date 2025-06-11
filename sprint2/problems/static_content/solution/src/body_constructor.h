#pragma once

#include <string>

#include "target_storage.h"
#include "model.h"

namespace body {
    std::string MakeBodyJSON(targets_storage::TargetRequestType req_type,
                             const  model::Game& game = model::Game(),
                             const std::string& req_object = "");
}