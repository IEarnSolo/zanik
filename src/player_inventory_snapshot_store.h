#pragma once

#include <cstdint>
#include <pthread.h>
#include <string>
#include <vector>

struct PlayerInventorySnapshotItem {
    std::string id;
    std::string key;
    std::string name;
    std::string icon;
    int slot = -1;
    int quantity = 0;
    int maxStack = 1;
    bool equippable = false;
    bool canUnequip = true;
};

struct PlayerInventorySnapshotContainer {
    std::string id;
    int totalSlots = 0;
    int hotbarStart = -1;
    int hotbarCount = 0;
    std::vector<PlayerInventorySnapshotItem> items;
};

struct PlayerInventoryDisplaySnapshot {
    std::string characterGuid;
    std::int64_t capturedAt = 0;
    std::vector<PlayerInventorySnapshotContainer> containers;
};

struct PlayerInventorySnapshotDiagnostics {
    bool loaded = false;
    bool readable = true;
    bool writable = false;
    bool restricted = true;
    bool lastSaveSucceeded = true;
    std::size_t snapshots = 0;
    std::size_t malformedRecords = 0;
};

class PlayerInventorySnapshotStore {
public:
    explicit PlayerInventorySnapshotStore(std::string path);
    bool Load();
    bool Replace(const PlayerInventoryDisplaySnapshot& value);
    bool Find(const std::string& characterGuid, PlayerInventoryDisplaySnapshot& value) const;
    PlayerInventorySnapshotDiagnostics Diagnostics() const;

private:
    bool SaveLocked();
    std::string path_;
    std::vector<PlayerInventoryDisplaySnapshot> snapshots_;
    bool loaded_ = false;
    bool lastSaveSucceeded_ = true;
    std::size_t malformedRecords_ = 0;
    mutable pthread_mutex_t mutex_ = PTHREAD_MUTEX_INITIALIZER;
};
