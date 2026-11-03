#include "../src/home_location_persistence.h"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

int main() {
    std::istringstream mixed(
        "H \"valid-guid\" 1 2 3 100\n"
        "H \"duplicate\" 4 5 6 101\n"
        "H \"duplicate\" 7 8 9 102\n"
        "H \"bad-guid\n"
        "H \"nan-location\" nan 2 3 103\n");
    auto parsed = HomeLocationPersistence::Parse(mixed);
    assert(parsed.records.size() == 2);
    assert(parsed.malformedRecords == 3);

    const std::string path = "dist/home-location-persistence-test.db";
    std::remove(path.c_str());
    assert(HomeLocationPersistence::SaveAtomic(path, parsed.records));
    struct stat details{};
    assert(stat(path.c_str(), &details) == 0);
    assert((details.st_mode & 0777) == 0600);
    std::ifstream saved(path);
    auto loaded = HomeLocationPersistence::Parse(saved);
    assert(loaded.records.size() == 2 && loaded.malformedRecords == 0);
    assert(!HomeLocationPersistence::SaveAtomic("dist/missing-parent/home.db", parsed.records));
    std::remove(path.c_str());
}
