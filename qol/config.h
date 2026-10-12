#pragma once

#include <cstdlib>
#include <cstring>

namespace zanik::qol {

// Each QoL feature owns its own flag. Missing means enabled. Explicit 0 or
// false disables it; malformed values fail closed.
inline bool compost_auto_equip_enabled() {
    const char* value = std::getenv("ZANIK_QOL_AUTO_COMPOST");
    if (!value) return true;
    if (std::strcmp(value, "1") == 0 || std::strcmp(value, "true") == 0) return true;
    return false;
}

} // namespace zanik::qol
