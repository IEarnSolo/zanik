#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace DomEngine { struct PlayerInfo; }
namespace DomEngine { struct LodestoneInfo; }
namespace DomEngine { struct NodestoneInfo; }
namespace DomEngine { struct MapLabelInfo; }
namespace DomEngine { struct ResourceStateInfo; }
namespace DomEngine { struct NpcStateInfo; }
struct StoredPlayer;

namespace Status {
std::string PlayersJson(const std::vector<std::string>& names);
std::string PlayerJson(const DomEngine::PlayerInfo& player);
std::string PlayersJson(const std::vector<StoredPlayer>& players);
std::string PlayerJson(const StoredPlayer& player);
std::string KillSummaryJson(const std::vector<StoredPlayer>& players);
std::string KillLeaderboardJson(const std::vector<StoredPlayer>& players,
                                const std::string& enemy);
std::string SkillSummaryJson(const std::vector<StoredPlayer>& players);
std::string SkillLeaderboardJson(const std::vector<StoredPlayer>& players,
                                 const std::string& skill);
std::string MapPlayersJson(const std::vector<StoredPlayer>& players,
                           const std::string& assetVersion, std::int64_t updatedAt);
std::string MapLodestonesJson(const std::vector<DomEngine::LodestoneInfo>& lodestones,
                              const std::string& assetVersion, std::int64_t updatedAt);
std::string MapNodestonesJson(const std::vector<DomEngine::NodestoneInfo>& nodestones,
                              const std::string& assetVersion, std::int64_t updatedAt);
std::string MapLabelsJson(const std::vector<DomEngine::MapLabelInfo>& labels,
                          const std::string& assetVersion, std::int64_t updatedAt);
std::string MapResourceStatesJson(const std::vector<DomEngine::ResourceStateInfo>& states,
                                  double gameSecondsPerRealSecond, std::int64_t updatedAt);
std::string MapNpcStatesJson(const std::vector<DomEngine::NpcStateInfo>& states,
                             double gameSecondsPerRealSecond, std::int64_t updatedAt);
std::string ErrorJson(const std::string& message);
bool ReadPlayers(std::vector<std::string>& names);
bool ReadPlayer(const std::string& name, DomEngine::PlayerInfo& player, bool& found);
int SkillLevel(int xp);
}
