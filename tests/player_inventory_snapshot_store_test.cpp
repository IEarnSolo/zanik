#include "player_inventory_snapshot_store.h"

#include <cassert>
#include <cstdlib>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

int main(){
    char directory[]="/tmp/zanik-player-inventory-snapshot-XXXXXX";assert(mkdtemp(directory));const std::string path=std::string(directory)+"/player-inventory-snapshots.db";
    PlayerInventoryDisplaySnapshot desired;desired.characterGuid="00000000000000000000000000000001";desired.capturedAt=1700000000;
    PlayerInventorySnapshotContainer inventory;inventory.id="0000000000000001";inventory.totalSlots=40;inventory.hotbarStart=0;inventory.hotbarCount=8;inventory.items.push_back({"opaque-item","ITEM_Test","Test item","/assets/items/ITEM_Test.png",2,7,100,true,true});desired.containers.push_back(inventory);
    PlayerInventorySnapshotContainer equipment;equipment.id="0000000000000002";equipment.totalSlots=8;equipment.hotbarStart=-1;equipment.items.push_back({"opaque-equipment","ITEM_Amulet","Amulet of testing","/assets/items/ITEM_Amulet.png",5,1,1,true,true});desired.containers.push_back(equipment);
    PlayerInventorySnapshotStore store(path);assert(store.Load());assert(store.Replace(desired));struct stat file{};assert(stat(path.c_str(),&file)==0&&(file.st_mode&0077)==0);
    PlayerInventoryDisplaySnapshot loaded;assert(store.Find(desired.characterGuid,loaded));assert(loaded.capturedAt==desired.capturedAt&&loaded.containers.size()==2&&loaded.containers[0].items[0].quantity==7);
    PlayerInventorySnapshotStore restarted(path);assert(restarted.Load());assert(restarted.Find(desired.characterGuid,loaded));assert(loaded.containers[1].items[0].name=="Amulet of testing");
    desired.capturedAt+=120;desired.containers[0].items[0].quantity=6;assert(restarted.Replace(desired));PlayerInventorySnapshotStore changed(path);assert(changed.Load()&&changed.Find(desired.characterGuid,loaded)&&loaded.containers[0].items[0].quantity==6&&loaded.capturedAt==desired.capturedAt);
    {std::ofstream corrupt(path,std::ios::trunc);corrupt<<"# zanik-player-inventory-snapshots\t1\nP\tbroken\n";}PlayerInventorySnapshotStore malformed(path);assert(!malformed.Load());const auto diagnostics=malformed.Diagnostics();assert(!diagnostics.loaded&&diagnostics.malformedRecords==1&&diagnostics.snapshots==0);
    unlink(path.c_str());rmdir(directory);return 0;
}
