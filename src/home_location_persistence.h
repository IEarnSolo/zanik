#pragma once

#include <cstdint>
#include <istream>
#include <string>
#include <unordered_map>

namespace HomeLocationPersistence {

struct Record {
    double x = 0, y = 0, z = 0;
    std::int64_t capturedAt = 0;
};

struct LoadResult {
    std::unordered_map<std::string, Record> records;
    std::uint64_t malformedRecords = 0;
};

bool ValidGuid(const std::string& guid);
LoadResult Parse(std::istream& input);
bool SaveAtomic(const std::string& path, const std::unordered_map<std::string, Record>& records);

}  // namespace HomeLocationPersistence
