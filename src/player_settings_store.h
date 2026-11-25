#pragma once

#include <pthread.h>
#include <string>
#include <unordered_set>

struct PlayerSettingsDiagnostics {
    bool loaded = false;
    bool readable = true;
    bool writable = false;
    bool restricted = true;
    bool lastSaveSucceeded = true;
    std::size_t freeBuildingCharacters = 0;
    std::size_t malformedRecords = 0;
};

class PlayerSettingsStore {
public:
    explicit PlayerSettingsStore(std::string path);
    bool Load();
    bool SetFreeBuilding(const std::string& characterGuid, bool enabled);
    bool FreeBuilding(const std::string& characterGuid) const;
    std::unordered_set<std::string> FreeBuildingCharacters() const;
    PlayerSettingsDiagnostics Diagnostics() const;

private:
    bool SaveLocked();
    std::string path_;
    std::unordered_set<std::string> freeBuilding_;
    bool loaded_ = false;
    bool lastSaveSucceeded_ = true;
    std::size_t malformedRecords_ = 0;
    mutable pthread_mutex_t mutex_ = PTHREAD_MUTEX_INITIALIZER;
};
