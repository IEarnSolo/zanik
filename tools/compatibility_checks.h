#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace CompatibilityChecks {

inline bool ExactBytesAt(const std::vector<unsigned char>& binary, std::size_t offset,
                         const unsigned char* expected, std::size_t expectedSize) {
    return expected && offset <= binary.size() && expectedSize <= binary.size() - offset &&
           std::memcmp(binary.data() + offset, expected, expectedSize) == 0;
}

inline bool ContainsBytes(const std::vector<unsigned char>& binary,
                          const std::vector<unsigned char>& expected) {
    return !expected.empty() &&
           std::search(binary.begin(), binary.end(), expected.begin(), expected.end()) != binary.end();
}

inline bool ContainsContractText(const std::vector<unsigned char>& binary, const char* text) {
    if (!text || !*text) return false;
    const auto length = std::strlen(text);
    const std::vector<unsigned char> narrow(text, text + length);
    if (ContainsBytes(binary, narrow)) return true;
    std::vector<unsigned char> wide;
    wide.reserve(length * 2);
    for (const unsigned char* at = reinterpret_cast<const unsigned char*>(text); *at; ++at) {
        wide.push_back(*at);
        wide.push_back(0);
    }
    return ContainsBytes(binary, wide);
}

}  // namespace CompatibilityChecks
