#pragma once

#include <pthread.h>
#include <string>
#include <unordered_set>
#include <vector>

struct RecipeSettingsDiagnostics {
    bool readable = true;
    bool writable = false;
    bool restricted = true;
    bool lastSaveSucceeded = true;
    std::size_t disabled = 0;
    std::size_t malformed = 0;
};

class RecipeSettingsStore {
public:
    explicit RecipeSettingsStore(std::string path);
    bool Load();
    bool SetEnabled(const std::string& id, bool enabled);
    bool SetManyEnabled(const std::vector<std::string>& ids, bool enabled);
    bool Enabled(const std::string& id);
    std::string Json();
    RecipeSettingsDiagnostics Diagnostics();
private:
    bool SaveLocked();
    std::string path_;
    std::unordered_set<std::string> disabled_;
    std::size_t malformed_ = 0;
    bool lastSaveSucceeded_ = true;
    pthread_mutex_t mutex_ = PTHREAD_MUTEX_INITIALIZER;
};
