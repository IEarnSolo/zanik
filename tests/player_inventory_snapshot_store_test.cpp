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
    PlayerInventorySnapshotContainer inventory;inventory.id="0000000000000001";inventory.totalSlots=40;inventory.hotbarStart=0;inventory.hotbarCount=8;inventory.items.push_back({"opaque-item","ITEM_Test","Test item","/assets/items/ITEM_Test.png",2,7,100,0,0,true,true,false});desired.containers.push_back(inventory);
    PlayerInventorySnapshotContainer equipment;equipment.id="0000000000000002";equipment.totalSlots=8;equipment.hotbarStart=-1;equipment.items.push_back({"opaque-equipment","ITEM_Amulet","Amulet of testing","/assets/items/ITEM_Amulet.png",5,1,1,6,9,true,true,true});desired.containers.push_back(equipment);
    PlayerInventorySnapshotContainer personal;personal.id="0000000000000005";personal.totalSlots=32;personal.hotbarStart=-1;personal.items.push_back({"opaque-personal","ITEM_Stone","Stone","/assets/items/ITEM_Stone.png",0,99,100,0,0,false,true,false});desired.containers.push_back(personal);
    PlayerInventorySnapshotStore store(path);assert(store.Load());assert(store.Replace(desired));struct stat file{};assert(stat(path.c_str(),&file)==0&&(file.st_mode&0077)==0);
    PlayerInventoryDisplaySnapshot loaded;assert(store.Find(desired.characterGuid,loaded));assert(loaded.capturedAt==desired.capturedAt&&loaded.containers.size()==3&&loaded.containers[0].items[0].quantity==7);
    PlayerInventorySnapshotStore restarted(path);assert(restarted.Load());assert(restarted.Find(desired.characterGuid,loaded));assert(loaded.containers[1].items[0].name=="Amulet of testing"&&loaded.containers[1].items[0].basePowerLevel==6&&loaded.containers[1].items[0].powerLevel==9&&loaded.containers[1].items[0].masterwork);
    desired.capturedAt+=120;desired.containers[0].items[0].quantity=6;assert(restarted.Replace(desired));PlayerInventorySnapshotStore changed(path);assert(changed.Load()&&changed.Find(desired.characterGuid,loaded)&&loaded.containers[0].items[0].quantity==6&&loaded.capturedAt==desired.capturedAt);
    const std::string legacyPath=std::string(directory)+"/legacy.db";{std::ofstream legacy(legacyPath);legacy<<"# zanik-player-inventory-snapshots\t1\nP\t3030303030303030303030303030303030303030303030303030303030303031\t1700000000\t1\nC\t0000000000000001\t40\t0\t8\t1\nI\t2\t7\t100\t1\t1\t6f70617175652d6974656d\t4954454d5f54657374\t54657374206974656d\t2f6173736574732f6974656d732f4954454d5f546573742e706e67\n";}PlayerInventorySnapshotStore legacy(legacyPath);assert(legacy.Load());PlayerInventoryDisplaySnapshot migrated;assert(legacy.Find("00000000000000000000000000000001",migrated));assert(migrated.containers[0].items[0].powerLevel==0&&!migrated.containers[0].items[0].masterwork);unlink(legacyPath.c_str());
    {std::ofstream corrupt(path,std::ios::trunc);corrupt<<"# zanik-player-inventory-snapshots\t1\nP\tbroken\n";}PlayerInventorySnapshotStore malformed(path);assert(!malformed.Load());const auto diagnostics=malformed.Diagnostics();assert(!diagnostics.loaded&&diagnostics.malformedRecords==1&&diagnostics.snapshots==0);
    unlink(path.c_str());rmdir(directory);return 0;
}
