#pragma once

#include "engine/dom_engine.h"
#include <cstdint>
#include <map>
#include <string>
#include <vector>

struct StoredRecipeSnapshot {
    std::string characterGuid;
    std::string catalogFingerprint;
    std::vector<std::string> unlockedIds;
    std::int64_t capturedAt = 0;
    bool complete = false;
};

class RecipeSnapshotStore {
public:
    explicit RecipeSnapshotStore(std::string path);
    bool Load();
    bool Save() const;
    bool Sync(const std::vector<DomEngine::PlayerRecipeSnapshot>& snapshots);
    bool Find(const std::string& characterGuid, StoredRecipeSnapshot& snapshot) const;
    std::size_t Count() const { return snapshots_.size(); }
    const std::string& Path() const { return path_; }
private:
    std::string path_;
    std::map<std::string, StoredRecipeSnapshot> snapshots_;
};
