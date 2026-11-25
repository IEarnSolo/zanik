#include "player_settings_store.h"

#include <cassert>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>

int main(){
    char directory[]="/tmp/zanik-player-settings-XXXXXX";assert(mkdtemp(directory));const std::string path=std::string(directory)+"/player-settings.db",first="00000000-0000-0000-0000-000000000001",second="00000000-0000-0000-0000-000000000002";
    PlayerSettingsStore store(path);assert(store.Load());assert(!store.FreeBuilding(first));assert(store.SetFreeBuilding(first,true));assert(store.FreeBuilding(first));assert(store.SetFreeBuilding(second,true));assert(store.SetFreeBuilding(first,false));assert(!store.FreeBuilding(first)&&store.FreeBuilding(second));struct stat file{};assert(stat(path.c_str(),&file)==0&&(file.st_mode&0077)==0);
    PlayerSettingsStore restarted(path);assert(restarted.Load());assert(!restarted.FreeBuilding(first)&&restarted.FreeBuilding(second));assert(restarted.Diagnostics().freeBuildingCharacters==1);
    {std::ofstream corrupt(path,std::ios::trunc);corrupt<<"# zanik-player-settings\t1\ninvalid\n";}PlayerSettingsStore malformed(path);assert(!malformed.Load());assert(malformed.Diagnostics().malformedRecords==1);unlink(path.c_str());rmdir(directory);
}
