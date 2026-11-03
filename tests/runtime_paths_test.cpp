#include "utils/runtime_paths.h"

#include <cassert>
#include <cstdlib>

int main() {
    unsetenv("DW_SERVER_ROOT");
    unsetenv("DW_ZANIK_ROOT");
    unsetenv("DW_DATA_ROOT");
    unsetenv("DW_WEB_ROOT");
    assert(RuntimePaths::ServerRoot()=="/home/container");

    setenv("DW_SERVER_ROOT","/srv/dragonwilds",1);
    assert(RuntimePaths::ZanikRoot()=="/srv/dragonwilds/zanik");
    assert(RuntimePaths::DataFile("runtime.log")=="/srv/dragonwilds/zanik-data/runtime.log");
    assert(RuntimePaths::WebFile("grave-items.tsv")=="/srv/dragonwilds/zanik/assets/current/grave-items.tsv");
    assert(RuntimePaths::GameSavedRoot()=="/srv/dragonwilds/RSDragonwilds/Saved");

    setenv("DW_DATA_ROOT","/data/zanik",1);
    setenv("DW_WEB_ROOT","/assets/current",1);
    assert(RuntimePaths::DataFile("players.db")=="/data/zanik/players.db");
    assert(RuntimePaths::WebFile("catalog.tsv")=="/assets/current/catalog.tsv");
}
