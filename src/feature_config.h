#pragma once

#include <cstdlib>
#include <cstring>
#include <string>

namespace ZanikConfig {

inline bool Flag(const char* name, bool fallback) {
    const char* value = std::getenv(name);
    if (!value || !*value) return fallback;
    if (std::strcmp(value, "1") == 0 || std::strcmp(value, "true") == 0) return true;
    if (std::strcmp(value, "0") == 0 || std::strcmp(value, "false") == 0) return false;
    return false;
}

inline bool PlayerStatus() { return Flag("ZANIK_FEATURE_PLAYER_STATUS", true); }
inline bool KillTracking() { return Flag("ZANIK_FEATURE_KILL_TRACKING", true); }
inline bool Map() { return Flag("ZANIK_FEATURE_MAP", true); }
inline bool ResourceTracking() { return Flag("ZANIK_FEATURE_RESOURCE_TRACKING", true); }
inline bool NpcTracking() { return Flag("ZANIK_FEATURE_NPC_TRACKING", true); }
inline bool PlayerRecipeTracking() { return Flag("ZANIK_FEATURE_PLAYER_RECIPE_TRACKING", true); }
inline bool PlayerQuestTracking() { return Flag("ZANIK_FEATURE_PLAYER_QUEST_TRACKING", true); }
inline bool AdminRead() { return Flag("ZANIK_FEATURE_ADMIN_READ", true); }
inline bool AdminRecipeSettings() { return Flag("ZANIK_FEATURE_ADMIN_RECIPE_SETTINGS", false); }
inline bool AdminTeleport() { return Flag("ZANIK_FEATURE_ADMIN_TELEPORT", false); }
inline bool AdminStorageMutation() { return Flag("ZANIK_FEATURE_ADMIN_STORAGE_MUTATION", false); }
inline bool AdminGravestoneMutation() { return Flag("ZANIK_FEATURE_ADMIN_GRAVESTONE_MUTATION", false); }
inline bool QolAutoCompost() { return Flag("ZANIK_FEATURE_QOL_AUTO_COMPOST", false); }

inline const char* State(bool enabled, int compatibility) {
    if (!enabled) return "disabled";
    return compatibility == 1 ? "compatible" : "incompatible";
}

inline std::string Json() {
    auto boolean=[](bool value){ return value ? "true" : "false"; };
    return std::string("{")+
        "\"playerStatus\":"+boolean(PlayerStatus())+
        ",\"killTracking\":"+boolean(KillTracking())+
        ",\"map\":"+boolean(Map())+
        ",\"resourceTracking\":"+boolean(ResourceTracking())+
        ",\"npcTracking\":"+boolean(NpcTracking())+
        ",\"playerRecipeTracking\":"+boolean(PlayerRecipeTracking())+
        ",\"playerQuestTracking\":"+boolean(PlayerQuestTracking())+
        ",\"adminRead\":"+boolean(AdminRead())+
        ",\"adminRecipeSettings\":"+boolean(AdminRecipeSettings())+
        ",\"adminTeleport\":"+boolean(AdminTeleport())+
        ",\"adminStorageMutation\":"+boolean(AdminStorageMutation())+
        ",\"adminGravestoneMutation\":"+boolean(AdminGravestoneMutation())+
        ",\"qolAutoCompost\":"+boolean(QolAutoCompost())+"}";
}

inline std::string StateJson() {
    auto state=[](bool value){ return value ? "\"enabled\"" : "\"disabled\""; };
    return std::string("{")+
        "\"playerStatus\":"+state(PlayerStatus())+
        ",\"killTracking\":"+state(KillTracking())+
        ",\"map\":"+state(Map())+
        ",\"resourceTracking\":"+state(ResourceTracking())+
        ",\"npcTracking\":"+state(NpcTracking())+
        ",\"playerRecipeTracking\":"+state(PlayerRecipeTracking())+
        ",\"playerQuestTracking\":"+state(PlayerQuestTracking())+
        ",\"adminRead\":"+state(AdminRead())+
        ",\"adminRecipeSettings\":"+state(AdminRecipeSettings())+
        ",\"adminTeleport\":"+state(AdminTeleport())+
        ",\"adminStorageMutation\":"+state(AdminStorageMutation())+
        ",\"adminGravestoneMutation\":"+state(AdminGravestoneMutation())+
        ",\"qolAutoCompost\":"+state(QolAutoCompost())+"}";
}

} // namespace ZanikConfig
