#include "status.h"
#include "engine/dom_engine.h"
#include "player_store.h"
#include "recipe_snapshot_store.h"
#include "quest_snapshot_store.h"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

int main() {
    {
        std::ofstream hashInput("dist/sha256-test.txt", std::ios::binary);
        hashInput << "abc";
        hashInput.close();
        assert(DomEngine::FileSha256("dist/sha256-test.txt") ==
               "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    }
    assert(Status::PlayersJson(std::vector<std::string>{}) == R"({"count":0,"players":[]})");
    assert(Status::PlayersJson({"Ada"}) == R"({"count":1,"players":["Ada"]})");
    assert(Status::PlayersJson(std::vector<std::string>{"Ada", "Bob"}) ==
           R"({"count":2,"players":["Ada","Bob"]})");
    assert(Status::PlayersJson({"<script>alert(1)</script>", "A\"B", "Line\nBreak"}) ==
           R"({"count":3,"players":["<script>alert(1)</script>","A\"B","Line\nBreak"]})");
    assert(Status::SkillLevel(0) == 1);
    assert(Status::SkillLevel(32) == 1);
    assert(Status::SkillLevel(33) == 2);
    assert(Status::SkillLevel(342568) == 93);
    assert(Status::SkillLevel(999999) == 98);
    assert(Status::SkillLevel(1000000) == 99);
    DomEngine::PlayerInfo player;
    player.name = "Ada <Admin>";
    player.skills = {{"SKILL_Woodcutting", 463}, {"SKILL_Attack", 33}};
    assert(Status::PlayerJson(player) ==
           R"({"name":"Ada <Admin>","skills":[{"name":"Attack","level":2,"xp":33},{"name":"Woodcutting","level":10,"xp":463}]})");
    StoredPlayer stored;
    stored.id = "abc123";
    stored.name = "Ada <Admin>";
    stored.online = false;
    stored.lastSeen = 123456;
    stored.location = "Temple <Woods>";
    stored.skills = {{"SKILL_Woodcutting", 463}, {"SKILL_Attack", 33}};
    stored.kills = {{"Goblin", 3, 100, 300}, {"Wolf", 2, 200, 250}};
    stored.recentKills = {{"Goblin", 300}, {"Wolf", 250}};
    assert(Status::PlayerJson(stored) ==
           R"({"id":"abc123","name":"Ada <Admin>","online":false,"lastSeen":123456,"location":"Temple <Woods>","skills":[{"name":"Attack","level":2,"xp":33},{"name":"Woodcutting","level":10,"xp":463}],"totalKills":5,"kills":[{"enemy":"Goblin","count":3,"firstKill":100,"lastKill":300},{"enemy":"Wolf","count":2,"firstKill":200,"lastKill":250}],"recentKills":[{"enemy":"Goblin","time":300},{"enemy":"Wolf","time":250}]})");
    assert(Status::PlayersJson(std::vector<StoredPlayer>{stored}) ==
           R"({"count":1,"players":[{"id":"abc123","name":"Ada <Admin>","online":false,"lastSeen":123456}]})");
    stored.online = true;
    stored.hasMapLocation = true;
    stored.mapX = 0.25;
    stored.mapY = 0.75;
    stored.mapAtDungeonEntrance = true;
    assert(Status::MapPlayersJson({stored}, "world-map-1.0.0.2", 987654) ==
           R"({"updatedAt":987654,"assetVersion":"world-map-1.0.0.2","players":[{"id":"abc123","name":"Ada <Admin>","x":0.25000000,"y":0.75000000,"location":"Temple <Woods>","dungeonEntrance":true}]})");
    DomEngine::LodestoneInfo lodestone;
    lodestone.id="opaque-a";lodestone.mapX=0.125;lodestone.mapY=0.875;
    lodestone.worldLocation={123.25,-456.75,78.0};
    std::vector<DomEngine::LodestoneInfo> lodestones={lodestone};
    assert(Status::MapLodestonesJson(lodestones,"world-map-1.0.0.2",987654)==
           R"({"updatedAt":987654,"assetVersion":"world-map-1.0.0.2","lodestones":[{"id":"opaque-a","x":0.12500000,"y":0.87500000,"worldX":123.2,"worldY":-456.8,"worldZ":78.0}]})");
    std::vector<DomEngine::MapLabelInfo> labels={{"label-a","Temple <Woods>",0.1,0.2,-3.5,42}};
    assert(Status::MapLabelsJson(labels,"world-map-1.0.0.2",987654)==
           R"({"updatedAt":987654,"assetVersion":"world-map-1.0.0.2","labels":[{"id":"label-a","text":"Temple <Woods>","x":0.10000000,"y":0.20000000,"rotation":-3.500,"fontSize":42,"letterTint":[0.000,0.000,0.000,1.000],"letterTintOpacity":1.000,"letterOpacity":1.000,"letterContrast":1.000,"glowTint":[0.000,0.000,0.000,1.000],"glowTintOpacity":1.000,"glowOpacity":1.000,"glowRange":0.000,"glowBrightness":1.000,"volumeToFlatness":0.000,"regularToBold":0.000}]})");
    const char* storePath = "dist/store-test.db";
    std::remove(storePath);
    PlayerStore store(storePath);
    DomEngine::PlayerInfo live;
    live.uniqueNetId = "private-account-id";
    live.name = "Saved Player";
    store.Sync({live});
    assert(store.All().empty());
    live.characterGuid = "character-guid";
    live.characterName = "Saved Player";
    live.skills = {{"SKILL_Mining", 5367}};
    live.location = "Bramblemead Valley";
    live.hasMapLocation = true;
    live.mapX = 0.125;
    live.mapY = 0.625;
    store.Sync({live});
    auto known = store.All();
    assert(known.size() == 1 && known[0].online && known[0].skills[0].xp == 5367);
    assert(known[0].location == "Bramblemead Valley");
    assert(known[0].hasMapLocation && known[0].mapX == 0.125 && known[0].mapY == 0.625);
    assert(!store.Sync({live}));
    assert(store.RecordKill("character-guid", "Goblin <Scout>", 1000));
    assert(store.RecordKill("character-guid", "Goblin <Scout>", 1010));
    assert(store.RenameEnemy("Goblin <Scout>", "Goblin Archer"));
    std::string publicId = known[0].id;
    PlayerStore restored(storePath);
    assert(restored.Load());
    StoredPlayer loaded;
    assert(restored.Find(publicId, loaded));
    assert(loaded.name == "Saved Player" && !loaded.online && loaded.skills[0].xp == 5367 &&
           loaded.location == "Bramblemead Valley");
    assert(loaded.kills.size() == 1 && loaded.kills[0].enemy == "Goblin Archer" &&
           loaded.kills[0].count == 2 && loaded.kills[0].firstKill == 1000 &&
           loaded.kills[0].lastKill == 1010);
    assert(loaded.recentKills.size() == 2 && loaded.recentKills[0].time == 1010);
    assert(Status::KillSummaryJson({loaded}) ==
           R"({"totalKills":2,"enemies":[{"enemy":"Goblin Archer","count":2}]})");
    assert(Status::KillLeaderboardJson({loaded}, "Goblin Archer") ==
           std::string("{\"enemy\":\"Goblin Archer\",\"players\":[{\"id\":\"") + publicId +
           "\",\"name\":\"Saved Player\",\"count\":2}]}");
    std::remove(storePath);
    const char* recipeStorePath="dist/player-recipe-snapshot-test.db";
    std::remove(recipeStorePath);
    RecipeSnapshotStore recipeStore(recipeStorePath);
    DomEngine::PlayerRecipeSnapshot recipeLive;
    recipeLive.characterGuid="character-guid";recipeLive.catalogFingerprint=std::string(64,'a');
    recipeLive.unlockedIds={"0123456789abcdef","fedcba9876543210"};recipeLive.capturedAt=1234;recipeLive.complete=true;
    assert(recipeStore.Sync({recipeLive}));
    RecipeSnapshotStore recipeRestored(recipeStorePath);assert(recipeRestored.Load());StoredRecipeSnapshot recipeSaved;
    assert(recipeRestored.Find("character-guid",recipeSaved)&&recipeSaved.complete&&recipeSaved.capturedAt==1234&&recipeSaved.unlockedIds.size()==2);
    DomEngine::PlayerRecipeSnapshot partial=recipeLive;partial.complete=false;partial.capturedAt=9999;partial.unlockedIds.clear();
    assert(recipeRestored.Sync({partial}));assert(recipeRestored.Find("character-guid",recipeSaved)&&recipeSaved.capturedAt==1234&&recipeSaved.unlockedIds.size()==2);
    DomEngine::PlayerRecipeSnapshot empty=recipeLive;empty.capturedAt=10000;empty.unlockedIds.clear();
    assert(recipeRestored.Sync({empty}));assert(recipeRestored.Find("character-guid",recipeSaved)&&recipeSaved.capturedAt==1234&&recipeSaved.unlockedIds.size()==2);
    DomEngine::PlayerRecipeSnapshot regressed=recipeLive;regressed.capturedAt=10001;regressed.unlockedIds={"0123456789abcdef"};
    assert(recipeRestored.Sync({regressed}));assert(recipeRestored.Find("character-guid",recipeSaved)&&recipeSaved.capturedAt==1234&&recipeSaved.unlockedIds.size()==2);
    std::remove(recipeStorePath);
    const char* questStorePath="dist/player-quest-snapshot-test.db";
    std::remove(questStorePath);
    QuestSnapshotStore questStore(questStorePath);
    DomEngine::PlayerQuestSnapshot questLive;
    questLive.characterGuid="character-guid";questLive.catalogFingerprint=std::string(64,'b');questLive.capturedAt=2000;questLive.complete=true;
    questLive.quests={{"quest-a","in_progress","Speak to the Wise Old Man",0},{"quest-b","completed","",0}};
    assert(questStore.Sync({questLive}));
    QuestSnapshotStore questRestored(questStorePath);assert(questRestored.Load());StoredQuestSnapshot questSaved;
    assert(questRestored.Find("character-guid",questSaved)&&questSaved.complete&&questSaved.quests.size()==2);
    assert(questSaved.quests[1].completedObservedAt==2000);
    DomEngine::PlayerQuestSnapshot questUpdated=questLive;questUpdated.capturedAt=3000;questUpdated.quests[0].status="completed";
    assert(questRestored.Sync({questUpdated}));assert(questRestored.Find("character-guid",questSaved));
    assert(questSaved.quests[0].completedObservedAt==3000&&questSaved.quests[1].completedObservedAt==2000);
    DomEngine::PlayerQuestSnapshot questPartial=questUpdated;questPartial.complete=false;questPartial.capturedAt=4000;questPartial.quests.clear();
    assert(questRestored.Sync({questPartial}));assert(questRestored.Find("character-guid",questSaved)&&questSaved.capturedAt==3000);
    std::remove(questStorePath);
    return 0;
}
