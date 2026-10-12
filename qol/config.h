#pragma once

#include <cstdlib>
#include <cstring>

namespace zanik::qol {

// Every gameplay-changing QoL feature is opt-in. Missing and malformed values
// fail closed so a new installation cannot silently alter gameplay.
inline bool compost_auto_equip_enabled() {
    const char* value = std::getenv("ZANIK_FEATURE_QOL_AUTO_COMPOST");
    if (!value) return false;
    if (std::strcmp(value, "1") == 0 || std::strcmp(value, "true") == 0) return true;
    return false;
}

} // namespace zanik::qol
