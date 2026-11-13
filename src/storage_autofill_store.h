#pragma once

#include <cstdint>
#include <pthread.h>
#include <string>
#include <vector>

struct StorageAutofillItem {
    int slot = -1;
    int quantity = 0;
    int maxStack = 1;
    std::string key;
};

struct StorageAutofillTemplate {
    std::string ownerGuid;
    std::string chestId;
    std::int64_t capturedAt = 0;
    int totalSlots = 0;
    std::vector<StorageAutofillItem> items;
};

struct StorageAutofillDiagnostics {
    bool loaded = false;
    bool readable = true;
    bool writable = false;
    bool restricted = true;
    bool lastSaveSucceeded = true;
    std::size_t templates = 0;
    std::size_t malformedRecords = 0;
};

enum class StorageAutofillPlanKind {
    None,
    Add,
    Move,
    Set,
};

struct StorageAutofillPlan {
    StorageAutofillPlanKind kind = StorageAutofillPlanKind::None;
    std::string key;
    int quantity = 0;
    int sourceSlot = -1;
    int destinationSlot = -1;
};

struct StorageAutofillRelocation {
    bool available = false;
    int sourceSlot = -1;
    int destinationSlot = -1;
    int quantity = 0;
};

StorageAutofillPlan PlanStorageAutofill(const StorageAutofillTemplate& desired,
                                        const std::vector<StorageAutofillItem>& current);
StorageAutofillRelocation PlanStorageAutofillRelocation(
    const std::vector<StorageAutofillItem>& desired,
    const std::vector<StorageAutofillItem>& current,
    const std::string& key);
bool ApplyStorageAutofillSelection(StorageAutofillTemplate& desired,
                                   const std::vector<StorageAutofillItem>& selected,
                                   bool enabled);

class StorageAutofillStore {
public:
    explicit StorageAutofillStore(std::string path);
    bool Load();
    bool Replace(const StorageAutofillTemplate& value);
    bool Remove(const std::string& ownerGuid, const std::string& chestId);
    bool Find(const std::string& ownerGuid, const std::string& chestId,
              StorageAutofillTemplate& value) const;
    std::vector<StorageAutofillTemplate> All() const;
    StorageAutofillDiagnostics Diagnostics() const;

private:
    bool SaveLocked();
    std::string path_;
    std::vector<StorageAutofillTemplate> templates_;
    bool loaded_ = false;
    bool lastSaveSucceeded_ = true;
    std::size_t malformedRecords_ = 0;
    mutable pthread_mutex_t mutex_ = PTHREAD_MUTEX_INITIALIZER;
};
