#include "admin_store.h"
#include <cassert>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>

int main(){
    char dir[]="/tmp/zanik-admin-test-XXXXXX";assert(mkdtemp(dir));
    std::string allow=std::string(dir)+"/admins.txt",db=std::string(dir)+"/sessions.db";
    {std::ofstream out(allow);out<<"# test\nplayer-public-id\n";}
    AdminStore store(allow,db);assert(store.Load());assert(store.IsAllowed("player-public-id"));
    assert(store.Authorize("0123456789abcdef0123456789abcdef","Second # Character\nIgnored"));assert(store.IsAllowed("0123456789abcdef0123456789abcdef"));assert(!store.Authorize("not-an-id","Bad"));
    auto claim=store.CreateClaim("player-public-id","character-guid","Test Character");assert(claim.size()==64);
    std::string token;AdminSession issued;assert(store.RedeemClaim(claim,token,issued));assert(!store.RedeemClaim(claim,token,issued));
    AdminSession session;assert(store.Authenticate(token,session));assert(session.publicId=="player-public-id");
    const std::string firstPrefix=session.sessionHashPrefix;
    auto parallelClaim=store.CreateClaim("player-public-id","character-guid","Test Character");assert(!parallelClaim.empty());
    std::string parallelToken;AdminSession parallelSession;assert(store.RedeemClaim(parallelClaim,parallelToken,parallelSession));assert(store.Authenticate(parallelToken,parallelSession));
    auto active=store.ActiveSessions();assert(active.size()==2);assert(active[0].hashPrefix.size()==12&&active[0].characterName=="Test Character");
    assert(store.RevokeSession(parallelSession.sessionHashPrefix));assert(!store.Authenticate(parallelToken,parallelSession));assert(store.Authenticate(token,session)&&session.sessionHashPrefix==firstPrefix);assert(!store.RevokeSession("not-a-prefix"));
    struct stat st{};assert(stat(db.c_str(),&st)==0);assert((st.st_mode&0777)==0600);
    auto health=store.Diagnostics();assert(health.allowlistReadable&&health.sessionStorageReadable&&health.sessionStorageWritable&&health.sessionStorageRestricted&&health.lastSaveSucceeded);assert(health.allowedCharacters==2&&health.activeSessions==1&&health.malformedSessionRecords==0);
    AdminStore reloaded(allow,db);assert(reloaded.Load());assert(reloaded.Authenticate(token,session));
    reloaded.Audit(session,"safe-destination","accepted");
    {std::ifstream audit(std::string(dir)+"/admin-audit.log");std::string contents((std::istreambuf_iterator<char>(audit)),{});assert(contents.find(token)==std::string::npos);assert(contents.find(claim)==std::string::npos);}
    usleep(1100000);
    {std::ofstream out(allow);out<<"# removed\n";}
    bool removed=false;assert(!reloaded.Authenticate(token,session,&removed));assert(removed);
    usleep(1100000);
    {std::ofstream out(allow);out<<"player-public-id\n";}
    auto secondClaim=reloaded.CreateClaim("player-public-id","character-guid","Test Character");assert(!secondClaim.empty());
    std::string secondToken;assert(reloaded.RedeemClaim(secondClaim,secondToken,session));
    assert(reloaded.Logout(secondToken));assert(!reloaded.Authenticate(secondToken,session));
    return 0;
}
