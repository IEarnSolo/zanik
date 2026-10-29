#include "status.h"
#include "engine/dom_engine.h"
#include "player_store.h"
#include "recipe_snapshot_store.h"
#include "quest_snapshot_store.h"
#include "player_activity_store.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <filesystem>
#include <string>
#include <sys/stat.h>
#include <vector>

int main() {
    assert(DomEngine::UpdateMeasuredGameClockRate(60.0,0,4.0)==0.0);
    assert(std::abs(DomEngine::UpdateMeasuredGameClockRate(0.0,2400000000LL,4.0)-60.0)<0.001);
    assert(std::string(DomEngine::ResolveNpcTrackedDeath(false,false,false))=="dead");
    assert(std::string(DomEngine::ResolveNpcTrackedDeath(false,false,true))=="ready");
    assert(std::string(DomEngine::ResolveNpcTrackedDeath(true,false,true))=="clear");
    assert(std::string(DomEngine::ResolveNpcTrackedDeath(true,true,false))=="clear");
    {const std::string ready=Status::MapNpcStatesJson({{"dragon","ready",false,0,0,0}},0,123);assert(ready.find("\"state\":\"ready\"")!=std::string::npos&&ready.find("remainingSeconds")==std::string::npos);}
    assert(DomEngine::PlayerActivitySourceCompatibility(false,false,false)==-2);
    assert(DomEngine::PlayerActivitySourceCompatibility(true,false,true)==-3);
    assert(DomEngine::PlayerActivitySourceCompatibility(true,true,false)==-4);
    assert(DomEngine::PlayerActivitySourceCompatibility(true,true,true)==1);
    assert(DomEngine::PlayerActivityQueryLayoutValid({{"ItemData",0},{"ReturnValue",8},{"UnrelatedInheritedField",64}},{"ItemData"}));
    assert(!DomEngine::PlayerActivityQueryLayoutValid({{"ItemData",8},{"ReturnValue",0},{"UnrelatedInheritedField",64}},{"ItemData"}));
    assert(DomEngine::PlayerActivityDiscoveryReady(1,-2));
    assert(DomEngine::PlayerActivityDiscoveryReady(-2,1));
    assert(!DomEngine::PlayerActivityDiscoveryReady(-2,-3));
    {
        const std::string web="dist/activity-web";std::filesystem::create_directories(web);
        std::ofstream recipes(web+"/recipe-runtime-catalog.tsv");recipes<<"# schema=2 fingerprint=test count=3\n"<<"r1\tcrafting\tRECIPE_Bronze\tBronze bar\t/assets/items/bronze.png\n"<<"r1-alt\tcrafting\tRECIPE_BronzeAlt\tBronze bar\t/assets/items/bronze.png\n"<<"r2\tcrafting\tRECIPE_Iron\tIron bar\t/assets/items/iron.png\n";recipes.close();
        std::ofstream quests(web+"/quest-runtime-catalog.tsv");quests<<"# schema=1 fingerprint=test count=1\nq1\tDragon \"Slayer\"\tQuest_DragonSlayer\n";quests.close();setenv("DW_WEB_ROOT",web.c_str(),1);
        const std::string path="dist/player-activity-test.db";std::remove(path.c_str());
        PlayerActivityStore activity(path);assert(activity.Load());
        DomEngine::PlayerInfo player;player.characterGuid="hero";player.skills={{"SKILL_Attack",33}};player.hasHealth=true;player.health=10;player.isDead=false;player.location="Ashenfall";
        DomEngine::PlayerRecipeSnapshot recipe{"hero",{"r1"},"recipes-a",100,true};
        DomEngine::PlayerQuestSnapshot quest{"hero",{{"q1","not_started","",0}},"quests-a",100,true};
        DomEngine::PlayerActivityDiscoverySnapshot discovery;discovery.characterGuid="hero";discovery.regionsComplete=true;discovery.regionBitmap=1;discovery.regions={{0,"Ashenfall"},{1,"Dowdun Reach"}};discovery.materialsComplete=true;discovery.materialFingerprint="materials-a";discovery.materials={{"wood","Wood","/assets/items/wood.png"}};discovery.unlocksComplete=true;discovery.unlockFingerprint="unlocks-a";discovery.unlocks={{"perk","perk-one","Stone Sense","/assets/skills/mining.png"},{"spell","spell-one","Wind Step",""}};
        assert(activity.Sync({player},{recipe},{quest},{discovery},100));assert(activity.Page("hero",8).events.empty());
        player.skills[0].currentXP=463;player.health=0;player.isDead=true;
        recipe.unlockedIds.push_back("r1-alt");recipe.unlockedIds.push_back("r2");quest.quests[0]={"q1","in_progress","Find the dragon",0};discovery.regionBitmap=3;discovery.materials.push_back({"iron","Iron ore","/assets/items/iron.png"});discovery.unlocks.push_back({"perk","perk-two","Sacred Geometry","/assets/skills/construction.png"});discovery.unlocks.push_back({"spell","USD_TempestShield","Tempest Shield","/assets/spells-and-perks/PerkV2_Attack_TempestShield.png"});
        assert(activity.Sync({player},{recipe},{quest},{discovery},200));auto page=activity.Page("hero",10);assert(page.events.size()==8&&!page.hasMore);assert(page.events[0].type=="spell_unlocked"&&page.events[0].title=="Unlocked Tempest Shield"&&page.events[0].icon=="/assets/spells-and-perks/PerkV2_Attack_TempestShield.png"&&page.events[1].type=="perk_unlocked");const auto count=activity.Diagnostics().events;
        assert(!activity.Sync({player},{recipe},{quest},{discovery},201));assert(activity.Diagnostics().events==count);
        auto regressedPlayer=player;regressedPlayer.skills[0].currentXP=33;assert(!activity.Sync({regressedPlayer},{},{},{},210));assert(!activity.Sync({player},{},{},{},211));
        auto regressedRecipe=recipe;regressedRecipe.unlockedIds={"r1"};assert(!activity.Sync({},{regressedRecipe},{},{},220));assert(!activity.Sync({},{recipe},{},{},221));
        auto regressedDiscovery=discovery;regressedDiscovery.regionBitmap=1;regressedDiscovery.materials.resize(1);assert(!activity.Sync({},{},{},{regressedDiscovery},230));assert(!activity.Sync({},{},{},{discovery},231));
        quest.quests[0].objective.clear();assert(!activity.Sync({},{},{quest},{},290));quest.quests[0].objective="Defeat the dragon";assert(activity.Sync({player},{recipe},{quest},{discovery},300));quest.quests[0].status="completed";assert(activity.Sync({player},{recipe},{quest},{discovery},400));
        page=activity.Page("hero",3);assert(page.events.size()==3&&page.hasMore&&page.events[0].type=="quest_completed");auto older=activity.Page("hero",50,page.nextBefore);assert(!older.events.empty());
        const auto beforeCatalogChange=activity.Diagnostics().events;recipe.catalogFingerprint="recipes-b";recipe.unlockedIds={"r1"};quest.catalogFingerprint="quests-b";quest.quests[0]={"q1","not_started","",0};assert(activity.Sync({},{recipe},{quest},{},450));assert(activity.Diagnostics().events==beforeCatalogChange);
        const std::string json=activity.PageJson("hero",8);assert(json.find("characterGuid")==std::string::npos&&json.find("hero")==std::string::npos&&json.find("quest_completed")!=std::string::npos&&json.find("Dragon \\\"Slayer\\\"")!=std::string::npos);
        struct stat activityMode{};assert(stat(path.c_str(),&activityMode)==0&&(activityMode.st_mode&0777)==0600);
        {std::ofstream bad(path,std::ios::app);bad<<"E\tmalformed\nX\thero\tzz\tmaybe\tzz\nU\thero\tspell\tzz\t2\n";}
        PlayerActivityStore loaded(path);assert(loaded.Load());assert(loaded.Diagnostics().events==activity.Diagnostics().events);assert(loaded.Page("hero",8).events[0].type=="quest_completed");
        auto afterRestart=discovery;afterRestart.unlocks.push_back({"perk","perk-three","Focused Builder",""});afterRestart.unlocks.push_back({"spell","spell-three","Lunar Step",""});const auto restartCount=loaded.Diagnostics().events;assert(loaded.Sync({},{},{},{afterRestart},500));assert(loaded.Diagnostics().events==restartCount+2);assert(loaded.Page("hero",2).events[0].type=="spell_unlocked"&&loaded.Page("hero",2).events[1].type=="perk_unlocked");
        auto changedCatalog=afterRestart;changedCatalog.materialFingerprint="materials-b";changedCatalog.unlockFingerprint="unlocks-b";changedCatalog.materials.push_back({"flax","Flax","/assets/items/flax.png"});changedCatalog.unlocks.push_back({"spell","spell-four","Silent Baseline Spell",""});const auto catalogCount=loaded.Diagnostics().events;assert(loaded.Sync({},{},{},{changedCatalog},510));assert(loaded.Diagnostics().events==catalogCount);std::remove(path.c_str());
        const std::string missing="dist/missing-activity-parent/player-activity.db";std::filesystem::remove_all("dist/missing-activity-parent");PlayerActivityStore failing(missing);assert(failing.Load());assert(!failing.Sync({player},{},{},{},500));assert(failing.Diagnostics().persistenceFailures==1);assert(failing.PageJson("hero",8).find("\"persistenceError\":true")!=std::string::npos);
    }
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
    stored.onlineSince = 120000;
    stored.lastSeen = 123456;
    stored.location = "Temple <Woods>";
    stored.skills = {{"SKILL_Woodcutting", 463}, {"SKILL_Attack", 33}};
    stored.kills = {{"Goblin", 3, 100, 300}, {"Wolf", 2, 200, 250}};
    stored.recentKills = {{"Goblin", 300}, {"Wolf", 250}};
    assert(Status::PlayerJson(stored) ==
           R"({"id":"abc123","name":"Ada <Admin>","online":false,"onlineSince":0,"lastSeen":123456,"location":"Temple <Woods>","skills":[{"name":"Attack","level":2,"xp":33},{"name":"Woodcutting","level":10,"xp":463}],"totalKills":5,"kills":[{"enemy":"Goblin","count":3,"firstKill":100,"lastKill":300},{"enemy":"Wolf","count":2,"firstKill":200,"lastKill":250}],"recentKills":[{"enemy":"Goblin","time":300},{"enemy":"Wolf","time":250}]})");
    assert(Status::PlayersJson(std::vector<StoredPlayer>{stored}) ==
           R"({"count":1,"players":[{"id":"abc123","name":"Ada <Admin>","online":false,"lastSeen":123456}]})");
    stored.online = true;
    assert(Status::PlayerJson(stored).find("\"onlineSince\":120000") != std::string::npos);
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
    DomEngine::NodestoneInfo nodestone;nodestone.id="opaque-b";nodestone.mapX=0.25;nodestone.mapY=0.75;
    nodestone.worldLocation={227321.125,54473.875,-123.0};nodestone.displayLocation="The Nexus · Garrison route";
    nodestone.nexusEndpoint=true;nodestone.nativeSubsystem=0x1000;nodestone.nativeSubsystemIdentity=42;
    assert(Status::MapNodestonesJson({nodestone},"world-map-1.0.0.2",987654)==
           R"({"updatedAt":987654,"assetVersion":"world-map-1.0.0.2","nodestones":[{"id":"opaque-b","x":0.25000000,"y":0.75000000,"displayLocation":"The Nexus · Garrison route","nexusEndpoint":true,"teleportable":true,"worldX":227321.1,"worldY":54473.9,"worldZ":-123.0}]})");
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
    assert(known[0].onlineSince > 0);
    assert(known[0].location == "Bramblemead Valley");
    assert(known[0].hasMapLocation && known[0].mapX == 0.125 && known[0].mapY == 0.625);
    const std::int64_t sessionStarted = known[0].onlineSince;
    StoredPlayer disconnected;
    assert(!store.Sync({live}));
    assert(store.Find(known[0].id, disconnected) && disconnected.onlineSince == sessionStarted);
    assert(store.Sync({}));
    assert(store.Find(known[0].id, disconnected) && !disconnected.online && disconnected.onlineSince == 0);
    assert(store.Sync({live}));
    assert(store.Find(known[0].id, disconnected) && disconnected.online && disconnected.onlineSince > 0);
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
    assert(Status::SkillSummaryJson({loaded}) ==
           R"({"playerCount":1,"skills":[{"skill":"Mining","players":1}]})");
    assert(Status::SkillLeaderboardJson({loaded}, "Mining") ==
           std::string("{\"skill\":\"Mining\",\"players\":[{\"id\":\"") + publicId +
           "\",\"name\":\"Saved Player\",\"level\":30,\"xp\":5367}]}");
    StoredPlayer multiSkill;
    multiSkill.id = "multi";
    multiSkill.name = "Multi Skill";
    multiSkill.skills = {{"SKILL_Attack", 33}, {"SKILL_Woodcutting", 463}};
    assert(Status::SkillLeaderboardJson({multiSkill, loaded}, "") ==
           std::string("{\"skill\":null,\"players\":[{\"id\":\"") + publicId +
           "\",\"name\":\"Saved Player\",\"totalLevel\":30,\"totalXp\":5367},{\"id\":\"multi\",\"name\":\"Multi Skill\",\"totalLevel\":12,\"totalXp\":496}]}");
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
