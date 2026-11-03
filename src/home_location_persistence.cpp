#include "home_location_persistence.h"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <fcntl.h>
#include <iomanip>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace HomeLocationPersistence {

bool ValidGuid(const std::string& guid) {
    return !guid.empty() && guid.size() <= 128 &&
           std::none_of(guid.begin(), guid.end(), [](unsigned char value) {
               return value < 0x20 || value == 0x7f;
           });
}

LoadResult Parse(std::istream& input) {
    LoadResult result;
    std::string line;
    while (std::getline(input, line)) {
        std::istringstream row(line);
        char kind = 0;
        std::string guid;
        Record record;
        if (!(row >> kind >> std::quoted(guid) >> record.x >> record.y >> record.z >> record.capturedAt) ||
            kind != 'H' || !ValidGuid(guid) || !std::isfinite(record.x) || !std::isfinite(record.y) ||
            !std::isfinite(record.z) || record.capturedAt <= 0) {
            ++result.malformedRecords;
            continue;
        }
        row >> std::ws;
        if (!row.eof() || !result.records.emplace(guid, record).second) {
            ++result.malformedRecords;
            continue;
        }
    }
    return result;
}

bool SaveAtomic(const std::string& path, const std::unordered_map<std::string, Record>& records) {
    std::ostringstream output;
    output << std::setprecision(17);
    for (const auto& entry : records) {
        if (!ValidGuid(entry.first) || !std::isfinite(entry.second.x) || !std::isfinite(entry.second.y) ||
            !std::isfinite(entry.second.z) || entry.second.capturedAt <= 0) return false;
        output << 'H' << ' ' << std::quoted(entry.first) << ' ' << entry.second.x << ' ' << entry.second.y << ' '
               << entry.second.z << ' ' << entry.second.capturedAt << '\n';
    }
    const std::string bytes = output.str(), temporary = path + ".tmp";
    int descriptor = open(temporary.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    bool okay = descriptor >= 0;
    std::size_t offset = 0;
    while (okay && offset < bytes.size()) {
        const ssize_t count = write(descriptor, bytes.data() + offset, bytes.size() - offset);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) { okay = false; break; }
        offset += static_cast<std::size_t>(count);
    }
    if (descriptor >= 0) {
        if (okay) okay = fchmod(descriptor, 0600) == 0 && fsync(descriptor) == 0;
        if (close(descriptor) != 0) okay = false;
    }
    if (okay) okay = rename(temporary.c_str(), path.c_str()) == 0 && chmod(path.c_str(), 0600) == 0;
    if (!okay) unlink(temporary.c_str());
    return okay;
}

}  // namespace HomeLocationPersistence
