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
    auto claim=store.CreateClaim("player-public-id","character-guid","Test Character");assert(claim.size()==64);
    std::string token;AdminSession issued;assert(store.RedeemClaim(claim,token,issued));assert(!store.RedeemClaim(claim,token,issued));
    AdminSession session;assert(store.Authenticate(token,session));assert(session.publicId=="player-public-id");
    struct stat st{};assert(stat(db.c_str(),&st)==0);assert((st.st_mode&0777)==0600);
    auto health=store.Diagnostics();assert(health.allowlistReadable&&health.sessionStorageReadable&&health.sessionStorageWritable&&health.sessionStorageRestricted&&health.lastSaveSucceeded);assert(health.allowedCharacters==1&&health.activeSessions==1&&health.malformedSessionRecords==0);
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
