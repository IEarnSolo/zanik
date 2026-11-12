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
    StorageAutofillTemplate berries;berries.ownerGuid=desired.ownerGuid;berries.chestId=desired.chestId;berries.capturedAt=100;berries.totalSlots=2;berries.items={{0,99,100,"ITEM_Redberries"},{1,97,100,"ITEM_Redberries"}};
    plan=PlanStorageAutofill(berries,{{1,97,100,"ITEM_Redberries"}});assert(plan.kind==StorageAutofillPlanKind::Add&&plan.quantity==99);
    plan=PlanStorageAutofill(berries,{{0,97,100,"ITEM_Redberries"},{1,99,100,"ITEM_Redberries"}});assert(plan.kind==StorageAutofillPlanKind::Move&&plan.quantity==2&&plan.sourceSlot==1&&plan.destinationSlot==0);
    plan=PlanStorageAutofill(berries,{{0,99,100,"ITEM_Redberries"},{1,99,100,"ITEM_Redberries"}});assert(plan.kind==StorageAutofillPlanKind::Set&&plan.quantity==97&&plan.sourceSlot==1);
    auto berriesWithOther=berries;berriesWithOther.totalSlots=3;berriesWithOther.items.push_back({2,1,1,"ITEM_Other"});plan=PlanStorageAutofill(berriesWithOther,{{0,99,100,"ITEM_Redberries"},{1,99,100,"ITEM_Redberries"}});assert(plan.kind==StorageAutofillPlanKind::Set&&plan.sourceSlot==1);
    plan=PlanStorageAutofill(berriesWithOther,{{0,99,100,"ITEM_Redberries"},{1,99,100,"ITEM_Redberries"},{2,2,1,"ITEM_Other"}});assert(plan.kind==StorageAutofillPlanKind::Set&&plan.sourceSlot==1);
    plan=PlanStorageAutofill(berries,{{0,1,1,"ITEM_PlayerWeapon"},{1,97,100,"ITEM_Redberries"}});assert(plan.kind==StorageAutofillPlanKind::None);

    char directory[]="/tmp/zanik-storage-autofill-XXXXXX";assert(mkdtemp(directory));const std::string path=std::string(directory)+"/storage-autofill.db";
    StorageAutofillStore store(path);assert(store.Load());assert(store.Replace(desired));StorageAutofillTemplate loaded;assert(store.Find(desired.ownerGuid,desired.chestId,loaded));assert(loaded.items.size()==2&&loaded.items[0].key=="ITEM_AshLogs");struct stat file{};assert(stat(path.c_str(),&file)==0&&(file.st_mode&0077)==0);
    StorageAutofillStore restarted(path);assert(restarted.Load());assert(restarted.Find(desired.ownerGuid,desired.chestId,loaded));assert(restarted.Diagnostics().templates==1);assert(restarted.Remove(desired.ownerGuid,desired.chestId));assert(!restarted.Find(desired.ownerGuid,desired.chestId,loaded));
    {std::ofstream corrupt(path,std::ios::trunc);corrupt<<"# zanik-storage-autofill\t1\nT\tbroken\n";}StorageAutofillStore malformed(path);assert(!malformed.Load());const auto diagnostics=malformed.Diagnostics();assert(!diagnostics.loaded&&diagnostics.malformedRecords==1&&diagnostics.templates==0);
    unlink(path.c_str());rmdir(directory);
}
