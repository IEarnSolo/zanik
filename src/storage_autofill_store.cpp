#include "storage_autofill_store.h"

#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace {
constexpr std::size_t MaximumTemplates = 4096;
constexpr std::size_t MaximumItems = 512;
struct Lock { pthread_mutex_t* value; explicit Lock(pthread_mutex_t& mutex):value(&mutex){pthread_mutex_lock(value);} ~Lock(){pthread_mutex_unlock(value);} };

std::vector<std::string> Fields(const std::string& line) {
    std::vector<std::string> result;
    std::size_t start = 0;
    for (;;) {
        const auto at = line.find('\t', start);
        result.push_back(line.substr(start, at == std::string::npos ? std::string::npos : at - start));
        if (at == std::string::npos) return result;
        start = at + 1;
    }
}

bool ParseInteger(const std::string& text, long long minimum, long long maximum, long long& value) {
    if (text.empty() || text.size() > 20) return false;
    char* end = nullptr;
    errno = 0;
    const long long parsed = std::strtoll(text.c_str(), &end, 10);
    if (errno || !end || *end || parsed < minimum || parsed > maximum) return false;
    value = parsed;
    return true;
}

char HexDigit(unsigned value) { return value < 10 ? char('0' + value) : char('a' + value - 10); }
std::string HexEncode(const std::string& value) {
    std::string result;
    result.reserve(value.size() * 2);
    for (unsigned char byte : value) { result.push_back(HexDigit(byte >> 4)); result.push_back(HexDigit(byte & 15)); }
    return result;
}
int HexValue(char value) {
    if (value >= '0' && value <= '9') return value - '0';
    if (value >= 'a' && value <= 'f') return value - 'a' + 10;
    if (value >= 'A' && value <= 'F') return value - 'A' + 10;
    return -1;
}
bool HexDecode(const std::string& encoded, std::string& value) {
    if (encoded.empty() || encoded.size() % 2 || encoded.size() > 512) return false;
    value.clear(); value.reserve(encoded.size() / 2);
    for (std::size_t i = 0; i < encoded.size(); i += 2) {
        const int high = HexValue(encoded[i]), low = HexValue(encoded[i + 1]);
        if (high < 0 || low < 0) return false;
        const char byte = char((high << 4) | low);
        if (static_cast<unsigned char>(byte) < 32 || byte == '\t') return false;
        value.push_back(byte);
    }
    return true;
}
bool ValidChestId(const std::string& value) {
    return value.size() == 16 && std::all_of(value.begin(), value.end(), [](char c){ return HexValue(c) >= 0; });
}
bool ValidTemplate(const StorageAutofillTemplate& value) {
    if (value.ownerGuid.empty() || value.ownerGuid.size() > 128 || !ValidChestId(value.chestId) ||
        value.capturedAt < 0 || value.totalSlots < 0 || value.totalSlots > int(MaximumItems) ||
        value.items.size() > MaximumItems) return false;
    std::vector<bool> slots(std::size_t(value.totalSlots), false);
    for (const auto& item : value.items) {
        if (item.slot < 0 || item.slot >= value.totalSlots || item.quantity < 1 || item.quantity > 1000000 ||
            item.maxStack < 1 || item.maxStack > 1000000 || item.key.empty() || item.key.size() > 256 ||
            slots[std::size_t(item.slot)]) return false;
        if (std::any_of(item.key.begin(), item.key.end(), [](unsigned char c){ return c < 32 || c == '\t'; })) return false;
        slots[std::size_t(item.slot)] = true;
    }
    return true;
}
std::string Parent(const std::string& path) {
    const auto slash = path.find_last_of('/');
    return slash == std::string::npos ? "." : (slash ? path.substr(0, slash) : "/");
}
bool WriteAll(int descriptor, const std::string& value) {
    std::size_t offset = 0;
    while (offset < value.size()) {
        const ssize_t written = write(descriptor, value.data() + offset, value.size() - offset);
        if (written <= 0) return false;
        offset += std::size_t(written);
    }
    return true;
}
}

StorageAutofillPlan PlanStorageAutofill(const StorageAutofillTemplate& desired,
                                        const std::vector<StorageAutofillItem>& current) {
    if (!ValidTemplate(desired)) return {};
    std::vector<const StorageAutofillItem*> currentBySlot(std::size_t(desired.totalSlots), nullptr);
    std::vector<bool> protectedSlots(std::size_t(desired.totalSlots), false);
    for (const auto& item : current) {
        if (item.slot < 0 || item.slot >= desired.totalSlots) continue;
        const auto slot = std::size_t(item.slot);
        if (item.quantity < 1 || item.maxStack < 1 || item.quantity > item.maxStack || item.key.empty() ||
            currentBySlot[slot]) { protectedSlots[slot] = true; currentBySlot[slot] = nullptr; continue; }
        if (!protectedSlots[slot]) currentBySlot[slot] = &item;
    }
    struct Difference { const StorageAutofillItem* desired = nullptr; int quantity = 0; };
    std::vector<Difference> deficits, surpluses;
    for (const auto& saved : desired.items) {
        if (protectedSlots[std::size_t(saved.slot)]) continue;
        const auto* actual = currentBySlot[std::size_t(saved.slot)];
        if (actual && actual->key != saved.key) continue;
        const int quantity = actual ? std::max(0, actual->quantity) : 0;
        if (quantity < saved.quantity) deficits.push_back({&saved, saved.quantity - quantity});
        else if (quantity > saved.quantity) surpluses.push_back({&saved, quantity - saved.quantity});
    }
    for (const auto& deficit : deficits) {
        for (const auto& surplus : surpluses) {
            if (deficit.desired->key != surplus.desired->key) continue;
            const int quantity = std::min(deficit.quantity, surplus.quantity);
            if (quantity > 0) return {StorageAutofillPlanKind::Move, deficit.desired->key,
                                     quantity, surplus.desired->slot, deficit.desired->slot};
        }
    }
    for (const auto& surplus : surpluses)
        return {StorageAutofillPlanKind::Set, surplus.desired->key,
                surplus.desired->quantity, surplus.desired->slot, -1};
    for (const auto& deficit : deficits) {
        long long quantity = 0;
        for (const auto& candidate : deficits)
            if (candidate.desired->key == deficit.desired->key) quantity += candidate.quantity;
        if (quantity > 0)
            return {StorageAutofillPlanKind::Add, deficit.desired->key,
                    int(std::min(quantity, 1000000LL)), -1, -1};
    }
    return {};
}

StorageAutofillStore::StorageAutofillStore(std::string path) : path_(std::move(path)) {}

bool StorageAutofillStore::Load() {
    Lock lock(mutex_);
    std::ifstream input(path_);
    if (!input) { templates_.clear(); loaded_ = true; malformedRecords_ = 0; return true; }
    std::string line;
    if (!std::getline(input, line) || line != "# zanik-storage-autofill\t1") {
        templates_.clear(); loaded_ = false; malformedRecords_ = 1; return false;
    }
    std::vector<StorageAutofillTemplate> parsed;
    std::size_t malformed = 0;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const auto fields = Fields(line);
        if (fields.size() != 6 || fields[0] != "T" || parsed.size() >= MaximumTemplates) { ++malformed; break; }
        StorageAutofillTemplate value;
        long long captured = 0, slots = 0, count = 0;
        if (!HexDecode(fields[1], value.ownerGuid) || !ValidChestId(fields[2]) ||
            !ParseInteger(fields[3], 0, LLONG_MAX, captured) || !ParseInteger(fields[4], 0, MaximumItems, slots) ||
            !ParseInteger(fields[5], 0, MaximumItems, count)) { ++malformed; break; }
        value.chestId = fields[2]; value.capturedAt = captured; value.totalSlots = int(slots);
        for (long long index = 0; index < count; ++index) {
            if (!std::getline(input, line)) { ++malformed; break; }
            if (!line.empty() && line.back() == '\r') line.pop_back();
            const auto itemFields = Fields(line);
            StorageAutofillItem item; long long slot = 0, quantity = 0, maxStack = 0;
            if (itemFields.size() != 5 || itemFields[0] != "I" ||
                !ParseInteger(itemFields[1], 0, MaximumItems - 1, slot) ||
                !ParseInteger(itemFields[2], 1, 1000000, quantity) ||
                !ParseInteger(itemFields[3], 1, 1000000, maxStack) || !HexDecode(itemFields[4], item.key)) {
                ++malformed; break;
            }
            item.slot = int(slot); item.quantity = int(quantity); item.maxStack = int(maxStack);
            value.items.push_back(std::move(item));
        }
        if (malformed || !ValidTemplate(value) || std::any_of(parsed.begin(), parsed.end(), [&](const auto& prior){ return prior.ownerGuid == value.ownerGuid && prior.chestId == value.chestId; })) { if (!malformed) ++malformed; break; }
        parsed.push_back(std::move(value));
    }
    if (!input.eof() && input.fail()) ++malformed;
    malformedRecords_ = malformed;
    if (malformed) { templates_.clear(); loaded_ = false; return false; }
    templates_ = std::move(parsed); loaded_ = true; return true;
}

bool StorageAutofillStore::SaveLocked() {
    std::vector<StorageAutofillTemplate> ordered = templates_;
    std::sort(ordered.begin(), ordered.end(), [](const auto& left, const auto& right){ return left.ownerGuid < right.ownerGuid || (left.ownerGuid == right.ownerGuid && left.chestId < right.chestId); });
    std::ostringstream content; content << "# zanik-storage-autofill\t1\n";
    for (auto& value : ordered) {
        std::sort(value.items.begin(), value.items.end(), [](const auto& left, const auto& right){ return left.slot < right.slot; });
        content << "T\t" << HexEncode(value.ownerGuid) << '\t' << value.chestId << '\t' << value.capturedAt << '\t' << value.totalSlots << '\t' << value.items.size() << '\n';
        for (const auto& item : value.items) content << "I\t" << item.slot << '\t' << item.quantity << '\t' << item.maxStack << '\t' << HexEncode(item.key) << '\n';
    }
    const std::string temporary = path_ + ".tmp";
    const int descriptor = open(temporary.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
    if (descriptor < 0) { lastSaveSucceeded_ = false; return false; }
    const std::string bytes = content.str();
    bool ok = WriteAll(descriptor, bytes) && fsync(descriptor) == 0;
    if (close(descriptor) != 0) ok = false;
    if (ok && std::rename(temporary.c_str(), path_.c_str()) == 0) {
        chmod(path_.c_str(), 0600);
        const int parent = open(Parent(path_).c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
        if (parent >= 0) { ok = fsync(parent) == 0; close(parent); }
    } else ok = false;
    if (!ok) unlink(temporary.c_str());
    lastSaveSucceeded_ = ok;
    return ok;
}

bool StorageAutofillStore::Replace(const StorageAutofillTemplate& value) {
    if (!ValidTemplate(value)) return false;
    Lock lock(mutex_);
    if (!loaded_) return false;
    const auto previous = templates_;
    auto found = std::find_if(templates_.begin(), templates_.end(), [&](const auto& item){ return item.ownerGuid == value.ownerGuid && item.chestId == value.chestId; });
    if (found == templates_.end()) { if (templates_.size() >= MaximumTemplates) return false; templates_.push_back(value); }
    else *found = value;
    if (!SaveLocked()) { templates_ = previous; return false; }
    return true;
}

bool StorageAutofillStore::Remove(const std::string& ownerGuid, const std::string& chestId) {
    Lock lock(mutex_);
    if (!loaded_) return false;
    const auto previous = templates_;
    templates_.erase(std::remove_if(templates_.begin(), templates_.end(), [&](const auto& item){ return item.ownerGuid == ownerGuid && item.chestId == chestId; }), templates_.end());
    if (!SaveLocked()) { templates_ = previous; return false; }
    return true;
}

bool StorageAutofillStore::Find(const std::string& ownerGuid, const std::string& chestId, StorageAutofillTemplate& value) const {
    Lock lock(mutex_);
    const auto found = std::find_if(templates_.begin(), templates_.end(), [&](const auto& item){ return item.ownerGuid == ownerGuid && item.chestId == chestId; });
    if (found == templates_.end()) return false;
    value = *found; return true;
}

std::vector<StorageAutofillTemplate> StorageAutofillStore::All() const { Lock lock(mutex_); return templates_; }

StorageAutofillDiagnostics StorageAutofillStore::Diagnostics() const {
    Lock lock(mutex_);
    StorageAutofillDiagnostics result; result.loaded = loaded_; result.lastSaveSucceeded = lastSaveSucceeded_; result.templates = templates_.size(); result.malformedRecords = malformedRecords_;
    struct stat file{}; const bool exists = stat(path_.c_str(), &file) == 0;
    result.readable = !exists || access(path_.c_str(), R_OK) == 0;
    result.writable = (exists ? access(path_.c_str(), W_OK) : access(Parent(path_).c_str(), W_OK)) == 0;
    result.restricted = !exists || (file.st_mode & 0077) == 0;
    return result;
}
