#pragma once

#include "engine/dom_engine.h"

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

struct PlayerActivityEvent {
    std::uint64_t id = 0;
    std::string characterGuid;
    std::int64_t timestamp = 0;
    std::string type;
    std::string title;
    std::string detail;
    std::string icon;
};

struct PlayerActivityPage {
    std::vector<PlayerActivityEvent> events;
    bool hasMore = false;
    std::uint64_t nextBefore = 0;
};

struct PlayerActivityDiagnostics {
    int compatibility = 1;
    std::uint64_t events = 0;
    std::uint64_t players = 0;
    std::uint64_t snapshots = 0;
    std::uint64_t droppedSamples = 0;
    std::uint64_t persistenceFailures = 0;
};

class PlayerActivityStore {
public:
    explicit PlayerActivityStore(std::string path);
    bool Load();
    bool Save() const;
    bool Sync(const std::vector<DomEngine::PlayerInfo>& players,
              const std::vector<DomEngine::PlayerRecipeSnapshot>& recipes,
              const std::vector<DomEngine::PlayerQuestSnapshot>& quests,
              const std::vector<DomEngine::PlayerActivityDiscoverySnapshot>& discoveries,
              std::int64_t timestamp = 0);
    PlayerActivityPage Page(const std::string& characterGuid, std::size_t limit,
                            std::uint64_t before = 0) const;
    std::string PageJson(const std::string& characterGuid, std::size_t limit,
                         std::uint64_t before = 0) const;
    PlayerActivityDiagnostics Diagnostics() const;

private:
    struct QuestState {
        std::string status, objective;
        bool operator==(const QuestState& other) const { return status==other.status&&objective==other.objective; }
        bool operator!=(const QuestState& other) const { return !(*this==other); }
    };
    struct Baseline {
        bool skillsInitialized = false;
        std::map<std::string, int> skillLevels;
        bool recipesInitialized = false;
        std::string recipeFingerprint;
        std::set<std::string> recipeGroups;
        bool questsInitialized = false;
        std::string questFingerprint;
        std::map<std::string, QuestState> quests;
        bool deathInitialized = false;
        bool dead = false;
        std::string lastKnownLocation;
        bool regionsInitialized = false;
        std::uint32_t regions = 0;
        std::uint32_t everRegions = 0;
        bool materialsInitialized = false;
        std::set<std::string> materials;
        std::set<std::string> everMaterials;
    };

    void Add(const std::string& guid, std::int64_t timestamp, const std::string& type,
             const std::string& title, const std::string& detail = {},
             const std::string& icon = {});
    bool LoadCatalogs();
    std::string QuestName(const std::string& id) const;
    std::string RecipeGroup(const std::string& id) const;
    std::string RecipeName(const std::string& id) const;
    std::string RecipeIcon(const std::string& id) const;

    std::string path_;
    std::uint64_t nextId_ = 1;
    std::map<std::string, Baseline> baselines_;
    std::vector<PlayerActivityEvent> events_;
    std::map<std::string, std::string> questNames_;
    struct RecipeMeta { std::string group, name, icon; };
    std::map<std::string, RecipeMeta> recipes_;
    mutable std::uint64_t persistenceFailures_ = 0;
    std::uint64_t snapshots_ = 0;
    std::uint64_t droppedSamples_ = 0;
    bool dirty_ = false;
    bool persistenceHealthy_ = true;
};
