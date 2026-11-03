#pragma once

#include <cstdlib>
#include <string>

namespace RuntimePaths {

inline std::string Environment(const char* name, const std::string& fallback) {
    const char* value = std::getenv(name);
    return value && *value ? value : fallback;
}

inline std::string ServerRoot() {
    return Environment("DW_SERVER_ROOT", "/home/container");
}

inline std::string ZanikRoot() {
    return Environment("DW_ZANIK_ROOT", ServerRoot() + "/zanik");
}

inline std::string DataRoot() {
    return Environment("DW_DATA_ROOT", ServerRoot() + "/zanik-data");
}

inline std::string WebRoot() {
    return Environment("DW_WEB_ROOT", ZanikRoot() + "/assets/current");
}

inline std::string GameSavedRoot() {
    return ServerRoot() + "/RSDragonwilds/Saved";
}

inline std::string DataFile(const char* name) {
    return DataRoot() + "/" + name;
}

inline std::string WebFile(const char* name) {
    return WebRoot() + "/" + name;
}

} // namespace RuntimePaths
