#include "player_store.h"
#include "utils/logger.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <set>
#include <sstream>
#include <random>

namespace {
std::string HexEncode(const std::string& value) {
    static const char digits[] = "0123456789abcdef";
    std::string out;
    out.reserve(value.size() * 2);
    for (unsigned char c : value) { out += digits[c >> 4]; out += digits[c & 15]; }
    return out;
}

int Nibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

bool HexDecode(const std::string& value, std::string& out) {
    if (value.size() % 2) return false;
    out.clear(); out.reserve(value.size() / 2);
    for (size_t i = 0; i < value.size(); i += 2) {
        int a = Nibble(value[i]), b = Nibble(value[i + 1]);
        if (a < 0 || b < 0) return false;
        out += static_cast<char>((a << 4) | b);
    }
    return true;
}

std::vector<std::string> Split(const std::string& line) {
    std::vector<std::string> fields;
    size_t start = 0;
    while (true) {
        size_t tab = line.find('\t', start);
        fields.push_back(line.substr(start, tab == std::string::npos ? tab : tab - start));
        if (tab == std::string::npos) break;
        start = tab + 1;
    }
    return fields;
}

std::int64_t Now() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string Identity(const DomEngine::PlayerInfo& player) {
    // A PlayerState initially contains the platform display name while the
    // selected character is still loading. Record only the stable character
    // identity so that phase never becomes a second player.
    return player.characterGuid.empty() ? "" : "character:" + player.characterGuid;
}

std::string PublicId(const std::string& identity) {
    std::uint64_t hash = 1469598103934665603ULL;
    for (unsigned char c : identity) { hash ^= c; hash *= 1099511628211ULL; }
    std::ostringstream out;
    out << std::hex << std::setfill('0') << std::setw(16) << hash;
    return out.str();
}

std::string RandomPublicId() {
    std::random_device source;
    std::uint64_t a = (static_cast<std::uint64_t>(source()) << 32) ^ source() ^
                      static_cast<std::uint64_t>(Now());
    std::uint64_t b = (static_cast<std::uint64_t>(source()) << 32) ^ source();
    std::ostringstream out;
    out << std::hex << std::setfill('0') << std::setw(16) << a << std::setw(16) << b;
    return out.str();
}
}

PlayerStore::PlayerStore(std::string path) : path_(std::move(path)) {}

bool PlayerStore::Load() {
    std::ifstream input(path_);
    if (!input.is_open()) return true;
    std::string line;
    while (std::getline(input, line)) {
        auto fields = Split(line);
        if (fields.empty()) continue;
        if (fields[0] == "P" && (fields.size() == 4 || fields.size() == 5)) {
            std::string identity, name;
            size_t nameIndex = fields.size() == 5 ? 3 : 2;
            size_t seenIndex = fields.size() == 5 ? 4 : 3;
            if (!HexDecode(fields[1], identity) || !HexDecode(fields[nameIndex], name)) continue;
            StoredPlayer player;
            player.identity = identity;
            if (identity.compare(0, 5, "name:") == 0) continue;
            player.id = fields.size() == 5 ? fields[2] : PublicId(identity);
            player.name = name;
            try { player.lastSeen = std::stoll(fields[seenIndex]); } catch (...) { continue; }
            players_[identity] = player;
        } else if (fields[0] == "S" && fields.size() == 4) {
            std::string identity, skill;
            if (!HexDecode(fields[1], identity) || !HexDecode(fields[2], skill)) continue;
            auto it = players_.find(identity);
            if (it == players_.end()) continue;
            int xp = 0;
            try { xp = std::stoi(fields[3]); } catch (...) { continue; }
            it->second.skills.push_back({skill, xp});
        } else if (fields[0] == "L" && fields.size() == 3) {
            std::string identity, location;
            if (!HexDecode(fields[1], identity) || !HexDecode(fields[2], location)) continue;
            auto it = players_.find(identity);
            if (it != players_.end()) it->second.location = location;
        } else if (fields[0] == "K" && fields.size() == 6) {
            std::string identity, enemy;
            if (!HexDecode(fields[1], identity) || !HexDecode(fields[2], enemy)) continue;
            auto it = players_.find(identity);
            if (it == players_.end()) continue;
            try {
                it->second.kills.push_back({enemy, std::stoull(fields[3]),
                                            std::stoll(fields[4]), std::stoll(fields[5])});
            } catch (...) {}
        } else if (fields[0] == "R" && fields.size() == 4) {
            std::string identity, enemy;
            if (!HexDecode(fields[1], identity) || !HexDecode(fields[2], enemy)) continue;
            auto it = players_.find(identity);
            if (it == players_.end()) continue;
            try { it->second.recentKills.push_back({enemy, std::stoll(fields[3])}); } catch (...) {}
        }
    }
    LogMessage("Loaded " + std::to_string(players_.size()) + " known players from " + path_);
    return true;
}

bool PlayerStore::Sync(const std::vector<DomEngine::PlayerInfo>& onlinePlayers) {
    bool changed = false;
    std::set<std::string> seen;
    std::int64_t now = Now();
    for (const auto& live : onlinePlayers) {
        std::string identity = Identity(live);
        if (identity.empty() || live.characterName.empty()) continue;
        seen.insert(identity);
        auto current = players_.find(identity);
        if (current == players_.end()) {
            // Migrate records made by versions that keyed a resolved player by
            // account ID. This preserves its public page URL and saved skills.
            for (auto legacy = players_.begin(); legacy != players_.end(); ++legacy) {
                if (legacy->first.compare(0, 4, "net:") != 0 ||
                    legacy->second.name != live.characterName) continue;
                StoredPlayer migrated = std::move(legacy->second);
                players_.erase(legacy);
                migrated.identity = identity;
                current = players_.emplace(identity, std::move(migrated)).first;
                changed = true;
                break;
            }
        }
        StoredPlayer& stored = current == players_.end() ? players_[identity] : current->second;
        if (stored.identity.empty()) {
            stored.identity = identity;
            stored.id = RandomPublicId();
            changed = true;
        }
        std::string name = live.characterName;
        if (!name.empty() && stored.name != name) { stored.name = name; changed = true; }
        if (!live.location.empty() && stored.location != live.location) {
            stored.location = live.location;
            changed = true;
        }
        stored.hasMapLocation = live.hasMapLocation;
        stored.mapX = live.mapX;
        stored.mapY = live.mapY;
        stored.mapAtDungeonEntrance = live.mapAtDungeonEntrance;
        std::vector<StoredSkill> skills;
        for (const auto& skill : live.skills) skills.push_back({skill.skill, skill.currentXP});
        std::sort(skills.begin(), skills.end(), [](const auto& a, const auto& b) { return a.name < b.name; });
        std::sort(stored.skills.begin(), stored.skills.end(), [](const auto& a, const auto& b) { return a.name < b.name; });
        if (skills.size() != stored.skills.size() || !std::equal(skills.begin(), skills.end(), stored.skills.begin(),
            [](const auto& a, const auto& b) { return a.name == b.name && a.xp == b.xp; })) {
            stored.skills = std::move(skills);
            changed = true;
        }
        if (!stored.online || stored.onlineSince <= 0) {
            stored.online = true;
            stored.onlineSince = now;
            changed = true;
        }
        // Persist last-seen at most once per minute; online status remains live in memory.
        if (now - stored.lastSeen >= 60) { stored.lastSeen = now; changed = true; }
    }
    for (auto& entry : players_) {
        if (entry.second.online && seen.find(entry.first) == seen.end()) {
            entry.second.online = false;
            entry.second.onlineSince = 0;
            entry.second.hasMapLocation = false;
            changed = true;
        }
    }
    if (changed) Save();
    return changed;
}

bool PlayerStore::Save() const {
    std::string temporary = path_ + ".tmp";
    std::ofstream output(temporary, std::ios::trunc);
    if (!output.is_open()) { LogMessage("Could not write player store " + temporary); return false; }
    for (const auto& entry : players_) {
        const StoredPlayer& player = entry.second;
        output << "P\t" << HexEncode(player.identity) << '\t' << player.id << '\t' << HexEncode(player.name)
               << '\t' << player.lastSeen << '\n';
        for (const auto& skill : player.skills)
            output << "S\t" << HexEncode(player.identity) << '\t' << HexEncode(skill.name)
                   << '\t' << skill.xp << '\n';
        if (!player.location.empty())
            output << "L\t" << HexEncode(player.identity) << '\t' << HexEncode(player.location) << '\n';
        for (const auto& kill : player.kills)
            output << "K\t" << HexEncode(player.identity) << '\t' << HexEncode(kill.enemy)
                   << '\t' << kill.count << '\t' << kill.firstKill << '\t' << kill.lastKill << '\n';
        for (const auto& kill : player.recentKills)
            output << "R\t" << HexEncode(player.identity) << '\t' << HexEncode(kill.enemy)
                   << '\t' << kill.time << '\n';
    }
    output.close();
    if (std::rename(temporary.c_str(), path_.c_str()) != 0) {
        LogMessage("Could not replace player store " + path_);
        return false;
    }
    return true;
}

bool PlayerStore::RecordKill(const std::string& characterGuid, const std::string& enemy,
                             std::int64_t timestamp) {
    if (characterGuid.empty() || enemy.empty()) return false;
    auto it = players_.find("character:" + characterGuid);
    if (it == players_.end()) return false;
    StoredPlayer& player = it->second;
    auto kill = std::find_if(player.kills.begin(), player.kills.end(),
        [&](const StoredKill& value) { return value.enemy == enemy; });
    if (kill == player.kills.end()) {
        player.kills.push_back({enemy, 1, timestamp, timestamp});
    } else {
        ++kill->count;
        if (!kill->firstKill) kill->firstKill = timestamp;
        kill->lastKill = timestamp;
    }
    player.recentKills.insert(player.recentKills.begin(), {enemy, timestamp});
    if (player.recentKills.size() > 20) player.recentKills.resize(20);
    return Save();
}

bool PlayerStore::RenameEnemy(const std::string& oldName, const std::string& newName) {
    if (oldName.empty() || newName.empty() || oldName == newName) return false;
    bool changed = false;
    for (auto& entry : players_) {
        StoredPlayer& player = entry.second;
        auto oldKill = std::find_if(player.kills.begin(), player.kills.end(),
            [&](const StoredKill& value) { return value.enemy == oldName; });
        if (oldKill != player.kills.end()) {
            auto newKill = std::find_if(player.kills.begin(), player.kills.end(),
                [&](const StoredKill& value) { return value.enemy == newName; });
            if (newKill == player.kills.end()) {
                oldKill->enemy = newName;
            } else {
                newKill->count += oldKill->count;
                if (!newKill->firstKill || (oldKill->firstKill && oldKill->firstKill < newKill->firstKill))
                    newKill->firstKill = oldKill->firstKill;
                if (oldKill->lastKill > newKill->lastKill) newKill->lastKill = oldKill->lastKill;
                player.kills.erase(oldKill);
            }
            changed = true;
        }
        for (auto& recent : player.recentKills) {
            if (recent.enemy == oldName) { recent.enemy = newName; changed = true; }
        }
    }
    if (changed) Save();
    return changed;
}

std::vector<StoredPlayer> PlayerStore::All() const {
    std::vector<StoredPlayer> result;
    for (const auto& entry : players_) result.push_back(entry.second);
    std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) { return a.name < b.name; });
    return result;
}

std::vector<StoredPlayer> PlayerStore::Online() const {
    auto result = All();
    result.erase(std::remove_if(result.begin(), result.end(), [](const auto& p) { return !p.online; }), result.end());
    return result;
}

bool PlayerStore::Find(const std::string& id, StoredPlayer& player) const {
    for (const auto& entry : players_) {
        if (entry.second.id != id) continue;
        player = entry.second;
        return true;
    }
    return false;
}
