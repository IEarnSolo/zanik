#pragma once

#include "engine/dom_engine.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

struct StoredSkill {
    std::string name;
    int xp = 0;
};

struct StoredKill {
    std::string enemy;
    std::uint64_t count = 0;
    std::int64_t firstKill = 0;
    std::int64_t lastKill = 0;
};

struct RecentKill {
    std::string enemy;
    std::int64_t time = 0;
};

struct StoredPlayer {
    std::string identity;
    std::string id;
    std::string name;
    std::string location;
    std::vector<StoredSkill> skills;
    std::vector<StoredKill> kills;
    std::vector<RecentKill> recentKills;
    std::int64_t lastSeen = 0;
    bool online = false;
    bool hasMapLocation = false;
    double mapX = 0, mapY = 0;
    bool mapAtDungeonEntrance = false;
};

class PlayerStore {
public:
    explicit PlayerStore(std::string path);
    bool Load();
    bool Sync(const std::vector<DomEngine::PlayerInfo>& onlinePlayers);
    bool Save() const;
    std::vector<StoredPlayer> All() const;
    std::vector<StoredPlayer> Online() const;
    bool Find(const std::string& id, StoredPlayer& player) const;
    bool RecordKill(const std::string& characterGuid, const std::string& enemy,
                    std::int64_t timestamp);
    bool RenameEnemy(const std::string& oldName, const std::string& newName);
    const std::string& Path() const { return path_; }

private:
    std::string path_;
    std::map<std::string, StoredPlayer> players_;
};
