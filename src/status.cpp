#include "status.h"
#include "engine/dom_engine.h"
#include "utils/memory.h"
#include "player_store.h"

#include <cstdio>
#include <algorithm>
#include <array>
#include <iomanip>
#include <map>
#include <sstream>

namespace Status {
namespace {
std::string Escape(const std::string& value) {
    std::string out;
    out.reserve(value.size() + 8);
    for (unsigned char c : value) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    char hex[7];
                    std::snprintf(hex, sizeof(hex), "\\u%04x", c);
                    out += hex;
                } else {
                    out += static_cast<char>(c);
                }
        }
    }
    return out;
}

std::string DisplayName(const DomEngine::PlayerInfo& player) {
    return player.characterName.empty() ? player.name : player.characterName;
}

std::string SkillName(std::string value) {
    const std::string prefix = "SKILL_";
    if (value.compare(0, prefix.size(), prefix) == 0) value.erase(0, prefix.size());
    std::replace(value.begin(), value.end(), '_', ' ');
    return value;
}

constexpr std::array<int, 99> kLevelXp = {
    0,33,70,111,156,206,261,322,389,463,545,636,736,847,969,1104,1253,1417,1598,1798,
    2018,2261,2529,2825,3152,3512,3910,4349,4833,5367,5957,6608,7326,8118,8993,9958,
    11023,12199,13496,14929,16510,18255,20181,22307,24654,27245,30105,33262,36747,40594,
    44581,48717,52997,57421,61990,66702,71560,76562,81708,86998,92433,98012,103735,
    109603,115616,121772,128073,134518,141108,147842,154721,161743,168910,176222,183678,
    191278,199022,206911,214944,223122,231444,239910,248521,257276,266176,275219,284407,
    293740,303217,312838,322604,332514,342568,395129,447689,543044,666881,819200,1000000
};
}

int SkillLevel(int xp) {
    if (xp <= 0) return 1;
    auto it = std::upper_bound(kLevelXp.begin(), kLevelXp.end(), xp);
    return static_cast<int>(it - kLevelXp.begin());
}

std::string PlayersJson(const std::vector<std::string>& names) {
    std::string json = "{\"count\":" + std::to_string(names.size()) + ",\"players\":[";
    for (size_t i = 0; i < names.size(); ++i) {
        if (i) json += ',';
        json += '"' + Escape(names[i]) + '"';
    }
    return json + "]}";
}

std::string PlayerJson(const DomEngine::PlayerInfo& player) {
    std::vector<DomEngine::SkillInfo> skills = player.skills;
    std::sort(skills.begin(), skills.end(), [](const auto& a, const auto& b) {
        return SkillName(a.skill) < SkillName(b.skill);
    });
    std::ostringstream json;
    json << "{\"name\":\"" << Escape(DisplayName(player)) << "\",\"skills\":[";
    for (size_t i = 0; i < skills.size(); ++i) {
        if (i) json << ',';
        json << "{\"name\":\"" << Escape(SkillName(skills[i].skill))
             << "\",\"level\":" << SkillLevel(skills[i].currentXP)
             << ",\"xp\":" << skills[i].currentXP << '}';
    }
    json << "]}";
    return json.str();
}

std::string KillSummaryJson(const std::vector<StoredPlayer>& players) {
    std::map<std::string, std::uint64_t> enemies;
    std::uint64_t total = 0;
    for (const auto& player : players) for (const auto& kill : player.kills) {
        enemies[kill.enemy] += kill.count;
        total += kill.count;
    }
    std::vector<std::pair<std::string, std::uint64_t>> sorted(enemies.begin(), enemies.end());
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) {
        return a.second != b.second ? a.second > b.second : a.first < b.first;
    });
    std::ostringstream json;
    json << "{\"totalKills\":" << total << ",\"enemies\":[";
    for (size_t i = 0; i < sorted.size(); ++i) {
        if (i) json << ',';
        json << "{\"enemy\":\"" << Escape(sorted[i].first) << "\",\"count\":" << sorted[i].second << '}';
    }
    json << "]}";
    return json.str();
}

std::string KillLeaderboardJson(const std::vector<StoredPlayer>& players,
                                const std::string& enemy) {
    struct Row { std::string id, name; std::uint64_t count = 0; };
    std::vector<Row> rows;
    for (const auto& player : players) {
        Row row{player.id, player.name, 0};
        for (const auto& kill : player.kills)
            if (enemy.empty() || kill.enemy == enemy) row.count += kill.count;
        if (row.count) rows.push_back(std::move(row));
    }
    std::sort(rows.begin(), rows.end(), [](const auto& a, const auto& b) {
        return a.count != b.count ? a.count > b.count : a.name < b.name;
    });
    std::ostringstream json;
    json << "{\"enemy\":" << (enemy.empty() ? "null" : "\"" + Escape(enemy) + "\"")
         << ",\"players\":[";
    for (size_t i = 0; i < rows.size(); ++i) {
        if (i) json << ',';
        json << "{\"id\":\"" << Escape(rows[i].id) << "\",\"name\":\""
             << Escape(rows[i].name) << "\",\"count\":" << rows[i].count << '}';
    }
    json << "]}";
    return json.str();
}

std::string SkillSummaryJson(const std::vector<StoredPlayer>& players) {
    std::map<std::string, std::size_t> skills;
    std::size_t playerCount = 0;
    for (const auto& player : players) {
        if (!player.skills.empty()) ++playerCount;
        for (const auto& skill : player.skills) ++skills[SkillName(skill.name)];
    }
    std::ostringstream json;
    json << "{\"playerCount\":" << playerCount << ",\"skills\":[";
    std::size_t index = 0;
    for (const auto& entry : skills) {
        if (index++) json << ',';
        json << "{\"skill\":\"" << Escape(entry.first) << "\",\"players\":" << entry.second << '}';
    }
    json << "]}";
    return json.str();
}

std::string SkillLeaderboardJson(const std::vector<StoredPlayer>& players,
                                 const std::string& skill) {
    struct Row { std::string id, name; int level = 0; std::int64_t xp = 0; };
    std::vector<Row> rows;
    for (const auto& player : players) {
        if (skill.empty()) {
            if (player.skills.empty()) continue;
            Row row{player.id, player.name};
            for (const auto& value : player.skills) {
                row.level += SkillLevel(value.xp);
                row.xp += value.xp;
            }
            rows.push_back(std::move(row));
            continue;
        }
        auto found = std::find_if(player.skills.begin(), player.skills.end(), [&](const auto& value) {
            return SkillName(value.name) == skill;
        });
        if (found != player.skills.end()) rows.push_back({player.id, player.name, SkillLevel(found->xp), found->xp});
    }
    std::sort(rows.begin(), rows.end(), [](const auto& a, const auto& b) {
        if (a.level != b.level) return a.level > b.level;
        return a.xp != b.xp ? a.xp > b.xp : a.name < b.name;
    });
    std::ostringstream json;
    json << "{\"skill\":" << (skill.empty() ? "null" : "\"" + Escape(skill) + "\"") << ",\"players\":[";
    for (std::size_t i = 0; i < rows.size(); ++i) {
        if (i) json << ',';
        json << "{\"id\":\"" << Escape(rows[i].id) << "\",\"name\":\"" << Escape(rows[i].name)
             << (skill.empty() ? "\",\"totalLevel\":" : "\",\"level\":") << rows[i].level
             << (skill.empty() ? ",\"totalXp\":" : ",\"xp\":") << rows[i].xp << '}';
    }
    json << "]}";
    return json.str();
}

std::string PlayersJson(const std::vector<StoredPlayer>& players) {
    std::ostringstream json;
    json << "{\"count\":" << players.size() << ",\"players\":[";
    for (size_t i = 0; i < players.size(); ++i) {
        if (i) json << ',';
        json << "{\"id\":\"" << Escape(players[i].id)
             << "\",\"name\":\"" << Escape(players[i].name)
             << "\",\"online\":" << (players[i].online ? "true" : "false")
             << ",\"lastSeen\":" << players[i].lastSeen << '}';
    }
    json << "]}";
    return json.str();
}

std::string MapPlayersJson(const std::vector<StoredPlayer>& players,
                           const std::string& assetVersion,std::int64_t updatedAt) {
    std::ostringstream json;
    json << "{\"updatedAt\":" << updatedAt << ",\"assetVersion\":\""
         << Escape(assetVersion) << "\",\"players\":[";
    bool first=true;
    for(const auto& player:players){
        if(!player.online||!player.hasMapLocation)continue;
        if(!first)json<<',';
        first=false;
        json << "{\"id\":\"" << Escape(player.id) << "\",\"name\":\""
             << Escape(player.name) << "\",\"x\":" << std::fixed << std::setprecision(8)
             << player.mapX << ",\"y\":" << player.mapY << ",\"location\":"
             << (player.location.empty()?"null":"\""+Escape(player.location)+"\"")
             << ",\"dungeonEntrance\":" << (player.mapAtDungeonEntrance?"true":"false") << '}';
    }
    json << "]}";
    return json.str();
}

std::string MapLodestonesJson(const std::vector<DomEngine::LodestoneInfo>& lodestones,
                              const std::string& assetVersion,std::int64_t updatedAt) {
    std::ostringstream json;
    json << "{\"updatedAt\":" << updatedAt << ",\"assetVersion\":\""
         << Escape(assetVersion) << "\",\"lodestones\":[";
    for(size_t i=0;i<lodestones.size();++i){
        if(i)json<<',';
        json << "{\"id\":\"" << Escape(lodestones[i].id) << "\",\"x\":"
             << std::fixed << std::setprecision(8) << lodestones[i].mapX
             << ",\"y\":" << lodestones[i].mapY
             << ",\"worldX\":" << std::setprecision(1) << lodestones[i].worldLocation.X
             << ",\"worldY\":" << lodestones[i].worldLocation.Y
             << ",\"worldZ\":" << lodestones[i].worldLocation.Z << '}';
    }
    json << "]}";
    return json.str();
}

std::string MapNodestonesJson(const std::vector<DomEngine::NodestoneInfo>& nodestones,
                              const std::string& assetVersion,std::int64_t updatedAt) {
    std::ostringstream json;
    json << "{\"updatedAt\":" << updatedAt << ",\"assetVersion\":\""
         << Escape(assetVersion) << "\",\"nodestones\":[";
    for(size_t i=0;i<nodestones.size();++i){
        if(i)json<<',';
        json << "{\"id\":\"" << Escape(nodestones[i].id) << "\",\"x\":"
             << std::fixed << std::setprecision(8) << nodestones[i].mapX
             << ",\"y\":" << nodestones[i].mapY
             << ",\"displayLocation\":\"" << Escape(nodestones[i].displayLocation) << "\""
             << ",\"nexusEndpoint\":" << (nodestones[i].nexusEndpoint?"true":"false")
             << ",\"teleportable\":" << (nodestones[i].nativeSubsystem&&nodestones[i].nativeSubsystemIdentity?"true":"false")
             << ",\"worldX\":" << std::setprecision(1) << nodestones[i].worldLocation.X
             << ",\"worldY\":" << nodestones[i].worldLocation.Y
             << ",\"worldZ\":" << nodestones[i].worldLocation.Z << '}';
    }
    json << "]}";
    return json.str();
}

std::string MapResourceStatesJson(const std::vector<DomEngine::ResourceStateInfo>& states,
                                  double rate,std::int64_t updatedAt) {
    std::ostringstream json;
    json << "{\"updatedAt\":" << updatedAt << ",\"gameSecondsPerRealSecond\":"
         << std::fixed << std::setprecision(5) << rate << ",\"defaultAvailable\":true,\"states\":[";
    bool first=true;
    for(const auto& state:states){
        // Available live records need not be transferred; the static catalog's
        // default is available. Depleted records remain explicit until respawn.
        if(state.available)continue;
        if(!first)json << ',';
        first=false;
        json << "{\"id\":\"" << Escape(state.id) << "\",\"available\":false,\"remainingSeconds\":"
             << std::fixed << std::setprecision(1) << std::max(0.0,state.remainingSeconds)
             << ",\"confidence\":\"" << Escape(state.confidence) << "\"}";
    }
    json << "]}";return json.str();
}

std::string MapNpcStatesJson(const std::vector<DomEngine::NpcStateInfo>& states,
                             double rate,std::int64_t updatedAt) {
    std::ostringstream json;
    json << "{\"updatedAt\":" << updatedAt << ",\"gameSecondsPerRealSecond\":"
         << std::fixed << std::setprecision(5) << rate << ",\"states\":[";
    for(size_t i=0;i<states.size();++i){
        if(i)json << ',';
        const auto& state=states[i];
        json << "{\"id\":\"" << Escape(state.id) << "\",\"state\":\"" << Escape(state.state) << "\"";
        if(state.hasMapLocation)json << ",\"x\":" << std::setprecision(8) << state.mapX
                                    << ",\"y\":" << state.mapY;
        if(state.state=="dead")json << ",\"remainingSeconds\":" << std::setprecision(1)
                                    << std::max(0.0,state.remainingSeconds);
        json << '}';
    }
    json << "]}";return json.str();
}

std::string MapLabelsJson(const std::vector<DomEngine::MapLabelInfo>& labels,
                          const std::string& assetVersion,std::int64_t updatedAt) {
    std::ostringstream json;
    json << "{\"updatedAt\":" << updatedAt << ",\"assetVersion\":\""
         << Escape(assetVersion) << "\",\"labels\":[";
    for(size_t i=0;i<labels.size();++i){
        if(i)json<<',';
        const auto& label=labels[i];
        json << "{\"id\":\"" << Escape(label.id) << "\",\"text\":\""
             << Escape(label.text) << "\",\"x\":" << std::fixed << std::setprecision(8)
             << label.mapX << ",\"y\":" << label.mapY << ",\"rotation\":"
             << std::setprecision(3) << label.rotation << ",\"fontSize\":" << label.fontSize
             << ",\"letterTint\":[" << label.letterR << ',' << label.letterG << ',' << label.letterB << ',' << label.letterA << ']'
             << ",\"letterTintOpacity\":" << label.letterTintOpacity
             << ",\"letterOpacity\":" << label.letterOpacity << ",\"letterContrast\":" << label.letterContrast
             << ",\"glowTint\":[" << label.glowR << ',' << label.glowG << ',' << label.glowB << ',' << label.glowA << ']'
             << ",\"glowTintOpacity\":" << label.glowTintOpacity << ",\"glowOpacity\":" << label.glowOpacity
             << ",\"glowRange\":" << label.glowRange << ",\"glowBrightness\":" << label.glowBrightness
             << ",\"volumeToFlatness\":" << label.volumeToFlatness << ",\"regularToBold\":" << label.regularToBold << '}';
    }
    json << "]}";
    return json.str();
}

std::string PlayerJson(const StoredPlayer& player) {
    std::vector<StoredSkill> skills = player.skills;
    std::sort(skills.begin(), skills.end(), [](const auto& a, const auto& b) {
        return SkillName(a.name) < SkillName(b.name);
    });
    std::ostringstream json;
    json << "{\"id\":\"" << Escape(player.id)
         << "\",\"name\":\"" << Escape(player.name)
         << "\",\"online\":" << (player.online ? "true" : "false")
         << ",\"onlineSince\":" << (player.online ? player.onlineSince : 0)
         << ",\"lastSeen\":" << player.lastSeen
         << ",\"location\":" << (player.location.empty() ? "null" : "\"" + Escape(player.location) + "\"")
         << ",\"skills\":[";
    for (size_t i = 0; i < skills.size(); ++i) {
        if (i) json << ',';
        json << "{\"name\":\"" << Escape(SkillName(skills[i].name))
             << "\",\"level\":" << SkillLevel(skills[i].xp)
             << ",\"xp\":" << skills[i].xp << '}';
    }
    std::vector<StoredKill> kills = player.kills;
    std::sort(kills.begin(), kills.end(), [](const auto& a, const auto& b) {
        return a.count != b.count ? a.count > b.count : a.enemy < b.enemy;
    });
    std::uint64_t total = 0;
    for (const auto& kill : kills) total += kill.count;
    json << "],\"totalKills\":" << total << ",\"kills\":[";
    for (size_t i = 0; i < kills.size(); ++i) {
        if (i) json << ',';
        json << "{\"enemy\":\"" << Escape(kills[i].enemy) << "\",\"count\":" << kills[i].count
             << ",\"firstKill\":" << kills[i].firstKill << ",\"lastKill\":" << kills[i].lastKill << '}';
    }
    json << "],\"recentKills\":[";
    for (size_t i = 0; i < player.recentKills.size(); ++i) {
        if (i) json << ',';
        json << "{\"enemy\":\"" << Escape(player.recentKills[i].enemy)
             << "\",\"time\":" << player.recentKills[i].time << '}';
    }
    json << "]}";
    return json.str();
}

std::string ErrorJson(const std::string& message) {
    return "{\"error\":\"" + Escape(message) + "\"}";
}

bool ReadPlayers(std::vector<std::string>& names) {
    names.clear();
    auto* engine = DomEngine::g_Engine;
    if (!engine || !engine->IsInitialized()) return false;
    uintptr_t gameState = engine->GetGameState();
    if (!gameState || !engine->IsA(gameState, "DominionGameStateBase")) return false;
    uintptr_t array = gameState + DomEngine::Offsets::GameStateBase_PlayerArray;
    if (!Mem::Readable(reinterpret_cast<void*>(array), sizeof(FUEArray))) return false;
    FUEArray header = {};
    if (!Mem::Read(array, header) || header.Num < 0 || header.Num > 512 ||
        header.Max < header.Num || (header.Num > 0 && !header.Data)) return false;
    if (header.Num > 0 &&
        !Mem::Readable(header.Data, sizeof(uintptr_t) * static_cast<size_t>(header.Num))) return false;

    auto players = engine->GetAllPlayers();
    for (const auto& player : players) {
        // Only display names leave the process; no account IDs or private state.
        names.push_back(DisplayName(player));
    }
    return true;
}

bool ReadPlayer(const std::string& name, DomEngine::PlayerInfo& player, bool& found) {
    found = false;
    std::vector<std::string> names;
    if (!ReadPlayers(names)) return false;
    for (const auto& candidate : DomEngine::g_Engine->GetAllPlayers()) {
        if (DisplayName(candidate) != name) continue;
        player = candidate;
        found = true;
        break;
    }
    return true;
}
}
