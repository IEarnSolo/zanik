#include "storage_autofill_store.h"

#include <cassert>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

int main(){
    StorageAutofillTemplate desired;desired.ownerGuid="00000000-0000-0000-0000-000000000001";desired.chestId="0123456789abcdef";desired.capturedAt=100;desired.totalSlots=4;desired.items={{0,100,100,"ITEM_AshLogs"},{1,100,100,"ITEM_AshPlanks"}};
    auto plan=PlanStorageAutofill(desired,{{0,50,100,"ITEM_AshLogs"},{1,1,1,"ITEM_PlayerWeapon"}});assert(plan.kind==StorageAutofillPlanKind::Add&&plan.key=="ITEM_AshLogs"&&plan.quantity==50);
    plan=PlanStorageAutofill(desired,{{0,100,100,"ITEM_AshLogs"},{1,1,1,"ITEM_PlayerWeapon"}});assert(plan.kind==StorageAutofillPlanKind::None);
    plan=PlanStorageAutofill(desired,{{0,1,1,"ITEM_PlayerWeapon"},{1,1,1,"ITEM_PlayerArmour"},{2,1,1,"ITEM_Other"},{3,1,1,"ITEM_Other2"}});assert(plan.kind==StorageAutofillPlanKind::None);
    plan=PlanStorageAutofill(desired,{{0,125,100,"ITEM_AshLogs"},{1,100,100,"ITEM_AshPlanks"}});assert(plan.kind==StorageAutofillPlanKind::None);
    StorageAutofillTemplate tea;tea.ownerGuid=desired.ownerGuid;tea.chestId=desired.chestId;tea.capturedAt=100;tea.totalSlots=1;tea.items={{0,49,100,"ITEM_StrongSweetTea"}};
    plan=PlanStorageAutofill(tea,{{0,50,100,"ITEM_StrongSweetTea"}});assert(plan.kind==StorageAutofillPlanKind::None);
    plan=PlanStorageAutofill(tea,{{0,48,100,"ITEM_StrongSweetTea"}});assert(plan.kind==StorageAutofillPlanKind::Add&&plan.quantity==1);
    plan=PlanStorageAutofill(desired,{});assert(plan.kind==StorageAutofillPlanKind::Add&&plan.key=="ITEM_AshLogs"&&plan.quantity==100);
    plan=PlanStorageAutofill(desired,{{0,100,100,"ITEM_AshLogs"}});assert(plan.kind==StorageAutofillPlanKind::Add&&plan.key=="ITEM_AshPlanks"&&plan.quantity==100);
    StorageAutofillTemplate building;building.ownerGuid=desired.ownerGuid;building.chestId=desired.chestId;building.capturedAt=100;building.totalSlots=3;building.items={{0,32,100,"ITEM_Stone"},{1,20,100,"ITEM_RuneEssence"},{2,1,10,"ITEM_VaultCore"}};
    plan=PlanStorageAutofill(building,{});assert(plan.kind==StorageAutofillPlanKind::Add&&plan.key=="ITEM_Stone"&&plan.quantity==32);
    plan=PlanStorageAutofill(building,{{0,32,100,"ITEM_Stone"}});assert(plan.kind==StorageAutofillPlanKind::Add&&plan.key=="ITEM_RuneEssence"&&plan.quantity==20);
    plan=PlanStorageAutofill(building,{{0,32,100,"ITEM_Stone"},{1,20,100,"ITEM_RuneEssence"}});assert(plan.kind==StorageAutofillPlanKind::Add&&plan.key=="ITEM_VaultCore"&&plan.quantity==1);
    plan=PlanStorageAutofill(building,{{0,32,100,"ITEM_Stone"},{1,20,100,"ITEM_RuneEssence"},{2,1,10,"ITEM_VaultCore"}});assert(plan.kind==StorageAutofillPlanKind::None);
    StorageAutofillTemplate berries;berries.ownerGuid=desired.ownerGuid;berries.chestId=desired.chestId;berries.capturedAt=100;berries.totalSlots=2;berries.items={{0,99,100,"ITEM_Redberries"},{1,97,100,"ITEM_Redberries"}};
    plan=PlanStorageAutofill(berries,{{1,97,100,"ITEM_Redberries"}});assert(plan.kind==StorageAutofillPlanKind::Add&&plan.quantity==99);
    plan=PlanStorageAutofill(berries,{{0,97,100,"ITEM_Redberries"},{1,99,100,"ITEM_Redberries"}});assert(plan.kind==StorageAutofillPlanKind::Add&&plan.quantity==2);
    plan=PlanStorageAutofill(berries,{{0,99,100,"ITEM_Redberries"},{1,99,100,"ITEM_Redberries"}});assert(plan.kind==StorageAutofillPlanKind::None);
    auto berriesWithOther=berries;berriesWithOther.totalSlots=3;berriesWithOther.items.push_back({2,1,1,"ITEM_Other"});plan=PlanStorageAutofill(berriesWithOther,{{0,99,100,"ITEM_Redberries"},{1,99,100,"ITEM_Redberries"}});assert(plan.kind==StorageAutofillPlanKind::Add&&plan.key=="ITEM_Other"&&plan.quantity==1);
    plan=PlanStorageAutofill(berriesWithOther,{{0,99,100,"ITEM_Redberries"},{1,99,100,"ITEM_Redberries"},{2,2,1,"ITEM_Other"}});assert(plan.kind==StorageAutofillPlanKind::None);
    plan=PlanStorageAutofill(berries,{{0,1,1,"ITEM_PlayerWeapon"},{1,97,100,"ITEM_Redberries"}});assert(plan.kind==StorageAutofillPlanKind::None);
    auto relocation=PlanStorageAutofillRelocation(berries.items,{{0,50,100,"ITEM_Redberries"},{1,97,100,"ITEM_Redberries"},{2,49,100,"ITEM_Redberries"}},"ITEM_Redberries");assert(relocation.available&&relocation.sourceSlot==2&&relocation.destinationSlot==0&&relocation.quantity==49);
    relocation=PlanStorageAutofillRelocation(berries.items,{{1,97,100,"ITEM_Redberries"},{2,99,100,"ITEM_Redberries"}},"ITEM_Redberries");assert(relocation.available&&relocation.sourceSlot==2&&relocation.destinationSlot==0&&relocation.quantity==99);
    relocation=PlanStorageAutofillRelocation(berries.items,{{0,1,1,"ITEM_PlayerWeapon"},{1,97,100,"ITEM_Redberries"},{2,99,100,"ITEM_Redberries"}},"ITEM_Redberries");assert(!relocation.available);
    auto selective=desired;assert(ApplyStorageAutofillSelection(selective,{{2,50,100,"ITEM_OakLogs"}},true));assert(selective.items.size()==3&&selective.items[0].key=="ITEM_AshLogs"&&selective.items[2].key=="ITEM_OakLogs");assert(ApplyStorageAutofillSelection(selective,{{0,100,100,"ITEM_AshLogs"}},false));assert(selective.items.size()==2&&selective.items[0].key=="ITEM_AshPlanks"&&selective.items[1].key=="ITEM_OakLogs");assert(!ApplyStorageAutofillSelection(selective,{{2,50,100,"ITEM_OakLogs"},{2,40,100,"ITEM_OakLogs"}},true));assert(selective.items.size()==2);

    char directory[]="/tmp/zanik-storage-autofill-XXXXXX";assert(mkdtemp(directory));const std::string path=std::string(directory)+"/storage-autofill.db";
    StorageAutofillStore store(path);assert(store.Load());assert(store.Replace(desired));StorageAutofillTemplate loaded;assert(store.Find(desired.ownerGuid,desired.chestId,loaded));assert(loaded.items.size()==2&&loaded.items[0].key=="ITEM_AshLogs");struct stat file{};assert(stat(path.c_str(),&file)==0&&(file.st_mode&0077)==0);
    StorageAutofillStore restarted(path);assert(restarted.Load());assert(restarted.Find(desired.ownerGuid,desired.chestId,loaded));assert(restarted.Diagnostics().templates==1);
    StorageAutofillTemplate runes; runes.ownerGuid=desired.ownerGuid;runes.chestId="0000000000000003";runes.capturedAt=200;runes.totalSlots=32;runes.items={{0,250,1000,"ITEM_LawRune"},{4,100,1000,"ITEM_NatureRune"}};assert(restarted.Replace(runes));
    StorageAutofillTemplate personal;personal.ownerGuid=desired.ownerGuid;personal.chestId="0000000000000005";personal.capturedAt=201;personal.totalSlots=32;personal.items={{0,99,100,"ITEM_Stone"},{5,9,100,"ITEM_VaultCore"}};assert(restarted.Replace(personal));
    StorageAutofillStore playerRestart(path);assert(playerRestart.Load());assert(playerRestart.Find(runes.ownerGuid,runes.chestId,loaded));assert(loaded.items.size()==2&&loaded.items[0].slot==0&&loaded.items[1].slot==4);
    plan=PlanStorageAutofill(loaded,{{0,1,1,"ITEM_Foreign"},{4,100,1000,"ITEM_NatureRune"}});assert(plan.kind==StorageAutofillPlanKind::None);
    plan=PlanStorageAutofill(loaded,{{0,200,1000,"ITEM_LawRune"},{4,100,1000,"ITEM_NatureRune"}});assert(plan.kind==StorageAutofillPlanKind::Add&&plan.key=="ITEM_LawRune"&&plan.quantity==50);
    plan=PlanStorageAutofill(loaded,{{4,100,1000,"ITEM_NatureRune"}});assert(plan.kind==StorageAutofillPlanKind::Add&&plan.key=="ITEM_LawRune"&&plan.quantity==250);
    assert(playerRestart.Remove(runes.ownerGuid,runes.chestId));
    bool transferred=false;const std::string newOwner="00000000000000000000000000000002",newChest="fedcba9876543210";assert(playerRestart.Transfer(desired.ownerGuid,desired.chestId,newOwner,newChest,transferred)&&transferred);assert(!playerRestart.Find(desired.ownerGuid,desired.chestId,loaded));assert(playerRestart.Find(newOwner,newChest,loaded)&&loaded.ownerGuid==newOwner&&loaded.chestId==newChest&&loaded.items.size()==2);StorageAutofillStore transferredRestart(path);assert(transferredRestart.Load()&&transferredRestart.Find(newOwner,newChest,loaded));transferred=false;assert(transferredRestart.Transfer(desired.ownerGuid,desired.chestId,newOwner,newChest,transferred)&&!transferred);assert(transferredRestart.Remove(newOwner,newChest));assert(!transferredRestart.Find(newOwner,newChest,loaded));
    {std::ofstream corrupt(path,std::ios::trunc);corrupt<<"# zanik-storage-autofill\t1\nT\tbroken\n";}StorageAutofillStore malformed(path);assert(!malformed.Load());const auto diagnostics=malformed.Diagnostics();assert(!diagnostics.loaded&&diagnostics.malformedRecords==1&&diagnostics.templates==0);
    unlink(path.c_str());rmdir(directory);
}
