#pragma once 

#include "util/tagged_uuid.h"

namespace domain {
namespace detail {
struct PlayerTag {};
}  // namespace detail
    
using PlayerId = util::TaggedUUID<detail::PlayerTag>;
}//namespace domain