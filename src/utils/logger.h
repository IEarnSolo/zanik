#pragma once

#include <cstdio>
#include <string>

inline void LogMessage(const std::string& message) {
    std::fprintf(stderr, "[DWStatus] %s\n", message.c_str());
    if(FILE* file=std::fopen("/home/container/zanik-data/runtime.log","a")){
        std::fprintf(file,"[DWStatus] %s\n",message.c_str());
        std::fclose(file);
    }
}

inline void LogVerbose(const std::string&) {}

inline std::string HexString(unsigned long long value) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "0x%llX", value);
    return buffer;
}
