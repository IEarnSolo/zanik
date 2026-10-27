#include "engine/dom_engine.h"
#include "feature_config.h"
#include "status.h"
#include "player_store.h"
#include "admin_store.h"
#include "recipe_settings_store.h"
#include "recipe_snapshot_store.h"
#include "quest_snapshot_store.h"
#include "player_activity_store.h"
#include "utils/logger.h"
#include "utils/memory.h"
#include "web_page.h"

#include <arpa/inet.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <string>
#include <memory>
#include <fstream>
#include <sstream>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>
#include <unistd.h>

namespace {
std::atomic<bool> stopping{false};
std::thread worker;
std::unique_ptr<PlayerStore> playerStore;
std::unique_ptr<AdminStore> adminStore;
std::unique_ptr<RecipeSettingsStore> recipeSettingsStore;
std::unique_ptr<RecipeSnapshotStore> recipeSnapshotStore;
std::unique_ptr<QuestSnapshotStore> questSnapshotStore;
std::unique_ptr<PlayerActivityStore> playerActivityStore;
std::unordered_map<std::string,std::chrono::steady_clock::time_point> adminTeleportRate;
std::unordered_map<std::string,std::chrono::steady_clock::time_point> adminStorageRate;
std::unordered_map<std::string,std::chrono::steady_clock::time_point> adminGravestoneRate;

int Port() {
    const char* raw = std::getenv("DW_STATUS_PORT");
    if (!raw || !*raw) return 0;
    char* end = nullptr;
    long port = std::strtol(raw, &end, 10);
    return *end == '\0' && port >= 1 && port <= 65535 ? static_cast<int>(port) : 0;
}

std::string StorePath() {
    const char* raw = std::getenv("DW_STATUS_DATA");
    return raw && *raw ? raw : "/home/container/zanik-data/players.db";
}

std::string ZanikRoot(){const char* raw=std::getenv("DW_ZANIK_ROOT");return raw&&*raw?raw:"/home/container/zanik";}
std::string DataRoot(){const char* raw=std::getenv("DW_DATA_ROOT");return raw&&*raw?raw:"/home/container/zanik-data";}

std::string WebRoot() {
    const char* raw = std::getenv("DW_WEB_ROOT");
    return raw && *raw ? raw : "/home/container/zanik/web";
}

std::string ConsoleSecret(){const char* raw=std::getenv("DW_CONSOLE_SECRET");return raw?raw:"";}
std::string AdminCookieName(){
    const char* raw=std::getenv("DW_ADMIN_COOKIE_NAME");
    std::string name=raw&&*raw?raw:"zanik_admin";
    if(name.empty()||name.size()>64)return "zanik_admin";
    for(unsigned char c:name)if(!std::isalnum(c)&&c!='_')return "zanik_admin";
    return name;
}
std::string PublicBaseUrl(){const char* raw=std::getenv("DW_PUBLIC_BASE_URL");return raw&&*raw?raw:"http://127.0.0.1:"+std::to_string(Port());}
std::string PersistentAdminCookie(const std::string& raw){
    // Admin sessions are revoked explicitly or by removing the character from
    // admins.txt.  Give the browser a persistent cookie as well; without
    // Max-Age, iOS Safari is free to discard it when the browsing session is
    // suspended or terminated.
    return "Set-Cookie: "+AdminCookieName()+"="+raw+"; HttpOnly; SameSite=Strict; Path=/; Max-Age=315360000\r\n";
}

bool ReadWebAsset(const std::string& name,std::string& body) {
    if(name.empty()||name.find("..")!=std::string::npos||name.find('/')!=std::string::npos)return false;
    std::ifstream input(WebRoot()+"/"+name,std::ios::binary);
    if(!input)return false;
    body.assign(std::istreambuf_iterator<char>(input),{});
    return body.size()<=32*1024*1024;
}
bool ReadQuestAsset(const std::string&name,std::string&body){if(name.empty()||name.size()>100||name.find("..")!=std::string::npos||name.find('/')!=std::string::npos)return false;std::ifstream in(WebRoot()+"/quests/"+name,std::ios::binary);if(!in)return false;body.assign(std::istreambuf_iterator<char>(in),{});return body.size()<=2*1024*1024;}

bool RecipeCatalogUnknownIds(const std::string& snapshotFingerprint,const std::string& currentFingerprint,
                             std::vector<std::string>& unknown,
                             std::unordered_set<std::string>* currentOut=nullptr,bool* removedOut=nullptr){
    unknown.clear();if(currentOut)currentOut->clear();if(removedOut)*removedOut=false;if(snapshotFingerprint==currentFingerprint)return true;
    std::ifstream history(WebRoot()+"/recipe-runtime-catalog-history.tsv");if(!history)return false;
    std::unordered_set<std::string> previous;std::string line;
    while(std::getline(history,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();std::istringstream row(line);std::string marker,fingerprint,ids;if(!std::getline(row,marker,'\t')||marker!="C"||!std::getline(row,fingerprint,'\t')||!std::getline(row,ids))continue;if(fingerprint!=snapshotFingerprint)continue;size_t start=0;while(start<=ids.size()){size_t at=ids.find(',',start);std::string id=ids.substr(start,at==std::string::npos?at:at-start);if(id.size()==16)previous.insert(id);if(at==std::string::npos)break;start=at+1;}}
    if(previous.empty())return false;
    std::ifstream current(WebRoot()+"/recipe-runtime-catalog.tsv");if(!current)return false;std::unordered_set<std::string> currentIds;
    while(std::getline(current,line)){if(line.empty()||line[0]=='#')continue;size_t at=line.find('\t');std::string id=line.substr(0,at);if(id.size()==16)currentIds.insert(id);}
    if(currentIds.empty())return false;
    bool removed=false;
    for(const auto&id:previous)removed=removed||!currentIds.count(id);
    for(const auto&id:currentIds)if(!previous.count(id))unknown.push_back(id);
    std::sort(unknown.begin(),unknown.end());
    if(currentOut)*currentOut=currentIds;
    if(removedOut)*removedOut=removed;
    return true;
}

int RecipeCatalogHistoryCompatibility(){
    const std::string fingerprint=DomEngine::GetPlayerRecipeCatalogFingerprint();if(fingerprint.empty())return 0;
    std::ifstream history(WebRoot()+"/recipe-runtime-catalog-history.tsv");if(!history)return -1;std::string line;
    while(std::getline(history,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();std::istringstream row(line);std::string marker,found,ids;if(!std::getline(row,marker,'\t')||marker!="C"||!std::getline(row,found,'\t')||!std::getline(row,ids)||found!=fingerprint)continue;size_t count=0,start=0;while(start<=ids.size()){size_t at=ids.find(',',start);if(ids.substr(start,at==std::string::npos?at:at-start).size()!=16)return -1;++count;if(at==std::string::npos)break;start=at+1;}return count==DomEngine::GetPlayerRecipeDiagnostics().catalogEntries?1:-1;}return -1;
}

bool ReadResourceAsset(const std::string& name,std::string& body) {
    if(name.empty()||name.size()>100)return false;
    for(char c:name)if(!(std::isalnum(static_cast<unsigned char>(c))||c=='-'||c=='.'))return false;
    std::ifstream input(WebRoot()+"/resources/"+name,std::ios::binary);if(!input)return false;
    body.assign(std::istreambuf_iterator<char>(input),{});return body.size()<=2*1024*1024;
}

bool ReadItemAsset(const std::string& name,std::string& body) {
    if(name.empty()||name.size()>160)return false;
    for(char c:name)if(!(std::isalnum(static_cast<unsigned char>(c))||c=='-'||c=='_'||c=='.'))return false;
    std::ifstream input(WebRoot()+"/items/"+name,std::ios::binary);if(!input)return false;
    body.assign(std::istreambuf_iterator<char>(input),{});return body.size()<=2*1024*1024;
}

bool ReadSkillAsset(const std::string& name,std::string& body) {
    if(name.empty()||name.size()>80)return false;
    for(char c:name)if(!(std::isalnum(static_cast<unsigned char>(c))||c=='-'||c=='_'||c=='.'))return false;
    std::ifstream input(WebRoot()+"/skills/"+name,std::ios::binary);if(!input)return false;
    body.assign(std::istreambuf_iterator<char>(input),{});return body.size()<=2*1024*1024;
}

bool ReadBuildingAsset(const std::string& name,std::string& body) {
    if(name.empty()||name.size()>160)return false;
    for(char c:name)if(!(std::isalnum(static_cast<unsigned char>(c))||c=='-'||c=='_'||c=='.'))return false;
    std::ifstream input(WebRoot()+"/buildings/"+name,std::ios::binary);if(!input)return false;
    body.assign(std::istreambuf_iterator<char>(input),{});return body.size()<=2*1024*1024;
}

bool ReadNpcAsset(const std::string& name,std::string& body) {
    if(name.empty()||name.size()>160)return false;
    for(char c:name)if(!(std::isalnum(static_cast<unsigned char>(c))||c=='-'||c=='_'||c=='.'))return false;
    std::ifstream input(WebRoot()+"/npcs/"+name,std::ios::binary);if(!input)return false;
    body.assign(std::istreambuf_iterator<char>(input),{});return body.size()<=2*1024*1024;
}

bool ReadRecipeIconAsset(const std::string& name,std::string& body) {
    if(name.size()!=20||name.substr(name.size()-4)!=".png")return false;
    for(char c:name)if(!(std::isalnum(static_cast<unsigned char>(c))||c=='.'))return false;
    std::ifstream input(WebRoot()+"/recipe-icons/"+name,std::ios::binary);if(!input)return false;
    body.assign(std::istreambuf_iterator<char>(input),{});return body.size()<=2*1024*1024;
}

struct NpcIconHealth { int compatibility=0,total=0,missing=0; };
NpcIconHealth ReadNpcIconHealth() {
    NpcIconHealth health;std::string catalog;if(!ReadWebAsset("npc-catalog.json",catalog))return health;
    const std::string prefix="\"icon\":\"/assets/npcs/";std::unordered_set<std::string> names;size_t cursor=0;
    while((cursor=catalog.find(prefix,cursor))!=std::string::npos){cursor+=prefix.size();size_t end=catalog.find('"',cursor);if(end==std::string::npos)break;names.insert(catalog.substr(cursor,end-cursor));cursor=end+1;}
    health.total=static_cast<int>(names.size());
    for(const auto& name:names){
        if(name.empty()||name.size()>160){++health.missing;continue;}
        bool valid=true;for(char c:name)if(!(std::isalnum(static_cast<unsigned char>(c))||c=='-'||c=='_'||c=='.')){valid=false;break;}
        std::ifstream input(WebRoot()+"/npcs/"+name,std::ios::binary|std::ios::ate);
        if(!valid||!input||input.tellg()<=0||input.tellg()>2*1024*1024)++health.missing;
    }
    health.compatibility=health.total>0&&health.missing==0?1:-1;return health;
}

struct SkillIconHealth { int compatibility=0,total=12,missing=0; };
SkillIconHealth ReadSkillIconHealth() {
    static constexpr std::array<const char*,12> names={"agility.png","artisan.png","attack.png","construction.png","cooking.png","farming.png","fishing.png","magic.png","mining.png","ranged.png","runecrafting.png","woodcutting.png"};
    SkillIconHealth health;
    for(const char* name:names){std::ifstream input(WebRoot()+"/skills/"+name,std::ios::binary|std::ios::ate);if(!input||input.tellg()<=0||input.tellg()>2*1024*1024)++health.missing;}
    health.compatibility=health.missing==0?1:-1;return health;
}

struct RecipeCatalogHealth { int compatibility=0,recipes=0,dependencyEdges=0,cycles=0,unknownTriggers=0,generated=0; };
RecipeCatalogHealth ReadRecipeCatalogHealth(std::string* body=nullptr) {
    RecipeCatalogHealth health;std::string catalog;if(!ReadWebAsset("recipe-unlocks.json",catalog))return health;
    auto number=[&](const char* key){const std::string needle=std::string("\"")+key+"\":";size_t at=catalog.find(needle);if(at==std::string::npos)return -1;at+=needle.size();int value=0;bool any=false;while(at<catalog.size()&&std::isdigit(static_cast<unsigned char>(catalog[at]))){any=true;value=value*10+(catalog[at++]-'0');}return any?value:-1;};
    health.recipes=number("recipes");health.dependencyEdges=number("dependencyEdges");health.cycles=number("cycles");health.unknownTriggers=number("unknownTriggers");if(health.unknownTriggers<0)health.unknownTriggers=0;
    health.generated=catalog.find("\"generatedAt\":\"")!=std::string::npos?1:0;
    size_t ids=0,cursor=0;while((cursor=catalog.find("\"id\":\"",cursor))!=std::string::npos){++ids;cursor+=6;}
    const bool safe=catalog.find("\"persistentId\"")==std::string::npos&&catalog.find("\"internalName\"")==std::string::npos;
    // Exact catalog counts are game-build data and are enforced by the
    // reviewed asset compatibility profile before activation.  At runtime,
    // validate the catalog's self-consistency instead of pinning a second,
    // stale count into the native library.
    health.compatibility=catalog.find("\"schemaVersion\":1")!=std::string::npos&&health.recipes>=100&&ids==static_cast<size_t>(health.recipes)&&health.dependencyEdges>=0&&health.cycles>=0&&health.unknownTriggers>=0&&health.generated==1&&safe?1:-1;
    if(body&&health.compatibility==1)*body=std::move(catalog);
    return health;
}

std::string UpdateStateJson() {
    const char* raw = std::getenv("DW_UPDATE_STATE");
    std::ifstream input(raw && *raw ? raw : "/home/container/zanik/state/update-status.json");
    if (!input) return "null";
    std::ostringstream value;
    value << input.rdbuf();
    std::string json = value.str();
    if (json.size() > 65536 || json.empty() || json.front() != '{' || json.back() != '}') return "null";
    return json;
}

std::string GameUpdateStateJson() {
    const char* raw = std::getenv("DW_GAME_UPDATE_STATE");
    std::ifstream input(raw && *raw ? raw : "/home/container/zanik/state/game-update-status.json");
    if (!input) return "null";
    std::ostringstream value;
    value << input.rdbuf();
    std::string json = value.str();
    while (!json.empty() && std::isspace(static_cast<unsigned char>(json.back()))) json.pop_back();
    if (json.size() > 65536 || json.empty() || json.front() != '{' || json.back() != '}') return "null";
    return json;
}

std::string BackupRetentionStateJson() {
    const char* raw = std::getenv("DW_BACKUP_RETENTION_STATE");
    std::ifstream input(raw && *raw ? raw : "/home/container/zanik/state/backup-retention-status.json");
    if (!input) return "null";
    std::ostringstream value; value << input.rdbuf(); std::string json=value.str();
    while(!json.empty()&&std::isspace(static_cast<unsigned char>(json.back())))json.pop_back();
    if(json.size()>65536||json.empty()||json.front()!='{'||json.back()!='}')return "null";
    return json;
}

std::string AssetManifestJson() {
    const char* raw=std::getenv("DW_ASSET_MANIFEST");
    std::ifstream input(raw&&*raw?raw:(WebRoot()+"/asset-manifest.json"));
    if(!input)return "null";std::ostringstream value;value<<input.rdbuf();std::string json=value.str();
    while(!json.empty()&&std::isspace(static_cast<unsigned char>(json.back())))json.pop_back();
    if(json.size()>1024*1024||json.empty()||json.front()!='{'||json.back()!='}')return "null";return json;
}

const std::string& PlaceholderImage() {
    static const std::string value=R"SVG(<svg xmlns="http://www.w3.org/2000/svg" width="128" height="128" viewBox="0 0 128 128"><rect width="128" height="128" fill="#17221f"/><path d="M20 96 48 62l18 20 16-18 26 32z" fill="#789487"/><circle cx="91" cy="38" r="12" fill="#9ee8c0"/></svg>)SVG";
    return value;
}

void SendAll(int fd, const std::string& bytes) {
    size_t sent = 0;
    while (sent < bytes.size()) {
        ssize_t n = send(fd, bytes.data() + sent, bytes.size() - sent, MSG_NOSIGNAL);
        if (n <= 0) break;
        sent += static_cast<size_t>(n);
    }
}

void Respond(int fd, int status, const char* type, const std::string& body,
             const std::string& extraHeaders={}) {
    const char* reason = status == 200 ? "OK" : status == 400 ? "Bad Request" :
                         status == 401 ? "Unauthorized" : status == 403 ? "Forbidden" :
                         status == 409 ? "Conflict" : status == 429 ? "Too Many Requests" :
                         status == 503 ? "Service Unavailable" :
                         status == 405 ? "Method Not Allowed" : "Not Found";
    std::string response = "HTTP/1.1 " + std::to_string(status) + " " + reason + "\r\n";
    response += "Content-Type: "; response += type; response += "\r\n";
    response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    response += "Cache-Control: no-store\r\n";
    response += "X-Content-Type-Options: nosniff\r\n";
    response += "Content-Security-Policy: default-src 'none'; img-src 'self' data:; script-src 'unsafe-inline'; style-src 'unsafe-inline'; connect-src 'self'\r\n";
    response += extraHeaders;
    response += "Connection: close\r\n\r\n";
    SendAll(fd, response + body);
}

struct HttpRequest { std::string method,target,path,query,body; std::unordered_map<std::string,std::string> headers; };
std::string Lower(std::string s){for(char&c:s)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));return s;}
bool ReadRequest(int fd,HttpRequest&r){
    std::string data;char b[4096];std::size_t headerEnd=std::string::npos,need=0;
    while(data.size()<65536){ssize_t n=recv(fd,b,sizeof(b),0);if(n<=0)return false;data.append(b,n);headerEnd=data.find("\r\n\r\n");if(headerEnd!=std::string::npos)break;}
    if(headerEnd==std::string::npos)return false;std::istringstream in(data.substr(0,headerEnd));std::string line,version;
    if(!std::getline(in,line))return false;if(!line.empty()&&line.back()=='\r')line.pop_back();std::istringstream first(line);if(!(first>>r.method>>r.target>>version))return false;
    while(std::getline(in,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();auto colon=line.find(':');if(colon==std::string::npos)continue;std::string key=Lower(line.substr(0,colon)),value=line.substr(colon+1);auto p=value.find_first_not_of(" \t");r.headers[key]=p==std::string::npos?"":value.substr(p);}
    if(auto it=r.headers.find("content-length");it!=r.headers.end()){try{need=std::stoul(it->second);}catch(...){return false;}if(need>32768)return false;}
    std::size_t bodyAt=headerEnd+4;while(data.size()-bodyAt<need){ssize_t n=recv(fd,b,sizeof(b),0);if(n<=0)return false;data.append(b,n);}r.body=data.substr(bodyAt,need);
    auto q=r.target.find('?');r.path=r.target.substr(0,q);r.query=q==std::string::npos?"":r.target.substr(q+1);return r.path.size()<2048;
}
std::string Cookie(const HttpRequest&r,const std::string&name){auto it=r.headers.find("cookie");if(it==r.headers.end())return {};std::size_t p=0;while(p<=it->second.size()){auto e=it->second.find(';',p);std::string x=it->second.substr(p,e==std::string::npos?e:e-p);auto a=x.find_first_not_of(" \t"),eq=x.find('=',a);if(a!=std::string::npos&&eq!=std::string::npos&&x.substr(a,eq-a)==name)return x.substr(eq+1);if(e==std::string::npos)break;p=e+1;}return {};}
std::string JsonValue(const std::string&body,const std::string&key){std::string mark="\""+key+"\"";auto p=body.find(mark);if(p==std::string::npos)return {};p=body.find(':',p+mark.size());if(p==std::string::npos)return {};p=body.find('"',p+1);if(p==std::string::npos)return {};std::string out;for(++p;p<body.size();++p){if(body[p]=='"')return out;if(body[p]=='\\'&&p+1<body.size()){char c=body[++p];if(c=='"'||c=='\\')out+=c;else return {};}else out+=body[p];if(out.size()>1024)return {};}return {};}
bool JsonInteger(const std::string&body,const std::string&key,int&value){std::string mark="\""+key+"\"";auto p=body.find(mark);if(p==std::string::npos||(p=body.find(':',p+mark.size()))==std::string::npos)return false;++p;while(p<body.size()&&std::isspace(static_cast<unsigned char>(body[p])))++p;auto start=p;if(p<body.size()&&(body[p]=='-'||body[p]=='+'))++p;while(p<body.size()&&std::isdigit(static_cast<unsigned char>(body[p])))++p;if(p==start||(p==start+1&&(body[start]=='-'||body[start]=='+')))return false;try{long long n=std::stoll(body.substr(start,p-start));if(n<1||n>1000000)return false;value=static_cast<int>(n);return true;}catch(...){return false;}}
bool JsonNonNegativeInteger(const std::string&body,const std::string&key,int&value){std::string mark="\""+key+"\"";auto p=body.find(mark);if(p==std::string::npos||(p=body.find(':',p+mark.size()))==std::string::npos)return false;++p;while(p<body.size()&&std::isspace(static_cast<unsigned char>(body[p])))++p;auto start=p;while(p<body.size()&&std::isdigit(static_cast<unsigned char>(body[p])))++p;if(p==start)return false;try{long long n=std::stoll(body.substr(start,p-start));if(n<0||n>511)return false;value=static_cast<int>(n);return true;}catch(...){return false;}}
std::vector<std::string> JsonStringArray(const std::string&body,const std::string&key){std::vector<std::string> out;std::string mark="\""+key+"\"";auto p=body.find(mark);if(p==std::string::npos||(p=body.find('[',p+mark.size()))==std::string::npos)return out;++p;while(p<body.size()){while(p<body.size()&&(std::isspace(static_cast<unsigned char>(body[p]))||body[p]==','))++p;if(p>=body.size()||body[p]==']')return out;if(body[p++]!='\"')return {};std::string value;while(p<body.size()&&body[p]!='\"'){if(body[p]=='\\'&&p+1<body.size()){char c=body[++p];if(c!='\"'&&c!='\\')return {};}value+=body[p++];if(value.size()>64)return {};}if(p>=body.size()||body[p++]!='\"')return {};out.push_back(std::move(value));if(out.size()>128)return {};};return {};}
std::string JsonEscape(const std::string&s){std::string o;for(unsigned char c:s){if(c=='"'||c=='\\'){o+='\\';o+=char(c);}else if(c>=32)o+=char(c);}return o;}

bool MaintenanceMode(){const char* raw=std::getenv("ZANIK_MAINTENANCE_MODE");return raw&&std::strcmp(raw,"1")==0;}
std::string MaintenanceReleaseJson(){
    const char* raw=std::getenv("DW_MAINTENANCE_RELEASE_STATE");
    std::ifstream input(raw&&*raw?raw:"/home/container/zanik/state/maintenance-release.json");
    if(!input)return "null";std::ostringstream value;value<<input.rdbuf();std::string json=value.str();
    while(!json.empty()&&std::isspace(static_cast<unsigned char>(json.back())))json.pop_back();
    if(json.size()>16384||json.empty()||json.front()!='{'||json.back()!='}')return "null";return json;
}
std::string MaintenancePage(){
    const std::string release=MaintenanceReleaseJson();
    const std::string releaseStatus=JsonValue(release,"status");
    const bool available=releaseStatus=="compatible-release-available";
    const bool unavailable=releaseStatus=="no-compatible-release";
    std::string releaseText=available?
        "A compatible stable Zanik release is available. Update Zanik Manager first, then use Update / repair Zanik for this server.":
        unavailable?"No compatible stable Zanik release is available yet. Check back after a new Zanik release is published.":
        "Zanik could not currently verify whether a compatible release is available. You can check GitHub Releases manually.";
    std::string mode=std::getenv("ZANIK_INSTALL_MODE")?std::getenv("ZANIK_INSTALL_MODE"):"unknown";
    std::string build=std::getenv("ZANIK_GAME_BUILD_ID")?std::getenv("ZANIK_GAME_BUILD_ID"):"unknown";
    std::string modeText=mode=="replace-executable"?
        "This server uses replacement mode. Pterodactyl AUTO_UPDATE must remain 0, and Manager needs startup.read and startup.update access to perform a safe game update.":
        "This server uses wrapper mode. Pterodactyl may update the untouched official executable normally.";
    return "<!doctype html><html><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width\"><title>Zanik maintenance</title>"
        "<style>body{margin:0;background:#101816;color:#eef7f1;font:17px system-ui;display:grid;place-items:center;min-height:100vh}main{max-width:46rem;margin:2rem;padding:2rem;background:#1b2925;border:1px solid #789487;border-radius:14px}h1{color:#9ee8c0}a{color:#9ee8c0}.notice{padding:1rem;background:#263a34;border-radius:8px}</style></head><body><main>"
        "<h1>Zanik is waiting for a compatibility update</h1><p>Dragonwilds build "+JsonEscape(build)+" is not reviewed by this installed Zanik release. The game server remains online, but Zanik's native hooks, scanners, dispatchers, persistence workers, and normal website features are disabled for safety.</p>"
        "<p class=\"notice\">"+releaseText+"</p><p>"+modeText+"</p>"
        "<p><a href=\"https://github.com/IEarnSolo/zanik/releases/latest\">Open the latest Zanik release on GitHub</a></p>"
        "<p>No game or world save files are changed by this maintenance page.</p></main></body></html>";
}

std::string UrlDecode(const std::string& value) {
    std::string decoded;
    decoded.reserve(value.size());
    auto hex = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1;
    };
    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '%' && i + 2 < value.size()) {
            int high = hex(value[i + 1]), low = hex(value[i + 2]);
            if (high >= 0 && low >= 0) {
                decoded += static_cast<char>((high << 4) | low);
                i += 2;
                continue;
            }
        }
        decoded += value[i] == '+' ? ' ' : value[i];
    }
    return decoded;
}

std::string QueryValue(const std::string& query, const std::string& key) {
    size_t start = 0;
    while (start <= query.size()) {
        size_t end = query.find('&', start);
        std::string part = query.substr(start, end == std::string::npos ? end : end - start);
        size_t equals = part.find('=');
        if (UrlDecode(part.substr(0, equals)) == key)
            return equals == std::string::npos ? "" : UrlDecode(part.substr(equals + 1));
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return "";
}

bool ParseUnsigned(const std::string& value, std::uint64_t& result) {
    if (value.empty() || value.size() > 20 ||
        !std::all_of(value.begin(), value.end(), [](unsigned char c) { return std::isdigit(c); }))
        return false;
    try {
        size_t used = 0;
        result = std::stoull(value, &used);
        return used == value.size();
    } catch (...) {
        return false;
    }
}

void Handle(int fd) {
    timeval timeout{2, 0};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    HttpRequest request;if(!ReadRequest(fd,request))return;
    if (request.method != "GET" && request.method != "POST") {
        Respond(fd, 405, "application/json; charset=utf-8", "{\"error\":\"Method not allowed\"}");
        return;
    }
    const std::string& path=request.path;const std::string& query=request.query;
    if(MaintenanceMode()){
        if(request.method!="GET"){Respond(fd,405,"application/json; charset=utf-8","{\"error\":\"Method not allowed during maintenance\"}");return;}
        if(path=="/api/update-status"){
            const char* hash=std::getenv("ZANIK_GAME_EXECUTABLE_SHA256");
            const char* mode=std::getenv("ZANIK_INSTALL_MODE");
            const char* build=std::getenv("ZANIK_GAME_BUILD_ID");
            const char* version=std::getenv("ZANIK_INSTALLED_VERSION");
            const char* autoValue=std::getenv("ZANIK_AUTO_UPDATE_VALUE");
            const char* autoHealth=std::getenv("ZANIK_AUTO_UPDATE_HEALTH");
            Respond(fd,200,"application/json; charset=utf-8","{\"website\":true,\"maintenance\":true,\"reason\":\"unsupported-game-build\",\"nativeRuntimeLoaded\":false,\"gameBuild\":\""+JsonEscape(build?build:"unknown")+"\",\"gameExecutableSha256\":\""+JsonEscape(hash?hash:"")+"\",\"zanikVersion\":\""+JsonEscape(version?version:"unknown")+"\",\"installMode\":\""+JsonEscape(mode?mode:"unknown")+"\",\"autoUpdate\":{\"value\":\""+JsonEscape(autoValue?autoValue:"unknown")+"\",\"health\":\""+JsonEscape(autoHealth?autoHealth:"unknown")+"\",\"requirement\":\""+std::string(mode&&std::strcmp(mode,"replace-executable")==0?"must-be-zero":"preserved")+"\"},\"release\":"+MaintenanceReleaseJson()+",\"backupRetention\":"+BackupRetentionStateJson()+"}");return;
        }
        if(path.rfind("/api/",0)==0){Respond(fd,503,"application/json; charset=utf-8","{\"error\":\"Zanik is in maintenance mode for an unsupported Dragonwilds build\"}");return;}
        Respond(fd,200,"text/html; charset=utf-8",PageWithSiteHeader(MaintenancePage(),""));return;
    }
    static const std::unordered_set<std::string> postRoutes={
        "/api/admin/claim","/api/admin/console-claim","/api/admin/recipe-unlock-settings",
        "/api/admin/logout","/api/admin/teleport","/api/admin/player-teleport",
        "/api/admin/storage/teleport","/api/admin/storage/mutate",
        "/api/admin/gravestone/mutate","/api/admin/gravestone/teleport"
    };
    if(request.method=="POST"&&!postRoutes.count(path)){
        Respond(fd,405,"application/json; charset=utf-8","{\"error\":\"Method not allowed\"}");return;
    }
    auto requirePost=[&](){if(request.method!="POST"){Respond(fd,405,"application/json; charset=utf-8","{\"error\":\"Method not allowed\"}");return false;}return true;};
    auto authenticate=[&](AdminSession&s,std::string&raw){raw=Cookie(request,AdminCookieName());bool removed=false;if(raw.empty()||!adminStore->Authenticate(raw,s,&removed)){Respond(fd,removed?403:401,"application/json; charset=utf-8",Status::ErrorJson(removed?"Authorization removed":"Authentication required"));return false;}return true;};
    auto disabled=[&](){Respond(fd,403,"application/json; charset=utf-8",Status::ErrorJson("Feature disabled by server operator"));};
    auto unavailable=[&](){Respond(fd,503,"application/json; charset=utf-8",Status::ErrorJson("Feature disabled by server operator"));};
    if(path.rfind("/assets/",0)==0&&(path.find("..")!=std::string::npos||path.find('\\')!=std::string::npos)){
        Respond(fd,404,"application/json; charset=utf-8",Status::ErrorJson("Not found"));return;
    }
    if (path == "/") {
        if(!ZanikConfig::PlayerStatus())Respond(fd,404,"application/json; charset=utf-8",Status::ErrorJson("Not found"));else Respond(fd, 200, "text/html; charset=utf-8", PageWithSiteHeader(kWebPage,"server"));
    } else if (path == "/player") {
        if(!ZanikConfig::PlayerStatus())Respond(fd,404,"application/json; charset=utf-8",Status::ErrorJson("Not found"));else Respond(fd, 200, "text/html; charset=utf-8", PageWithSiteHeader(kPlayerPage,"players"));
    } else if (path == "/players") {
        if(!ZanikConfig::PlayerStatus())Respond(fd,404,"application/json; charset=utf-8",Status::ErrorJson("Not found"));else Respond(fd, 200, "text/html; charset=utf-8", PageWithSiteHeader(kPlayersPage,"players"));
    } else if (path == "/kills") {
        if(!ZanikConfig::KillTracking()&&!ZanikConfig::PlayerStatus())Respond(fd,404,"application/json; charset=utf-8",Status::ErrorJson("Not found"));else Respond(fd, 200, "text/html; charset=utf-8", PageWithSiteHeader(kKillsPage,"kills"));
    } else if (path == "/map") {
        if(!ZanikConfig::Map())Respond(fd,404,"application/json; charset=utf-8",Status::ErrorJson("Not found"));else Respond(fd, 200, "text/html; charset=utf-8", PageWithSiteHeader(kMapPage,"map"));
    } else if (path == "/recipe-unlocks") {
        Respond(fd, 200, "text/html; charset=utf-8", PageWithSiteHeader(kRecipeUnlocksPage,"recipes"));
    } else if(path=="/admin/recipe-unlocks"){
        AdminSession s;std::string raw;if(!authenticate(s,raw))return;if(!ZanikConfig::AdminRecipeSettings()){disabled();return;}Respond(fd,200,"text/html; charset=utf-8",PageWithSiteHeader(kRecipeUnlockSettingsPage,"recipes"),PersistentAdminCookie(raw));
    } else if(path=="/admin/claim"){
        Respond(fd,200,"text/html; charset=utf-8",PageWithSiteHeader(R"HTML(<!doctype html><meta charset="utf-8"><meta name="viewport" content="width=device-width"><title>Zanik admin login</title><style>body{font:16px system-ui;background:#101816;color:#eef7f1;display:grid;place-items:center;min-height:100vh}main{max-width:32rem;padding:2rem;background:#1b2925;border:1px solid #789487;border-radius:.8rem}a{color:#9ee8c0}</style><main><h1>Zanik administration</h1><p id="status">Redeeming private login link…</p><p><a href="/map">Open map</a></p></main><script>(async()=>{const s=document.getElementById('status'),p=new URLSearchParams(location.hash.slice(1)),token=p.get('token');history.replaceState(null,'',location.pathname);if(!token){s.textContent='This login link is missing its token.';return}try{const r=await fetch('/api/admin/claim',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({token})}),j=await r.json();if(!r.ok)throw Error(j.error||'Login failed');s.textContent='Signed in as '+j.characterName+'. Redirecting to the map…';setTimeout(()=>location.replace('/map'),600)}catch(e){s.textContent=e.message}})()</script>)HTML","map"));
    } else if(path=="/api/admin/claim"){
        if(!requirePost())return;if(!ZanikConfig::AdminRead()){disabled();return;}std::string raw;AdminSession s;std::string token=JsonValue(request.body,"token");
        if(token.size()!=64||!adminStore->RedeemClaim(token,raw,s))Respond(fd,401,"application/json; charset=utf-8",Status::ErrorJson("Invalid, expired, or used login link"));
        else Respond(fd,200,"application/json; charset=utf-8","{\"authenticated\":true,\"characterId\":\""+JsonEscape(s.publicId)+"\",\"characterName\":\""+JsonEscape(s.characterName)+"\",\"csrf\":\""+s.csrf+"\"}",PersistentAdminCookie(raw));
    } else if(path=="/api/admin/console-claim"){
        if(!requirePost())return;if(!ZanikConfig::AdminRead()){disabled();return;}auto key=request.headers.find("x-zanik-console-key");std::string secret=ConsoleSecret();if(secret.empty()||key==request.headers.end()||key->second!=secret){Respond(fd,403,"application/json; charset=utf-8",Status::ErrorJson("Console authorization required"));return;}
        std::string id=request.body;while(!id.empty()&&(id.back()=='\r'||id.back()=='\n'||id.back()==' '||id.back()=='\t'))id.pop_back();auto begin=id.find_first_not_of(" \t");if(begin!=std::string::npos&&begin)id.erase(0,begin);if(id.empty()){auto ids=adminStore->AllowedIds();if(ids.size()!=1){Respond(fd,409,"application/json; charset=utf-8",Status::ErrorJson("Specify a public character ID"));return;}id=ids.front();}
        StoredPlayer p;if(!playerStore->Find(id,p)||p.identity.rfind("character:",0)!=0||!adminStore->IsAllowed(id)){Respond(fd,403,"application/json; charset=utf-8",Status::ErrorJson("Character is not authorized"));return;}std::string token=adminStore->CreateClaim(id,p.identity.substr(10),p.name);if(token.empty()){Respond(fd,503,"application/json; charset=utf-8",Status::ErrorJson("Could not create claim"));return;}std::string url=PublicBaseUrl()+"/admin/claim#token="+token;Respond(fd,200,"application/json; charset=utf-8","{\"url\":\""+JsonEscape(url)+"\",\"expiresIn\":300,\"characterName\":\""+JsonEscape(p.name)+"\"}");
    } else if(path=="/api/admin/session"){
        AdminSession s;std::string raw;if(!authenticate(s,raw))return;if(!ZanikConfig::AdminRead()){disabled();return;}StoredPlayer p;bool online=playerStore->Find(s.publicId,p)&&p.online;
        // Reflection registration can finish after the native hook is installed.  Retry the
        // bounded nodestone layout check here on the HTTP worker before advertising capability.
        DomEngine::RefreshAdminNodestoneTeleportCompatibility();
        const auto storage=DomEngine::GetAdminStorageDiagnostics();const auto graves=DomEngine::GetAdminGravestoneDiagnostics();const auto teleport=DomEngine::GetAdminTeleportDiagnostics();
        const bool engineReadable=DomEngine::g_Engine&&DomEngine::g_Engine->IsInitialized();
        const std::string capabilities="{\"storageRead\":"+std::string(engineReadable?"true":"false")+
            ",\"gravestoneRead\":"+(graves.compatibility==1?"true":"false")+
            ",\"storageMutation\":"+(ZanikConfig::AdminStorageMutation()&&storage.compatibility==1?"true":"false")+
            ",\"storageRename\":"+(ZanikConfig::AdminStorageMutation()&&storage.renameCompatibility==1?"true":"false")+
            ",\"gravestoneMutation\":"+(ZanikConfig::AdminGravestoneMutation()&&graves.compatibility==1?"true":"false")+
            ",\"teleport\":"+(ZanikConfig::AdminTeleport()&&teleport.compatibility==1?"true":"false")+
            ",\"nodestoneTeleport\":"+(ZanikConfig::AdminTeleport()&&teleport.nodestoneCompatibility==1?"true":"false")+"}";
        Respond(fd,200,"application/json; charset=utf-8","{\"authenticated\":true,\"characterId\":\""+JsonEscape(s.publicId)+"\",\"characterName\":\""+JsonEscape(s.characterName)+"\",\"online\":"+(online?"true":"false")+",\"csrf\":\""+s.csrf+"\",\"features\":"+ZanikConfig::Json()+",\"capabilities\":"+capabilities+"}",PersistentAdminCookie(raw));
    } else if(path=="/api/recipe-unlock-settings"){
        Respond(fd,200,"application/json; charset=utf-8",recipeSettingsStore->Json());
    } else if(path=="/api/admin/recipe-unlock-settings"){
        AdminSession s;std::string raw;if(!authenticate(s,raw))return;
        if(!ZanikConfig::AdminRecipeSettings()){disabled();return;}
        if(request.method=="GET"){std::string body=recipeSettingsStore->Json();if(!body.empty()&&body.back()=='}')body.pop_back();body+=",\"csrf\":\""+s.csrf+"\"}";Respond(fd,200,"application/json; charset=utf-8",body);return;}
        if(!requirePost())return;if(JsonValue(request.body,"csrf")!=s.csrf){Respond(fd,403,"application/json; charset=utf-8",Status::ErrorJson("Invalid CSRF token"));return;}const std::string enabledValue=JsonValue(request.body,"enabled");const bool enabled=enabledValue=="true";std::string catalog;if((enabledValue!="true"&&enabledValue!="false")||ReadRecipeCatalogHealth(&catalog).compatibility!=1){Respond(fd,400,"application/json; charset=utf-8",Status::ErrorJson("Invalid recipe setting"));return;}auto validKey=[&](const std::string&key){std::string id=key;auto dot=key.rfind('.');if(dot!=std::string::npos){const std::string route=key.substr(dot+1);if(route.empty()||route.size()>3||route.find_first_not_of("0123456789")!=std::string::npos||std::stoi(route)>=100)return false;id=key.substr(0,dot);}return !id.empty()&&catalog.find("\"id\":\""+id+"\"")!=std::string::npos;};if(JsonValue(request.body,"bulk")=="true"){const auto ids=JsonStringArray(request.body,"ids");if(ids.empty()||ids.size()>4096||!std::all_of(ids.begin(),ids.end(),validKey)){Respond(fd,400,"application/json; charset=utf-8",Status::ErrorJson("Invalid recipe settings"));return;}if(!recipeSettingsStore->SetManyEnabled(ids,enabled)){Respond(fd,503,"application/json; charset=utf-8",Status::ErrorJson("Recipe settings could not be saved"));return;}adminStore->Audit(s,"bulk:"+std::to_string(ids.size()),enabled?"recipe-routes-enabled":"recipe-routes-disabled");Respond(fd,200,"application/json; charset=utf-8",recipeSettingsStore->Json());return;}const std::string id=JsonValue(request.body,"id"),route=JsonValue(request.body,"route"),key=route.empty()?id:id+"."+route;if(!validKey(key)){Respond(fd,400,"application/json; charset=utf-8",Status::ErrorJson("Invalid recipe setting"));return;}if(!recipeSettingsStore->SetEnabled(key,enabled)){Respond(fd,503,"application/json; charset=utf-8",Status::ErrorJson("Recipe settings could not be saved"));return;}adminStore->Audit(s,key,enabled?"recipe-route-enabled":"recipe-route-disabled");Respond(fd,200,"application/json; charset=utf-8",recipeSettingsStore->Json());
    } else if(path=="/api/admin/logout"){
        if(!requirePost())return;AdminSession s;std::string raw;if(!authenticate(s,raw))return;if(JsonValue(request.body,"csrf")!=s.csrf){Respond(fd,403,"application/json; charset=utf-8",Status::ErrorJson("Invalid CSRF token"));return;}adminStore->Logout(raw);Respond(fd,200,"application/json; charset=utf-8","{\"authenticated\":false}","Set-Cookie: "+AdminCookieName()+"=; HttpOnly; SameSite=Strict; Path=/; Max-Age=0\r\n");
    } else if(path=="/api/admin/teleport"){
        if(!requirePost())return;AdminSession s;std::string raw;if(!authenticate(s,raw))return;if(!ZanikConfig::AdminTeleport()){disabled();return;}if(JsonValue(request.body,"csrf")!=s.csrf){Respond(fd,403,"application/json; charset=utf-8",Status::ErrorJson("Invalid CSRF token"));return;}const std::string lodestoneId=JsonValue(request.body,"lodestoneId"),nodestoneId=JsonValue(request.body,"nodestoneId");const bool isNodestone=!nodestoneId.empty();const std::string destination=isNodestone?nodestoneId:lodestoneId;
        if((lodestoneId.empty()==nodestoneId.empty())||destination.size()!=16||destination.find_first_not_of("0123456789abcdefABCDEF")!=std::string::npos){adminStore->Audit(s,destination,"invalid-destination");Respond(fd,409,"application/json; charset=utf-8",Status::ErrorJson("Select exactly one valid teleport destination"));return;}
        bool destinationFound=false;DomEngine::NodestoneInfo selectedNodestone;
        if(isNodestone){std::vector<DomEngine::NodestoneInfo> current;if(DomEngine::ReadNodestones(current))for(const auto&n:current)if(n.id==destination){selectedNodestone=n;destinationFound=true;break;}}
        else{std::vector<DomEngine::LodestoneInfo> current;if(DomEngine::ReadLodestones(current))for(const auto&l:current)if(l.id==destination){destinationFound=true;break;}}
        if(!destinationFound){adminStore->Audit(s,destination,"stale-destination");Respond(fd,409,"application/json; charset=utf-8",Status::ErrorJson(isNodestone?"Nodestone no longer exists":"Lodestone no longer exists"));return;}
        auto now=std::chrono::steady_clock::now();std::string rateKey=adminStore->SessionHashPrefix(raw);auto rate=adminTeleportRate.find(rateKey);if(rate!=adminTeleportRate.end()&&now-rate->second<std::chrono::seconds(3)){adminStore->Audit(s,destination,"rate-limited");Respond(fd,429,"application/json; charset=utf-8",Status::ErrorJson("Please wait before teleporting again"));return;}adminTeleportRate[rateKey]=now;
        StoredPlayer p;if(!playerStore->Find(s.publicId,p)||!p.online){adminStore->Audit(s,destination,"offline");Respond(fd,409,"application/json; charset=utf-8",Status::ErrorJson("Linked character is offline"));return;}
        const bool queued=isNodestone?DomEngine::QueueAdminNodestoneTeleport(s.characterGuid,s.publicId,selectedNodestone,rateKey):DomEngine::QueueAdminTeleport(s.characterGuid,s.publicId,destination,rateKey);
        if(!queued){
            auto diagnostics=DomEngine::GetAdminTeleportDiagnostics();const bool unavailable=diagnostics.compatibility!=1||(isNodestone&&diagnostics.nodestoneCompatibility!=1);
            adminStore->Audit(s,destination,unavailable?"compatibility-unavailable":"already-pending");
            Respond(fd,unavailable?503:409,"application/json; charset=utf-8",Status::ErrorJson(unavailable?(isNodestone?"Nodestone teleport compatibility unavailable":"Admin teleport compatibility unavailable"):"A teleport is already pending"));return;
        }
        adminStore->Audit(s,destination,"queued");Respond(fd,202,"application/json; charset=utf-8","{\"accepted\":true,\"status\":\"queued\"}");
    } else if(path=="/api/admin/player-teleport"){
        if(!requirePost())return;AdminSession s;std::string raw;if(!authenticate(s,raw))return;
        if(!ZanikConfig::AdminTeleport()){disabled();return;}
        if(JsonValue(request.body,"csrf")!=s.csrf){Respond(fd,403,"application/json; charset=utf-8",Status::ErrorJson("Invalid CSRF token"));return;}
        const std::string action=JsonValue(request.body,"action"),targetId=JsonValue(request.body,"playerId");
        if((action!="to-player"&&action!="player-to-me")||targetId.empty()||targetId.size()>128||targetId==s.publicId){
            adminStore->Audit(s,targetId,"invalid-player-action");Respond(fd,409,"application/json; charset=utf-8",Status::ErrorJson("Invalid player teleport request"));return;
        }
        StoredPlayer adminPlayer,targetPlayer;
        if(!playerStore->Find(s.publicId,adminPlayer)||!adminPlayer.online){adminStore->Audit(s,targetId,"admin-offline");Respond(fd,409,"application/json; charset=utf-8",Status::ErrorJson("Linked character is offline"));return;}
        if(!playerStore->Find(targetId,targetPlayer)||!targetPlayer.online||targetPlayer.identity.rfind("character:",0)!=0){adminStore->Audit(s,targetId,"target-offline");Respond(fd,409,"application/json; charset=utf-8",Status::ErrorJson("Selected player is offline"));return;}
        auto now=std::chrono::steady_clock::now();std::string rateKey=adminStore->SessionHashPrefix(raw);auto rate=adminTeleportRate.find(rateKey);
        if(rate!=adminTeleportRate.end()&&now-rate->second<std::chrono::seconds(3)){adminStore->Audit(s,targetId,"rate-limited");Respond(fd,429,"application/json; charset=utf-8",Status::ErrorJson("Please wait before teleporting again"));return;}adminTeleportRate[rateKey]=now;
        const bool toPlayer=action=="to-player";
        const std::string movingGuid=toPlayer?s.characterGuid:targetPlayer.identity.substr(10),movingId=toPlayer?s.publicId:targetId;
        const std::string anchorGuid=toPlayer?targetPlayer.identity.substr(10):s.characterGuid,anchorId=toPlayer?targetId:s.publicId;
        if(!DomEngine::QueueAdminPlayerTeleport(movingGuid,movingId,anchorGuid,anchorId,action,rateKey)){
            auto diagnostics=DomEngine::GetAdminTeleportDiagnostics();const bool unavailable=diagnostics.compatibility!=1;
            adminStore->Audit(s,targetId,unavailable?"player-teleport-unavailable":"player-teleport-pending");
            Respond(fd,unavailable?503:409,"application/json; charset=utf-8",Status::ErrorJson(unavailable?"Admin teleport compatibility unavailable":"A teleport is already pending"));return;
        }
        adminStore->Audit(s,targetId,"queued-"+action);Respond(fd,202,"application/json; charset=utf-8","{\"accepted\":true,\"status\":\"queued\"}");
    } else if(path=="/api/admin-diagnostics"){
        auto initialStorage=DomEngine::GetAdminStorageDiagnostics();if(initialStorage.compatibility==1&&initialStorage.resolvedBuildingPieces==0)(void)DomEngine::GetAdminStorageRecipesJson();
        auto d=DomEngine::GetAdminTeleportDiagnostics();auto storage=DomEngine::GetAdminStorageDiagnostics();auto graves=DomEngine::GetAdminGravestoneDiagnostics();auto auth=adminStore->Diagnostics();const bool authHealthy=auth.allowlistReadable&&auth.sessionStorageReadable&&auth.sessionStorageWritable&&auth.sessionStorageRestricted&&auth.lastSaveSucceeded&&auth.malformedSessionRecords==0;const bool consoleHealthy=authHealthy&&!ConsoleSecret().empty();std::ostringstream body;body<<"{\"authorizationStorage\":"<<(authHealthy?1:0)<<",\"allowlistReadable\":"<<(auth.allowlistReadable?1:0)<<",\"sessionStorageReadable\":"<<(auth.sessionStorageReadable?1:0)<<",\"sessionStorageWritable\":"<<(auth.sessionStorageWritable?1:0)<<",\"sessionStorageRestricted\":"<<(auth.sessionStorageRestricted?1:0)<<",\"sessionStorageMalformedRecords\":"<<auth.malformedSessionRecords<<",\"allowedAdminCharacters\":"<<auth.allowedCharacters<<",\"activeAdminSessions\":"<<auth.activeSessions<<",\"consoleCommandCompatibility\":"<<(consoleHealthy?1:0)<<",\"storageCompatibility\":"<<storage.compatibility<<",\"storageDispatcherCompatibility\":"<<storage.dispatcherCompatibility<<",\"storageMutationCompatibility\":"<<storage.mutationCompatibility<<",\"storageBatchCompatibility\":"<<storage.batchCompatibility<<",\"storageOrganizationCompatibility\":"<<storage.organizationCompatibility<<",\"storageRenameCompatibility\":"<<storage.renameCompatibility<<",\"storageRemovalTrackingCompatibility\":"<<storage.removalTrackingCompatibility<<",\"storageCatalogCompatibility\":"<<storage.catalogCompatibility<<",\"storageUnlockCatalogCompatibility\":"<<storage.unlockCatalogCompatibility<<",\"gravestoneCompatibility\":"<<graves.compatibility<<",\"gravestoneInventoryCompatibility\":"<<graves.inventoryCompatibility<<",\"gravestoneRegistryCompatibility\":"<<graves.registryCompatibility<<",\"gravestoneStreamingCompatibility\":"<<graves.streamingCompatibility<<",\"gravestoneRespawnCaptureCompatibility\":"<<graves.respawnCaptureCompatibility<<",\"gravestonePersistenceCompatibility\":"<<graves.persistenceCompatibility<<",\"discoveredGravestones\":"<<graves.discovered<<",\"unresolvedGravestones\":"<<graves.unresolved<<",\"queuedGravestoneMutations\":"<<graves.queuedRequests<<",\"acceptedGravestoneMutations\":"<<graves.acceptedMutations<<",\"rejectedGravestoneMutations\":"<<graves.rejectedRequests<<",\"queuedStorageMutations\":"<<storage.queuedRequests<<",\"acceptedStorageMutations\":"<<storage.acceptedMutations<<",\"rejectedStorageMutations\":"<<storage.rejectedRequests<<",\"teleportCompatibility\":"<<d.compatibility<<",\"lodestoneTeleportCompatibility\":"<<d.lodestoneCompatibility<<",\"nodestoneTeleportCompatibility\":"<<d.nodestoneCompatibility<<",\"queuedNodestoneTeleports\":"<<d.queuedNodestoneTeleports<<",\"acceptedNodestoneTeleports\":"<<d.acceptedNodestoneTeleports<<",\"rejectedNodestoneTeleports\":"<<d.rejectedNodestoneTeleports<<",\"timedOutNodestoneTeleports\":"<<d.timedOutNodestoneTeleports<<",\"staleNodestoneDestinations\":"<<d.staleNodestoneDestinations<<",\"concurrentNodestoneRequests\":"<<d.concurrentNodestoneRequests<<",\"maximumNodestoneExecutionMicroseconds\":"<<d.maximumNodestoneExecutionMicroseconds<<",\"slowNodestoneCalls\":"<<d.slowNodestoneCalls<<",\"nodestoneCircuitBroken\":"<<d.nodestoneCircuitBroken<<",\"playerTeleportCompatibility\":"<<d.playerCompatibility<<",\"teleportGroundTraceCompatibility\":"<<d.groundTraceCompatibility<<",\"storageTeleportPipelineCompatibility\":"<<d.storagePipelineCompatibility<<",\"nativeTeleportCaptureCompatibility\":"<<d.nativeCaptureCompatibility<<",\"nativeTeleportTemplateAvailable\":"<<d.nativeTemplateAvailable<<",\"queuedRequests\":"<<d.queuedRequests<<",\"acceptedTeleports\":"<<d.acceptedTeleports<<",\"rejectedRequests\":"<<d.rejectedRequests<<",\"acceptedStorageTeleports\":"<<d.acceptedStorageTeleports<<",\"staleStorageDestinations\":"<<d.staleStorageDestinations<<",\"unsafeStorageDestinations\":"<<d.unsafeStorageDestinations<<",\"maximumExecutionMicroseconds\":"<<d.maximumExecutionMicroseconds<<",\"teleportTickCalls\":"<<d.tickCalls<<",\"teleportMaximumTickMicroseconds\":"<<d.maximumTickMicroseconds<<",\"teleportSlowTickCalls\":"<<d.slowTickCalls<<",\"engineTickCalls\":"<<storage.tickCalls<<",\"engineMaximumTickMicroseconds\":"<<storage.maximumTickMicroseconds<<",\"engineSlowTickCalls\":"<<storage.slowTickCalls<<",\"maximumRecipeTickMicroseconds\":"<<storage.maximumRecipeTickMicroseconds<<",\"maximumCustomNameTickMicroseconds\":"<<storage.maximumCustomNameTickMicroseconds<<",\"maximumStorageTickMicroseconds\":"<<storage.maximumStorageTickMicroseconds<<",\"maximumGravestoneTickMicroseconds\":"<<storage.maximumGravestoneTickMicroseconds<<'}';Respond(fd,200,"application/json; charset=utf-8",body.str());
    } else if(path=="/api/admin/storage-diagnostics"){
        AdminSession s;std::string raw;if(!authenticate(s,raw))return;
        if(!DomEngine::g_Engine||!DomEngine::g_Engine->IsInitialized())Respond(fd,503,"application/json; charset=utf-8",Status::ErrorJson("Engine unavailable"));
        else {auto d=DomEngine::GetAdminStorageDiagnostics();std::string base=DomEngine::g_Engine->GetStorageDiagnosticsJson();if(!base.empty()&&base.back()=='}')base.pop_back();base+=",\"dispatcherCompatibility\":"+std::to_string(d.dispatcherCompatibility)+",\"mutationCompatibility\":"+std::to_string(d.mutationCompatibility)+",\"snapshotCompatibility\":"+std::to_string(d.snapshotCompatibility)+",\"singleItemCompatibility\":"+std::to_string(d.singleItemCompatibility)+",\"slotMovementCompatibility\":"+std::to_string(d.slotMovementCompatibility)+",\"batchAdditionCompatibility\":"+std::to_string(d.batchAdditionCompatibility)+",\"batchMovementCompatibility\":"+std::to_string(d.batchMovementCompatibility)+",\"organizationCompatibility\":"+std::to_string(d.organizationCompatibility)+",\"deletionCompatibility\":"+std::to_string(d.deletionCompatibility)+",\"renameCompatibility\":"+std::to_string(d.renameCompatibility)+",\"rollbackCompatibility\":"+std::to_string(d.rollbackCompatibility)+",\"removalTrackingCompatibility\":"+std::to_string(d.removalTrackingCompatibility)+",\"catalogCompatibility\":"+std::to_string(d.catalogCompatibility)+",\"buildingTaxonomyCompatibility\":"+std::to_string(d.buildingTaxonomyCompatibility)+",\"buildingRequirementsCompatibility\":"+std::to_string(d.buildingRequirementsCompatibility)+",\"buildingIconsCompatibility\":"+std::to_string(d.buildingIconsCompatibility)+",\"craftingRecipeCompatibility\":"+std::to_string(d.craftingRecipeCompatibility)+",\"expectedBuildingPieces\":"+std::to_string(d.expectedBuildingPieces)+",\"resolvedBuildingPieces\":"+std::to_string(d.resolvedBuildingPieces)+",\"unavailableBuildingPieces\":"+std::to_string(d.unavailableBuildingPieces)+",\"buildingTabs\":"+std::to_string(d.buildingTabs)+",\"buildingSections\":"+std::to_string(d.buildingSections)+",\"buildingReferences\":"+std::to_string(d.buildingReferences)+",\"queuedMutations\":"+std::to_string(d.queuedRequests)+",\"queueDepth\":"+std::to_string(d.queueDepth)+",\"queueDrops\":"+std::to_string(d.queueDrops)+",\"runningJobs\":"+std::to_string(d.runningJobs)+",\"completedJobs\":"+std::to_string(d.completedJobs)+",\"failedJobs\":"+std::to_string(d.failedJobs)+",\"acceptedMutations\":"+std::to_string(d.acceptedMutations)+",\"rejectedMutations\":"+std::to_string(d.rejectedRequests)+",\"staleMutations\":"+std::to_string(d.staleRequests)+",\"staleIdentities\":"+std::to_string(d.staleIdentities)+",\"worldChanges\":"+std::to_string(d.worldChanges)+",\"snapshotGenerations\":"+std::to_string(d.snapshotGenerations)+",\"snapshotCacheHits\":"+std::to_string(d.snapshotCacheHits)+",\"snapshotCacheMisses\":"+std::to_string(d.snapshotCacheMisses)+",\"snapshotInvalidations\":"+std::to_string(d.snapshotInvalidations)+",\"maximumSnapshotPreparationMicroseconds\":"+std::to_string(d.maximumSnapshotPreparationMicroseconds)+",\"rollbackAttempts\":"+std::to_string(d.rollbackAttempts)+",\"rollbackFailures\":"+std::to_string(d.rollbackFailures)+",\"circuitBreakers\":"+std::to_string(d.circuitBreakers)+",\"maximumMutationMicroseconds\":"+std::to_string(d.maximumExecutionMicroseconds)+",\"singleItemMaximumMicroseconds\":"+std::to_string(d.singleItemMaximumMicroseconds)+",\"slotMovementMaximumMicroseconds\":"+std::to_string(d.slotMovementMaximumMicroseconds)+",\"batchAdditionMaximumMicroseconds\":"+std::to_string(d.batchAdditionMaximumMicroseconds)+",\"batchMovementMaximumMicroseconds\":"+std::to_string(d.batchMovementMaximumMicroseconds)+",\"deletionMaximumMicroseconds\":"+std::to_string(d.deletionMaximumMicroseconds)+",\"renameMaximumMicroseconds\":"+std::to_string(d.renameMaximumMicroseconds)+",\"rollbackMaximumMicroseconds\":"+std::to_string(d.rollbackMaximumMicroseconds)+"}";Respond(fd,200,"application/json; charset=utf-8",base);}
    } else if(path=="/api/admin/storage/catalog"){
        AdminSession s;std::string raw;if(!authenticate(s,raw))return;std::string body=DomEngine::GetAdminStorageCatalogJson();if(body.empty())Respond(fd,503,"application/json; charset=utf-8",Status::ErrorJson("Storage editing compatibility unavailable"));else Respond(fd,200,"application/json; charset=utf-8",body);
    } else if(path=="/api/admin/storage/recipes"){
        AdminSession s;std::string raw;if(!authenticate(s,raw))return;std::string body=DomEngine::GetAdminStorageRecipesJson();if(body.empty())Respond(fd,503,"application/json; charset=utf-8",Status::ErrorJson("Recipe compatibility unavailable"));else Respond(fd,200,"application/json; charset=utf-8",body);
    } else if(path=="/api/admin/storage/teleport"){
        if(!requirePost())return;AdminSession s;std::string raw;if(!authenticate(s,raw))return;
        if(!ZanikConfig::AdminTeleport()){disabled();return;}
        if(JsonValue(request.body,"csrf")!=s.csrf){Respond(fd,403,"application/json; charset=utf-8",Status::ErrorJson("Invalid CSRF token"));return;}
        const std::string targetId=JsonValue(request.body,"playerId"),chestId=JsonValue(request.body,"chestId");
        auto validOpaque=[](const std::string&v){return v.size()==16&&v.find_first_not_of("0123456789abcdefABCDEF")==std::string::npos;};
        if(targetId.empty()||targetId.size()>128||!validOpaque(chestId)){adminStore->Audit(s,chestId,"storage-teleport-invalid");Respond(fd,400,"application/json; charset=utf-8",Status::ErrorJson("Invalid storage teleport request"));return;}
        StoredPlayer adminPlayer,targetPlayer;
        if(!playerStore->Find(s.publicId,adminPlayer)||!adminPlayer.online){adminStore->Audit(s,chestId,"storage-teleport-offline");Respond(fd,409,"application/json; charset=utf-8",Status::ErrorJson("Linked character is offline"));return;}
        if(!playerStore->Find(targetId,targetPlayer)||targetPlayer.identity.rfind("character:",0)!=0){Respond(fd,404,"application/json; charset=utf-8",Status::ErrorJson("Player not found"));return;}
        FVector3d location{};if(!DomEngine::ResolveAdminStorageLocation(targetPlayer.identity.substr(10),chestId,location)){adminStore->Audit(s,chestId,"storage-teleport-stale");Respond(fd,409,"application/json; charset=utf-8",Status::ErrorJson("Storage container is no longer available"));return;}
        auto now=std::chrono::steady_clock::now();const std::string rateKey=adminStore->SessionHashPrefix(raw);auto rate=adminTeleportRate.find(rateKey);
        if(rate!=adminTeleportRate.end()&&now-rate->second<std::chrono::seconds(3)){adminStore->Audit(s,chestId,"storage-teleport-rate-limited");Respond(fd,429,"application/json; charset=utf-8",Status::ErrorJson("Please wait before teleporting again"));return;}adminTeleportRate[rateKey]=now;
        if(!DomEngine::QueueAdminStorageTeleport(s.characterGuid,s.publicId,targetPlayer.identity.substr(10),targetId,chestId,rateKey)){
            auto diagnostics=DomEngine::GetAdminTeleportDiagnostics();const bool unavailable=diagnostics.compatibility!=1||diagnostics.groundTraceCompatibility!=1||diagnostics.storagePipelineCompatibility!=1||diagnostics.nativeCaptureCompatibility!=1||diagnostics.nativeTemplateAvailable!=1||diagnostics.storageTransportCompatibility!=1;
            adminStore->Audit(s,chestId,unavailable?"storage-teleport-unavailable":"storage-teleport-pending");Respond(fd,unavailable?503:409,"application/json; charset=utf-8",Status::ErrorJson(unavailable?"Storage teleport compatibility unavailable":"A teleport is already pending"));return;
        }
        adminStore->Audit(s,chestId,"storage-teleport-queued");Respond(fd,202,"application/json; charset=utf-8","{\"accepted\":true,\"status\":\"queued\"}");
    } else if(path=="/api/admin/storage/mutate"){
        if(!requirePost())return;AdminSession s;std::string raw;if(!authenticate(s,raw))return;if(!ZanikConfig::AdminStorageMutation()){disabled();return;}if(JsonValue(request.body,"csrf")!=s.csrf){Respond(fd,403,"application/json; charset=utf-8",Status::ErrorJson("Invalid CSRF token"));return;}
        const std::string operation=JsonValue(request.body,"operation"),targetId=JsonValue(request.body,"playerId"),chestId=JsonValue(request.body,"chestId"),itemId=JsonValue(request.body,"itemId"),destinationChestId=JsonValue(request.body,"destinationChestId"),customName=JsonValue(request.body,"name");std::string catalogId=JsonValue(request.body,"catalogId");if(operation=="add_recipe")catalogId=JsonValue(request.body,"recipeId");int quantity=0,destinationSlot=-1;const auto itemIds=JsonStringArray(request.body,"itemIds");
        auto validOpaque=[](const std::string&v){return v.size()==16&&v.find_first_not_of("0123456789abcdefABCDEF")==std::string::npos;};
        const bool slotOperation=operation=="slot_move",batchOperation=operation=="move_batch"||operation=="copy_batch"||operation=="organize_batch",deleteBatch=operation=="delete_batch";bool valid=targetId.size()>0&&validOpaque(chestId);
        if(operation=="add"||operation=="add_recipe")valid=valid&&JsonInteger(request.body,"quantity",quantity)&&validOpaque(catalogId);else if(operation=="delete"||operation=="take"||operation=="add_existing")valid=valid&&JsonInteger(request.body,"quantity",quantity)&&validOpaque(itemId);else if(operation=="rename")valid=valid&&customName.size()<=96&&std::none_of(customName.begin(),customName.end(),[](unsigned char c){return c<32&&c!='\t';});else if(slotOperation)valid=valid&&validOpaque(itemId)&&validOpaque(destinationChestId)&&JsonNonNegativeInteger(request.body,"destinationSlot",destinationSlot);else if(batchOperation||deleteBatch){valid=valid&&(!batchOperation||validOpaque(destinationChestId))&&!itemIds.empty()&&itemIds.size()<=128;for(const auto&value:itemIds)valid=valid&&validOpaque(value);}else valid=false;
        if(!valid){adminStore->Audit(s,chestId,"storage-invalid-request");Respond(fd,400,"application/json; charset=utf-8",Status::ErrorJson("Invalid storage request"));return;}
        StoredPlayer target;if(!playerStore->Find(targetId,target)||target.identity.rfind("character:",0)!=0){Respond(fd,404,"application/json; charset=utf-8",Status::ErrorJson("Player not found"));return;}
        const std::string rateKey=adminStore->SessionHashPrefix(raw);auto now=std::chrono::steady_clock::now();auto rate=adminStorageRate.find(rateKey);if(rate!=adminStorageRate.end()&&now-rate->second<std::chrono::milliseconds(400)){adminStore->Audit(s,chestId,"storage-rate-limited");Respond(fd,429,"application/json; charset=utf-8",Status::ErrorJson("Please wait before changing storage again"));return;}adminStorageRate[rateKey]=now;
        std::string requestId,result;bool ok=DomEngine::SubmitAdminStorageMutation(operation,s.characterGuid,s.publicId,target.identity.substr(10),targetId,chestId,itemId,catalogId,quantity,rateKey,requestId,result,destinationChestId,destinationSlot,itemIds,customName);
        adminStore->Audit(s,chestId,operation+":"+(itemId.empty()?catalogId:itemId)+":"+std::to_string(quantity)+":"+(ok?"accepted":"rejected")+":"+result);
        if(ok)Respond(fd,202,"application/json; charset=utf-8","{\"accepted\":true,\"status\":\"queued\",\"requestId\":\""+JsonEscape(requestId)+"\",\"message\":\""+JsonEscape(result)+"\"}");else{auto d=DomEngine::GetAdminStorageDiagnostics();int status=d.compatibility!=1?503:(result.find("pending")!=std::string::npos||result.find("queue")!=std::string::npos?429:409);Respond(fd,status,"application/json; charset=utf-8",Status::ErrorJson(result));}
    } else if(path=="/api/admin/storage/mutation-status"){
        AdminSession s;std::string raw;if(!authenticate(s,raw))return;const std::string id=QueryValue(query,"id");if(id.size()!=32||id.find_first_not_of("0123456789abcdefABCDEF")!=std::string::npos){Respond(fd,400,"application/json; charset=utf-8",Status::ErrorJson("Invalid storage request id"));return;}const std::string body=DomEngine::GetAdminStorageMutationStatus(id,adminStore->SessionHashPrefix(raw));if(body.empty())Respond(fd,404,"application/json; charset=utf-8",Status::ErrorJson("Storage request not found"));else Respond(fd,200,"application/json; charset=utf-8",body);
    } else if(path=="/api/admin/player-storage"){
        AdminSession s;std::string raw;if(!authenticate(s,raw))return;std::string id=QueryValue(query,"id");StoredPlayer player;
        if(id.empty()){Respond(fd,400,"application/json; charset=utf-8",Status::ErrorJson("Player id required"));return;}
        if(!playerStore->Find(id,player)||player.identity.rfind("character:",0)!=0){Respond(fd,404,"application/json; charset=utf-8",Status::ErrorJson("Player not found"));return;}
        if(!DomEngine::g_Engine||!DomEngine::g_Engine->IsInitialized()){Respond(fd,503,"application/json; charset=utf-8",Status::ErrorJson("Storage data unavailable"));return;}
        std::string body=DomEngine::GetAdminPlayerStorageJson(player.identity.substr(10),s.characterGuid);
        if(body.empty())Respond(fd,503,"application/json; charset=utf-8",Status::ErrorJson("Storage compatibility unavailable"));
        else Respond(fd,200,"application/json; charset=utf-8",body);
    } else if(path=="/api/admin/player-gravestones"){
        AdminSession s;std::string raw;if(!authenticate(s,raw))return;std::string id=QueryValue(query,"id");StoredPlayer player;
        if(id.empty()){Respond(fd,400,"application/json; charset=utf-8",Status::ErrorJson("Player id required"));return;}
        if(!playerStore->Find(id,player)||player.identity.rfind("character:",0)!=0){Respond(fd,404,"application/json; charset=utf-8",Status::ErrorJson("Player not found"));return;}
        if(!DomEngine::g_Engine||!DomEngine::g_Engine->IsInitialized()){Respond(fd,503,"application/json; charset=utf-8",Status::ErrorJson("Gravestone data unavailable"));return;}
        std::string body=DomEngine::GetPlayerGravestonesJson(player.identity.substr(10),player.name);
        if(body.empty())Respond(fd,503,"application/json; charset=utf-8",Status::ErrorJson("Gravestone compatibility unavailable"));else Respond(fd,200,"application/json; charset=utf-8",body);
    } else if(path=="/api/admin/gravestone/mutate"){
        if(!requirePost())return;AdminSession s;std::string raw;if(!authenticate(s,raw))return;if(!ZanikConfig::AdminGravestoneMutation()){disabled();return;}if(JsonValue(request.body,"csrf")!=s.csrf){Respond(fd,403,"application/json; charset=utf-8",Status::ErrorJson("Invalid CSRF token"));return;}
        const std::string operation=JsonValue(request.body,"action"),ownerId=JsonValue(request.body,"playerId"),graveId=JsonValue(request.body,"graveId"),itemId=JsonValue(request.body,"itemId");int quantity=0;JsonInteger(request.body,"quantity",quantity);auto opaque=[](const std::string&v){return v.size()==16&&v.find_first_not_of("0123456789abcdefABCDEF")==std::string::npos;};
        const bool itemAction=operation=="claim_item_owner"||operation=="claim_item_self"||operation=="delete_item";if(ownerId.empty()||!opaque(graveId)||(itemAction&&(!opaque(itemId)||quantity<1))){Respond(fd,400,"application/json; charset=utf-8",Status::ErrorJson("Invalid gravestone request"));return;}StoredPlayer owner;if(!playerStore->Find(ownerId,owner)||owner.identity.rfind("character:",0)!=0){Respond(fd,404,"application/json; charset=utf-8",Status::ErrorJson("Player not found"));return;}
        const std::string rateKey=adminStore->SessionHashPrefix(raw);auto now=std::chrono::steady_clock::now();auto rate=adminGravestoneRate.find(rateKey);if(rate!=adminGravestoneRate.end()&&now-rate->second<std::chrono::milliseconds(400)){Respond(fd,429,"application/json; charset=utf-8",Status::ErrorJson("Please wait before changing a gravestone again"));return;}adminGravestoneRate[rateKey]=now;std::string result;bool ok=DomEngine::SubmitAdminGravestoneMutation(operation,s.characterGuid,s.publicId,owner.identity.substr(10),ownerId,graveId,itemId,quantity,rateKey,result);adminStore->Audit(s,graveId,"gravestone:"+operation+":"+(itemId.empty()?"all":itemId)+":"+std::to_string(quantity)+":"+(ok?"accepted":"rejected")+":"+result);if(ok)Respond(fd,200,"application/json; charset=utf-8","{\"updated\":true,\"message\":\""+JsonEscape(result)+"\"}");else{auto d=DomEngine::GetAdminGravestoneDiagnostics();Respond(fd,d.compatibility==1?409:503,"application/json; charset=utf-8",Status::ErrorJson(result));}
    } else if(path=="/api/admin/gravestone/teleport"){
        if(!requirePost())return;AdminSession s;std::string raw;if(!authenticate(s,raw))return;if(!ZanikConfig::AdminTeleport()){disabled();return;}if(JsonValue(request.body,"csrf")!=s.csrf){Respond(fd,403,"application/json; charset=utf-8",Status::ErrorJson("Invalid CSRF token"));return;}const std::string ownerId=JsonValue(request.body,"playerId"),graveId=JsonValue(request.body,"graveId");StoredPlayer admin,owner;if(!playerStore->Find(s.publicId,admin)||!admin.online){Respond(fd,409,"application/json; charset=utf-8",Status::ErrorJson("Linked character is offline"));return;}if(!playerStore->Find(ownerId,owner)||owner.identity.rfind("character:",0)!=0){Respond(fd,404,"application/json; charset=utf-8",Status::ErrorJson("Player not found"));return;}uintptr_t actor=0;FVector3d where{};if(!DomEngine::ResolveAdminGravestoneLocation(owner.identity.substr(10),graveId,actor,where)){Respond(fd,409,"application/json; charset=utf-8",Status::ErrorJson("Gravestone is no longer available"));return;}const std::string rateKey=adminStore->SessionHashPrefix(raw);auto now=std::chrono::steady_clock::now();auto rate=adminTeleportRate.find(rateKey);if(rate!=adminTeleportRate.end()&&now-rate->second<std::chrono::seconds(3)){Respond(fd,429,"application/json; charset=utf-8",Status::ErrorJson("Please wait before teleporting again"));return;}adminTeleportRate[rateKey]=now;if(!DomEngine::QueueAdminGravestoneTeleport(s.characterGuid,s.publicId,owner.identity.substr(10),ownerId,graveId,rateKey)){auto diagnostics=DomEngine::GetAdminTeleportDiagnostics();const bool unavailable=diagnostics.compatibility!=1||diagnostics.groundTraceCompatibility!=1||diagnostics.storagePipelineCompatibility!=1||diagnostics.nativeCaptureCompatibility!=1||diagnostics.nativeTemplateAvailable!=1||diagnostics.storageTransportCompatibility!=1;Respond(fd,unavailable?503:409,"application/json; charset=utf-8",Status::ErrorJson(unavailable?"Gravestone teleport compatibility unavailable":"A teleport is already pending"));return;}adminStore->Audit(s,graveId,"gravestone-teleport-queued");Respond(fd,202,"application/json; charset=utf-8","{\"accepted\":true,\"status\":\"queued\"}");
    } else if (path == "/assets/world-map.jpg") {
        std::string asset;
        if(ReadWebAsset("world-map.jpg",asset))Respond(fd,200,"image/jpeg",asset);
        else Respond(fd,200,"image/svg+xml",PlaceholderImage());
    } else if (path == "/assets/lodestone-icon.png") {
        std::string asset;
        if(ReadWebAsset("lodestone-icon.png",asset))Respond(fd,200,"image/png",asset);
        else Respond(fd,200,"image/svg+xml",PlaceholderImage());
    } else if (path == "/assets/nodestone-icon.png") {
        std::string asset;
        if(ReadWebAsset("nodestone-icon.png",asset))Respond(fd,200,"image/png",asset);
        else Respond(fd,200,"image/svg+xml",PlaceholderImage());
    } else if (path == "/assets/gravestone-icon.png") {
        std::string asset;
        if(ReadWebAsset("gravestone-icon.png",asset))Respond(fd,200,"image/png",asset);
        else Respond(fd,200,"image/svg+xml",PlaceholderImage());
    } else if (path == "/assets/Jancient-Regular.ttf") {
        std::string asset;
        if(ReadWebAsset("Jancient-Regular.ttf",asset))Respond(fd,200,"font/ttf",asset);
        else Respond(fd,503,"application/json; charset=utf-8",Status::ErrorJson("Map font unavailable"));
    } else if (path.rfind("/assets/resources/",0)==0) {
        std::string asset;
        if(ReadResourceAsset(path.substr(18),asset))Respond(fd,200,"image/png",asset);
        else Respond(fd,200,"image/svg+xml",PlaceholderImage());
    } else if (path.rfind("/assets/items/",0)==0) {
        std::string asset;
        if(ReadItemAsset(path.substr(14),asset))Respond(fd,200,"image/png",asset);
        else Respond(fd,200,"image/svg+xml",PlaceholderImage());
    } else if (path.rfind("/assets/skills/",0)==0) {
        std::string asset;
        if(ReadSkillAsset(path.substr(15),asset))Respond(fd,200,"image/png",asset);
        else Respond(fd,200,"image/svg+xml",PlaceholderImage());
    } else if (path.rfind("/assets/buildings/",0)==0) {
        std::string asset;
        if(ReadBuildingAsset(path.substr(18),asset))Respond(fd,200,"image/png",asset);
        else Respond(fd,200,"image/svg+xml",PlaceholderImage());
    } else if (path.rfind("/assets/npcs/",0)==0) {
        std::string asset;
        if(ReadNpcAsset(path.substr(13),asset))Respond(fd,200,"image/png",asset);
        else Respond(fd,200,"image/svg+xml",PlaceholderImage());
    } else if (path.rfind("/assets/recipe-icons/",0)==0) {
        std::string asset;
        if(ReadRecipeIconAsset(path.substr(21),asset))Respond(fd,200,"image/png",asset);
        else Respond(fd,200,"image/svg+xml",PlaceholderImage());
    } else if (path.rfind("/assets/quests/",0)==0) {
        std::string asset;const std::string name=path.substr(15);if(ReadQuestAsset(name,asset))Respond(fd,200,name.size()>=4&&name.substr(name.size()-4)==".svg"?"image/svg+xml":"image/png",asset);else Respond(fd,200,"image/svg+xml",PlaceholderImage());
    } else if (path == "/api/quests/catalog") {
        if(!ZanikConfig::PlayerQuestTracking()){unavailable();return;}std::string asset;if(ReadWebAsset("quest-catalog.json",asset))Respond(fd,200,"application/json; charset=utf-8",asset);else Respond(fd,503,"application/json; charset=utf-8",Status::ErrorJson("Quest catalog unavailable"));
    } else if (path == "/api/map/resources/catalog") {
        if(!ZanikConfig::ResourceTracking()){unavailable();return;}
        std::string asset;
        if(ReadWebAsset("resource-catalog.json",asset))Respond(fd,200,"application/json; charset=utf-8",asset);
        else Respond(fd,503,"application/json; charset=utf-8",Status::ErrorJson("Resource catalog unavailable"));
    } else if (path == "/api/map/resources/state") {
        if(!ZanikConfig::ResourceTracking()){unavailable();return;}
        std::vector<DomEngine::ResourceStateInfo> states;double rate=0;std::int64_t timestamp=0;
        if(!DomEngine::ReadResourceStates(states,rate,timestamp))Respond(fd,503,"application/json; charset=utf-8",Status::ErrorJson("Resource state unavailable"));
        else Respond(fd,200,"application/json; charset=utf-8",Status::MapResourceStatesJson(states,rate,timestamp));
    } else if (path == "/api/map/npcs/catalog") {
        if(!ZanikConfig::NpcTracking()){unavailable();return;}
        std::string asset;
        if(ReadWebAsset("npc-catalog.json",asset))Respond(fd,200,"application/json; charset=utf-8",asset);
        else Respond(fd,503,"application/json; charset=utf-8",Status::ErrorJson("NPC catalog unavailable"));
    } else if (path == "/api/map/npcs/state") {
        if(!ZanikConfig::NpcTracking()){unavailable();return;}
        std::vector<DomEngine::NpcStateInfo> states;double rate=0;std::int64_t timestamp=0;
        if(!DomEngine::ReadNpcStates(states,rate,timestamp))Respond(fd,503,"application/json; charset=utf-8",Status::ErrorJson("NPC state unavailable"));
        else Respond(fd,200,"application/json; charset=utf-8",Status::MapNpcStatesJson(states,rate,timestamp));
    } else if (path == "/api/recipe-unlocks") {
        std::string catalog;auto health=ReadRecipeCatalogHealth(&catalog);
        if(health.compatibility==1)Respond(fd,200,"application/json; charset=utf-8",catalog);
        else Respond(fd,503,"application/json; charset=utf-8",Status::ErrorJson("Recipe unlock catalog unavailable"));
    } else if (path == "/api/players") {
        if(!ZanikConfig::PlayerStatus())unavailable();else Respond(fd, 200, "application/json; charset=utf-8", Status::PlayersJson(playerStore->Online()));
    } else if (path == "/api/known-players") {
        if(!ZanikConfig::PlayerStatus())unavailable();else Respond(fd, 200, "application/json; charset=utf-8", Status::PlayersJson(playerStore->All()));
    } else if (path == "/api/map/players") {
        if(!ZanikConfig::Map()){unavailable();return;}
        if(!DomEngine::g_Engine||!DomEngine::g_Engine->IsInitialized()||
           !DomEngine::g_Engine->IsMapProjectionAvailable()) {
            Respond(fd,503,"application/json; charset=utf-8",Status::ErrorJson("Live map unavailable"));
        } else {
            auto timestamp=std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            Respond(fd,200,"application/json; charset=utf-8",Status::MapPlayersJson(
                playerStore->Online(),DomEngine::g_Engine->GetMapAssetVersion(),timestamp));
        }
    } else if (path == "/api/map/lodestones") {
        std::vector<DomEngine::LodestoneInfo> lodestones;
        if(!DomEngine::ReadLodestones(lodestones)) {
            Respond(fd,503,"application/json; charset=utf-8",Status::ErrorJson("Lodestone map unavailable"));
        } else {
            auto timestamp=std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            Respond(fd,200,"application/json; charset=utf-8",Status::MapLodestonesJson(
                lodestones,DomEngine::g_Engine->GetMapAssetVersion(),timestamp));
        }
    } else if (path == "/api/map/nodestones") {
        std::vector<DomEngine::NodestoneInfo> nodestones;
        if(!DomEngine::ReadNodestones(nodestones)) {
            Respond(fd,503,"application/json; charset=utf-8",Status::ErrorJson("Nodestone map unavailable"));
        } else {
            auto timestamp=std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            Respond(fd,200,"application/json; charset=utf-8",Status::MapNodestonesJson(
                nodestones,DomEngine::g_Engine->GetMapAssetVersion(),timestamp));
        }
    } else if (path == "/api/map/labels") {
        static std::vector<DomEngine::MapLabelInfo> labels;
        static auto lastAttempt=std::chrono::steady_clock::time_point{};
        auto now=std::chrono::steady_clock::now();
        if(labels.empty()||now-lastAttempt>std::chrono::minutes(5)){
            lastAttempt=now;
            std::vector<DomEngine::MapLabelInfo> discovered;
            if(DomEngine::ReadMapLabels(discovered))labels=std::move(discovered);
        }
        if(labels.empty()){
            Respond(fd,503,"application/json; charset=utf-8",Status::ErrorJson("Map labels unavailable"));
        }else{
            auto timestamp=std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            Respond(fd,200,"application/json; charset=utf-8",Status::MapLabelsJson(
                labels,DomEngine::g_Engine->GetMapAssetVersion(),timestamp));
        }
    } else if (path == "/api/player") {
        if(!ZanikConfig::PlayerStatus()){unavailable();return;}
        std::string id = QueryValue(query, "id");
        if (id.empty()) {
            Respond(fd, 400, "application/json; charset=utf-8", Status::ErrorJson("Player id required"));
        } else {
            StoredPlayer player;
            if (!playerStore->Find(id, player)) {
                Respond(fd, 404, "application/json; charset=utf-8", Status::ErrorJson("Player not found"));
            } else {
                Respond(fd, 200, "application/json; charset=utf-8", Status::PlayerJson(player));
            }
        }
    } else if(path=="/api/player-activity"){
        if(!ZanikConfig::PlayerStatus()){unavailable();return;}
        const std::string id=QueryValue(query,"id");StoredPlayer player;
        if(id.empty())Respond(fd,400,"application/json; charset=utf-8",Status::ErrorJson("Player id required"));
        else if(!playerStore->Find(id,player)||player.identity.rfind("character:",0)!=0)Respond(fd,404,"application/json; charset=utf-8",Status::ErrorJson("Player not found"));
        else {std::size_t limit=8;std::uint64_t before=0,parsed=0;const auto rawLimit=QueryValue(query,"limit"),rawBefore=QueryValue(query,"before");if((!rawLimit.empty()&&(!ParseUnsigned(rawLimit,parsed)||parsed==0))||(!rawBefore.empty()&&!ParseUnsigned(rawBefore,before))){Respond(fd,400,"application/json; charset=utf-8",Status::ErrorJson("Invalid activity cursor"));return;}if(!rawLimit.empty())limit=static_cast<std::size_t>(std::min<std::uint64_t>(parsed,50));Respond(fd,200,"application/json; charset=utf-8",playerActivityStore?playerActivityStore->PageJson(player.identity.substr(10),limit,before):"{\"events\":[],\"hasMore\":false,\"nextBefore\":null}");}
    } else if(path=="/api/player-activity-diagnostics"){
        auto store=playerActivityStore?playerActivityStore->Diagnostics():PlayerActivityDiagnostics{};auto engine=DomEngine::GetPlayerActivityDiscoveryDiagnostics();std::ostringstream out;out<<"{\"compatibility\":"<<store.compatibility<<",\"regionCompatibility\":"<<engine.regionCompatibility<<",\"materialCompatibility\":"<<engine.materialCompatibility<<",\"events\":"<<store.events<<",\"players\":"<<store.players<<",\"snapshots\":"<<store.snapshots+engine.snapshots<<",\"droppedSamples\":"<<store.droppedSamples+engine.droppedSamples<<",\"persistenceFailures\":"<<store.persistenceFailures<<'}';Respond(fd,200,"application/json; charset=utf-8",out.str());
    } else if (path == "/api/player-recipes") {
        if(!ZanikConfig::PlayerRecipeTracking()){unavailable();return;}
        const std::string id=QueryValue(query,"id");StoredPlayer player;
        if(id.empty()){Respond(fd,400,"application/json; charset=utf-8",Status::ErrorJson("Player id required"));}
        else if(!playerStore->Find(id,player)||player.identity.rfind("character:",0)!=0){Respond(fd,404,"application/json; charset=utf-8",Status::ErrorJson("Player not found"));}
        else {auto diagnostics=DomEngine::GetPlayerRecipeDiagnostics();auto dispatcher=DomEngine::GetAdminStorageDiagnostics().dispatcherCompatibility;StoredRecipeSnapshot snapshot;const bool found=recipeSnapshotStore&&recipeSnapshotStore->Find(player.identity.substr(10),snapshot);const std::string currentFingerprint=DomEngine::GetPlayerRecipeCatalogFingerprint();std::vector<std::string> unknown;std::unordered_set<std::string> currentIds;bool removed=false;if(diagnostics.compatibility!=1||dispatcher!=1){Respond(fd,503,"application/json; charset=utf-8",Status::ErrorJson("Player recipe tracking is unavailable for this game build"));}else if(!found||!snapshot.complete||snapshot.unlockedIds.empty()){Respond(fd,503,"application/json; charset=utf-8","{\"error\":\"Recipe data has not been captured for this character yet\",\"captured\":false,\"online\":"+std::string(player.online?"true":"false")+"}");}else if(!RecipeCatalogUnknownIds(snapshot.catalogFingerprint,currentFingerprint,unknown,&currentIds,&removed)){Respond(fd,503,"application/json; charset=utf-8","{\"error\":\"This character's recipe snapshot is incompatible with the current game catalog\",\"captured\":false,\"online\":"+std::string(player.online?"true":"false")+"}");}else{std::vector<std::string> unlocked;for(const auto&id:snapshot.unlockedIds)if(currentIds.empty()||currentIds.count(id))unlocked.push_back(id);std::ostringstream out;out<<"{\"captured\":true,\"online\":"<<(player.online?"true":"false")<<",\"capturedAt\":"<<snapshot.capturedAt<<",\"catalogFingerprint\":\""<<JsonEscape(snapshot.catalogFingerprint)<<"\",\"compatibility\":\""<<(removed?"changed":(unknown.empty()?"exact":"additive"))<<"\",\"unlockedCount\":"<<unlocked.size()<<",\"unlocked\":[";for(size_t i=0;i<unlocked.size();++i){if(i)out<<',';out<<'\"'<<JsonEscape(unlocked[i])<<'\"';}out<<"],\"unknown\":[";for(size_t i=0;i<unknown.size();++i){if(i)out<<',';out<<'\"'<<unknown[i]<<'\"';}out<<"]}";Respond(fd,200,"application/json; charset=utf-8",out.str());}}
    } else if (path == "/api/player-recipe-diagnostics") {
        auto d=DomEngine::GetPlayerRecipeDiagnostics();auto dispatcher=DomEngine::GetAdminStorageDiagnostics().dispatcherCompatibility;Respond(fd,200,"application/json; charset=utf-8","{\"compatibility\":"+std::to_string(d.compatibility)+",\"dispatcherCompatibility\":"+std::to_string(dispatcher)+",\"catalogCompatibility\":"+std::to_string(d.catalogCompatibility)+",\"catalogHistoryCompatibility\":"+std::to_string(RecipeCatalogHistoryCompatibility())+",\"progressLayoutCompatibility\":"+std::to_string(d.progressLayoutCompatibility)+",\"queryCompatibility\":"+std::to_string(d.queryCompatibility)+",\"catalogEntries\":"+std::to_string(d.catalogEntries)+",\"resolvedEntries\":"+std::to_string(d.resolvedEntries)+",\"snapshots\":"+std::to_string(d.snapshots)+",\"scansCompleted\":"+std::to_string(d.scansCompleted)+",\"queryFailures\":"+std::to_string(d.queryFailures)+",\"lastScan\":"+std::to_string(d.lastScan)+",\"maximumProgressLookupMicroseconds\":"+std::to_string(d.maximumProgressLookupMicroseconds)+",\"maximumSingleQueryMicroseconds\":"+std::to_string(d.maximumSingleQueryMicroseconds)+",\"slowQueries\":"+std::to_string(d.slowQueries)+",\"queryIntervalMilliseconds\":"+std::to_string(d.queryIntervalMilliseconds)+"}");
    } else if (path == "/api/quests/steps") {
        std::string asset;if(ReadWebAsset("quest-step-locations.json",asset))Respond(fd,200,"application/json; charset=utf-8",asset);else Respond(fd,503,"application/json; charset=utf-8",Status::ErrorJson("Quest step locations unavailable"));
    } else if (path == "/api/player-quests") {
        if(!ZanikConfig::PlayerQuestTracking()){unavailable();return;}
        const std::string id=QueryValue(query,"id");StoredPlayer player;if(id.empty())Respond(fd,400,"application/json; charset=utf-8",Status::ErrorJson("Player id required"));else if(!playerStore->Find(id,player)||player.identity.rfind("character:",0)!=0)Respond(fd,404,"application/json; charset=utf-8",Status::ErrorJson("Player not found"));else{auto d=DomEngine::GetPlayerQuestDiagnostics();auto dispatcher=DomEngine::GetAdminStorageDiagnostics().dispatcherCompatibility;StoredQuestSnapshot snapshot;bool found=questSnapshotStore&&questSnapshotStore->Find(player.identity.substr(10),snapshot);if(d.compatibility!=1||dispatcher!=1)Respond(fd,503,"application/json; charset=utf-8","{\"error\":\"Player quest tracking is unavailable for this game build\",\"captured\":false,\"online\":"+std::string(player.online?"true":"false")+"}");else if(!found||!snapshot.complete)Respond(fd,503,"application/json; charset=utf-8","{\"error\":\"Quest data has not been captured for this character yet\",\"captured\":false,\"online\":"+std::string(player.online?"true":"false")+"}");else if(snapshot.catalogFingerprint!=DomEngine::GetPlayerQuestCatalogFingerprint())Respond(fd,503,"application/json; charset=utf-8","{\"error\":\"This character's quest snapshot is incompatible with the current catalog\",\"captured\":false,\"online\":"+std::string(player.online?"true":"false")+"}");else{size_t done=0,activeCount=0;for(const auto&q:snapshot.quests){done+=q.status=="completed";activeCount+=q.status=="in_progress";}std::ostringstream out;out<<"{\"captured\":true,\"online\":"<<(player.online?"true":"false")<<",\"capturedAt\":"<<snapshot.capturedAt<<",\"catalogFingerprint\":\""<<JsonEscape(snapshot.catalogFingerprint)<<"\",\"counts\":{\"total\":"<<snapshot.quests.size()<<",\"completed\":"<<done<<",\"inProgress\":"<<activeCount<<",\"notStarted\":"<<(snapshot.quests.size()-done-activeCount)<<"},\"quests\":[";for(size_t i=0;i<snapshot.quests.size();++i){if(i)out<<',';const auto&q=snapshot.quests[i];out<<"{\"id\":\""<<JsonEscape(q.id)<<"\",\"status\":\""<<JsonEscape(q.status)<<"\",\"objective\":\""<<JsonEscape(q.objective)<<"\",\"completedObservedAt\":"<<q.completedObservedAt<<'}';}out<<"]}";Respond(fd,200,"application/json; charset=utf-8",out.str());}}
    } else if (path == "/api/quest-tracker-diagnostics") {
        auto d=DomEngine::GetPlayerQuestDiagnostics();auto dispatcher=DomEngine::GetAdminStorageDiagnostics().dispatcherCompatibility;std::ostringstream out;out<<"{\"enabled\":"<<d.enabled<<",\"compatibility\":"<<d.compatibility<<",\"dispatcherCompatibility\":"<<dispatcher<<",\"catalogCompatibility\":"<<d.catalogCompatibility<<",\"componentCompatibility\":"<<d.componentCompatibility<<",\"queryCompatibility\":"<<d.queryCompatibility<<",\"objectiveCompatibility\":"<<d.objectiveCompatibility<<",\"catalogEntries\":"<<d.catalogEntries<<",\"resolvedEntries\":"<<d.resolvedEntries<<",\"snapshots\":"<<d.snapshots<<",\"scansCompleted\":"<<d.scansCompleted<<",\"queryFailures\":"<<d.queryFailures<<",\"snapshotChanges\":"<<d.snapshotChanges<<",\"persistenceFailures\":"<<d.persistenceFailures<<",\"cacheHits\":"<<d.cacheHits<<",\"cacheMisses\":"<<d.cacheMisses<<",\"invalidIdentities\":"<<d.invalidIdentities<<",\"lastScan\":"<<d.lastScan<<",\"lastQueryMicroseconds\":"<<d.lastQueryMicroseconds<<",\"maximumQueryMicroseconds\":"<<d.maximumQueryMicroseconds<<",\"slowQueries\":"<<d.slowQueries<<'}';Respond(fd,200,"application/json; charset=utf-8",out.str());
    } else if (path == "/api/kills") {
        if(!ZanikConfig::KillTracking()){unavailable();return;}Respond(fd, 200, "application/json; charset=utf-8",
                Status::KillSummaryJson(playerStore->All()));
    } else if (path == "/api/kill-leaderboard") {
        Respond(fd, 200, "application/json; charset=utf-8",
                Status::KillLeaderboardJson(playerStore->All(), QueryValue(query, "enemy")));
    } else if (path == "/api/skills") {
        if(!ZanikConfig::PlayerStatus()){unavailable();return;}Respond(fd, 200, "application/json; charset=utf-8",
                Status::SkillSummaryJson(playerStore->All()));
    } else if (path == "/api/skill-leaderboard") {
        if(!ZanikConfig::PlayerStatus()){unavailable();return;}Respond(fd, 200, "application/json; charset=utf-8",
                Status::SkillLeaderboardJson(playerStore->All(), QueryValue(query, "skill")));
    } else if (path == "/api/kill-tracker-diagnostics") {
        auto d = DomEngine::GetKillDiagnostics();
        Respond(fd, 200, "application/json; charset=utf-8",
            "{\"hookCompatibility\":" + std::to_string(d.hookCompatibility) +
            ",\"regionCompatibility\":" + std::to_string(d.regionCompatibility) +
            ",\"rawFatalEvents\":" + std::to_string(d.rawFatalEvents) +
            ",\"acceptedKills\":" + std::to_string(d.acceptedKills) +
            ",\"duplicateEvents\":" + std::to_string(d.duplicateEvents) +
            ",\"unresolvedAttackers\":" + std::to_string(d.unresolvedAttackers) +
            ",\"rejectedNonPlayerDeaths\":" + std::to_string(d.rejectedNonPlayerDeaths) +
            ",\"droppedQueueEvents\":" + std::to_string(d.droppedQueueEvents) +
            ",\"maximumHookMicroseconds\":" + std::to_string(d.maximumHookMicroseconds) +
            ",\"slowHookEvents\":" + std::to_string(d.slowHookEvents) + "}");
    } else if (path == "/api/player-discovery-diagnostics") {
        auto d = DomEngine::GetPlayerDiscoveryDiagnostics();
        Respond(fd, 200, "application/json; charset=utf-8",
            "{\"syncAttempts\":" + std::to_string(d.syncAttempts) +
            ",\"gameStateValid\":" + std::to_string(d.gameStateValid) +
            ",\"playerArrayCount\":" + std::to_string(d.playerArrayCount) +
            ",\"readablePlayerStates\":" + std::to_string(d.readablePlayerStates) +
            ",\"dominionPlayerStates\":" + std::to_string(d.dominionPlayerStates) +
            ",\"characterGuids\":" + std::to_string(d.characterGuids) +
            ",\"characterNames\":" + std::to_string(d.characterNames) +
            ",\"spawnedPawns\":" + std::to_string(d.spawnedPawns) +
            ",\"worldLocations\":" + std::to_string(d.worldLocations) +
            ",\"regionLookupAttempts\":" + std::to_string(d.regionLookupAttempts) +
            ",\"playersProduced\":" + std::to_string(d.playersProduced) + "}");
    } else if (path == "/api/lodestone-diagnostics") {
        auto d=DomEngine::GetLodestoneDiagnostics();
        Respond(fd,200,"application/json; charset=utf-8",
            "{\"compatibility\":"+std::to_string(d.compatibility)+
            ",\"registryFound\":"+std::to_string(d.registryFound)+
            ",\"registryCount\":"+std::to_string(d.registryCount)+
            ",\"validMarkers\":"+std::to_string(d.validMarkers)+
            ",\"rejectedRecords\":"+std::to_string(d.rejectedRecords)+
            ",\"lastSuccessfulCollection\":"+std::to_string(d.lastSuccessfulCollection)+"}");
    } else if (path == "/api/nodestone-diagnostics") {
        auto d=DomEngine::GetNodestoneDiagnostics();
        Respond(fd,200,"application/json; charset=utf-8",
            "{\"compatibility\":"+std::to_string(d.compatibility)+
            ",\"actorsFound\":"+std::to_string(d.actorsFound)+
            ",\"validMarkers\":"+std::to_string(d.validMarkers)+
            ",\"rejectedRecords\":"+std::to_string(d.rejectedRecords)+
            ",\"lastSuccessfulCollection\":"+std::to_string(d.lastSuccessfulCollection)+"}");
    } else if (path == "/api/map-label-diagnostics") {
        auto d=DomEngine::GetMapLabelDiagnostics();
        Respond(fd,200,"application/json; charset=utf-8",
            "{\"components\":"+std::to_string(d.components)+
            ",\"textProperties\":"+std::to_string(d.textProperties)+
            ",\"readableTexts\":"+std::to_string(d.readableTexts)+
            ",\"actorOwners\":"+std::to_string(d.actorOwners)+
            ",\"validAnchors\":"+std::to_string(d.validAnchors)+"}");
    } else if (path == "/api/resource-diagnostics") {
        auto d=DomEngine::GetResourceDiagnostics();
        Respond(fd,200,"application/json; charset=utf-8",
            "{\"compatibility\":"+std::to_string(d.compatibility)+
            ",\"catalogNodes\":"+std::to_string(d.catalogNodes)+
            ",\"actionableCatalogNodes\":"+std::to_string(d.actionableCatalogNodes)+
            ",\"foliageCatalogNodes\":"+std::to_string(d.foliageCatalogNodes)+
            ",\"loadedComponents\":"+std::to_string(d.loadedComponents)+
            ",\"matchedLoadedNodes\":"+std::to_string(d.matchedLoadedNodes)+
            ",\"matchedActionableNodes\":"+std::to_string(d.matchedActionableNodes)+
            ",\"matchedFoliageNodes\":"+std::to_string(d.matchedFoliageNodes)+
            ",\"persistedStateNodes\":"+std::to_string(d.persistedStateNodes)+
            ",\"unresolvedNodes\":"+std::to_string(d.unresolvedNodes)+
            ",\"rejectedRecords\":"+std::to_string(d.rejectedRecords)+
            ",\"collectionEvents\":"+std::to_string(d.collectionEvents)+
            ",\"respawnEvents\":"+std::to_string(d.respawnEvents)+
            ",\"rawSetAmountCalls\":"+std::to_string(d.rawSetAmountCalls)+
            ",\"unresolvedHookCalls\":"+std::to_string(d.unresolvedHookCalls)+
            ",\"lastSuccessfulReconciliation\":"+std::to_string(d.lastSuccessfulReconciliation)+"}");
    } else if (path == "/api/diagnostics/npcs") {
        auto d=DomEngine::GetNpcDiagnostics();
        auto icons=ReadNpcIconHealth();
        Respond(fd,200,"application/json; charset=utf-8",
            "{\"enabled\":"+std::to_string(d.enabled)+",\"catalogCompatibility\":"+std::to_string(d.catalogCompatibility)+
            ",\"registryCompatibility\":"+std::to_string(d.registryCompatibility)+",\"identityCompatibility\":"+std::to_string(d.identityCompatibility)+
            ",\"transformCompatibility\":"+std::to_string(d.transformCompatibility)+",\"lifecycleCompatibility\":"+std::to_string(d.lifecycleCompatibility)+
            ",\"respawnCompatibility\":"+std::to_string(d.respawnCompatibility)+",\"dungeonExclusionCompatibility\":"+std::to_string(d.dungeonExclusionCompatibility)+
            ",\"iconCompatibility\":"+std::to_string(icons.compatibility)+",\"iconAssets\":"+std::to_string(icons.total)+",\"missingIconAssets\":"+std::to_string(icons.missing)+
            ",\"catalogNodes\":"+std::to_string(d.catalogNodes)+",\"loadedActors\":"+std::to_string(d.loadedActors)+
            ",\"matchedLiveNodes\":"+std::to_string(d.matchedLiveNodes)+",\"deadNodes\":"+std::to_string(d.deadNodes)+
            ",\"inactiveNodes\":"+std::to_string(d.inactiveNodes)+",\"unloadedNodes\":"+std::to_string(d.unloadedNodes)+
            ",\"loadedSpawnPoints\":"+std::to_string(d.loadedSpawnPoints)+",\"ambiguousMatches\":"+std::to_string(d.ambiguousMatches)+
            ",\"rejectedActors\":"+std::to_string(d.rejectedActors)+",\"lastSample\":"+std::to_string(d.lastSample)+
            ",\"scanInProgress\":"+std::to_string(d.scanInProgress)+",\"scannedObjects\":"+std::to_string(d.scannedObjects)+
            ",\"totalObjects\":"+std::to_string(d.totalObjects)+",\"completedScans\":"+std::to_string(d.completedScans)+
            ",\"lastScanMilliseconds\":"+std::to_string(d.lastScanMilliseconds)+",\"maximumSliceMicroseconds\":"+std::to_string(d.maximumSliceMicroseconds)+
            ",\"slowSliceCount\":"+std::to_string(d.slowSliceCount)+
            ",\"lastGenerationWorkMicroseconds\":"+std::to_string(d.lastGenerationWorkMicroseconds)+
            ",\"lastFinalizationMicroseconds\":"+std::to_string(d.lastFinalizationMicroseconds)+
            ",\"maximumFinalizationMicroseconds\":"+std::to_string(d.maximumFinalizationMicroseconds)+
            ",\"lastLiveRefreshMicroseconds\":"+std::to_string(d.lastLiveRefreshMicroseconds)+
            ",\"maximumLiveRefreshMicroseconds\":"+std::to_string(d.maximumLiveRefreshMicroseconds)+
            ",\"classCacheHits\":"+std::to_string(d.classCacheHits)+",\"classCacheMisses\":"+std::to_string(d.classCacheMisses)+
            ",\"invalidObjectSlots\":"+std::to_string(d.invalidObjectSlots)+",\"changedObjectSlots\":"+std::to_string(d.changedObjectSlots)+
            ",\"lastDutyCycleBasisPoints\":"+std::to_string(d.lastDutyCycleBasisPoints)+",\"idleMilliseconds\":"+std::to_string(d.idleMilliseconds)+"}");
    } else if (path == "/api/diagnostics/player-skills") {
        auto icons=ReadSkillIconHealth();
        Respond(fd,200,"application/json; charset=utf-8","{\"iconCompatibility\":"+std::to_string(icons.compatibility)+",\"iconAssets\":"+std::to_string(icons.total)+",\"missingIconAssets\":"+std::to_string(icons.missing)+"}");
    } else if (path == "/api/diagnostics/storage-plans") {
        auto storage=DomEngine::GetAdminStorageDiagnostics();
        Respond(fd,200,"application/json; charset=utf-8","{\"compatibility\":"+std::to_string(storage.planCatalogCompatibility)+",\"items\":"+std::to_string(storage.planCatalogItems)+"}");
    } else if (path == "/api/update-status") {
        DomEngine::RefreshAdminNodestoneTeleportCompatibility();
        auto d = DomEngine::GetKillDiagnostics();
        auto l = DomEngine::GetLodestoneDiagnostics();
        auto n = DomEngine::GetNodestoneDiagnostics();
        auto labels = DomEngine::GetMapLabelDiagnostics();
        auto resources = DomEngine::GetResourceDiagnostics();
        auto npcs = DomEngine::GetNpcDiagnostics();
        auto npcIcons = ReadNpcIconHealth();
        auto skillIcons = ReadSkillIconHealth();
        auto recipeCatalog = ReadRecipeCatalogHealth();
        auto recipeSettings = recipeSettingsStore->Diagnostics();
        auto players = DomEngine::GetPlayerDiscoveryDiagnostics();
        auto adminTeleport = DomEngine::GetAdminTeleportDiagnostics();
        auto adminStorage = DomEngine::GetAdminStorageDiagnostics();
        auto adminGraves = DomEngine::GetAdminGravestoneDiagnostics();
        auto adminAuth = adminStore->Diagnostics();
        bool adminAuthorizationCompatible = adminAuth.allowlistReadable && adminAuth.sessionStorageReadable && adminAuth.sessionStorageWritable && adminAuth.sessionStorageRestricted && adminAuth.lastSaveSucceeded && adminAuth.malformedSessionRecords == 0;
        bool adminConsoleCompatible = adminAuthorizationCompatible && !ConsoleSecret().empty();
        auto onlinePlayers = playerStore->Online().size();auto playerRecipes=DomEngine::GetPlayerRecipeDiagnostics();auto playerQuests=DomEngine::GetPlayerQuestDiagnostics();auto activity=playerActivityStore?playerActivityStore->Diagnostics():PlayerActivityDiagnostics{};auto activityDiscovery=DomEngine::GetPlayerActivityDiscoveryDiagnostics();
        bool playerCompatible = DomEngine::g_Engine && DomEngine::g_Engine->IsInitialized();
        bool mapCompatible = playerCompatible && DomEngine::g_Engine->IsMapProjectionAvailable();
        const bool recipeSettingsCompatible=recipeSettings.readable&&recipeSettings.writable&&recipeSettings.restricted&&recipeSettings.lastSaveSucceeded&&recipeSettings.malformed==0;
        auto featureState=[](bool enabled,bool compatible){return std::string("\"")+(enabled?(compatible?"compatible":"incompatible"):"disabled")+"\"";};
        const bool npcCompatible=npcs.catalogCompatibility==1&&npcs.registryCompatibility==1&&npcs.identityCompatibility==1&&npcs.transformCompatibility==1&&npcs.lifecycleCompatibility==1;
        const std::string featureStates=std::string("{")+
            "\"playerStatus\":"+featureState(ZanikConfig::PlayerStatus(),playerCompatible)+
            ",\"killTracking\":"+featureState(ZanikConfig::KillTracking(),d.hookCompatibility==1)+
            ",\"map\":"+featureState(ZanikConfig::Map(),mapCompatible)+
            ",\"resourceTracking\":"+featureState(ZanikConfig::ResourceTracking(),resources.compatibility==1)+
            ",\"npcTracking\":"+featureState(ZanikConfig::NpcTracking(),npcCompatible)+
            ",\"playerRecipeTracking\":"+featureState(ZanikConfig::PlayerRecipeTracking(),playerRecipes.compatibility==1&&adminStorage.dispatcherCompatibility==1)+
            ",\"playerQuestTracking\":"+featureState(ZanikConfig::PlayerQuestTracking(),playerQuests.compatibility==1&&adminStorage.dispatcherCompatibility==1)+
            ",\"adminRead\":"+featureState(ZanikConfig::AdminRead(),adminAuthorizationCompatible&&adminConsoleCompatible)+
            ",\"adminRecipeSettings\":"+featureState(ZanikConfig::AdminRecipeSettings(),recipeSettingsCompatible&&recipeCatalog.compatibility==1)+
            ",\"adminTeleport\":"+featureState(ZanikConfig::AdminTeleport(),adminTeleport.compatibility==1)+
            ",\"adminNodestoneTeleport\":"+featureState(ZanikConfig::AdminTeleport(),adminTeleport.nodestoneCompatibility==1)+
            ",\"adminStorageMutation\":"+featureState(ZanikConfig::AdminStorageMutation(),adminStorage.compatibility==1)+
            ",\"adminGravestoneMutation\":"+featureState(ZanikConfig::AdminGravestoneMutation(),adminGraves.compatibility==1)+
            ",\"qolAutoCompost\":"+featureState(ZanikConfig::QolAutoCompost(),false)+"}";
        Respond(fd, 200, "application/json; charset=utf-8",
            "{\"website\":true,\"configuration\":{\"valid\":"+std::string(!std::getenv("ZANIK_CONFIG_VALID")||std::strcmp(std::getenv("ZANIK_CONFIG_VALID"),"0")!=0?"true":"false")+",\"installMode\":\""+JsonEscape(std::getenv("ZANIK_INSTALL_MODE")?std::getenv("ZANIK_INSTALL_MODE"):"legacy")+"\"},\"features\":"+ZanikConfig::Json()+",\"featureStates\":"+featureStates+",\"assets\":"+AssetManifestJson()+",\"playerDataCompatibility\":" +
            std::string(playerCompatible ? "true" : "false") +
            ",\"killHookCompatibility\":" + std::to_string(d.hookCompatibility) +
            ",\"regionCompatibility\":" + std::to_string(d.regionCompatibility) +
            ",\"mapProjectionCompatibility\":" + std::string(mapCompatible ? "true" : "false") +
            ",\"mapLabelCount\":" + std::to_string(labels.validAnchors) +
            ",\"lodestoneCompatibility\":" + std::to_string(l.compatibility) +
            ",\"nodestoneCompatibility\":" + std::to_string(n.compatibility) +
            ",\"nodestoneMarkers\":" + std::to_string(n.validMarkers) +
            ",\"nodestoneTeleportCompatibility\":" + std::to_string(adminTeleport.nodestoneCompatibility) +
            ",\"nodestoneTeleportMaximumMicroseconds\":" + std::to_string(adminTeleport.maximumNodestoneExecutionMicroseconds) +
            ",\"nodestoneTeleportSlowCalls\":" + std::to_string(adminTeleport.slowNodestoneCalls) +
            ",\"resourceCompatibility\":" + std::to_string(resources.compatibility) +
            ",\"resourceCatalogNodes\":" + std::to_string(resources.catalogNodes) +
            ",\"npcCatalogCompatibility\":" + std::to_string(npcs.catalogCompatibility) +
            ",\"npcRegistryCompatibility\":" + std::to_string(npcs.registryCompatibility) +
            ",\"npcIdentityCompatibility\":" + std::to_string(npcs.identityCompatibility) +
            ",\"npcTransformCompatibility\":" + std::to_string(npcs.transformCompatibility) +
            ",\"npcLifecycleCompatibility\":" + std::to_string(npcs.lifecycleCompatibility) +
            ",\"npcRespawnCompatibility\":" + std::to_string(npcs.respawnCompatibility) +
            ",\"npcDungeonExclusionCompatibility\":" + std::to_string(npcs.dungeonExclusionCompatibility) +
            ",\"npcLastGenerationWorkMicroseconds\":" + std::to_string(npcs.lastGenerationWorkMicroseconds) +
            ",\"npcLastFinalizationMicroseconds\":" + std::to_string(npcs.lastFinalizationMicroseconds) +
            ",\"npcMaximumSliceMicroseconds\":" + std::to_string(npcs.maximumSliceMicroseconds) +
            ",\"npcSlowSliceCount\":" + std::to_string(npcs.slowSliceCount) +
            ",\"npcLastDutyCycleBasisPoints\":" + std::to_string(npcs.lastDutyCycleBasisPoints) +
            ",\"npcInvalidObjectSlots\":" + std::to_string(npcs.invalidObjectSlots) +
            ",\"npcChangedObjectSlots\":" + std::to_string(npcs.changedObjectSlots) +
            ",\"npcIdleMilliseconds\":" + std::to_string(npcs.idleMilliseconds) +
            ",\"npcIconCompatibility\":" + std::to_string(npcIcons.compatibility) +
            ",\"npcIconAssets\":" + std::to_string(npcIcons.total) +
            ",\"playerSkillIconCompatibility\":" + std::to_string(skillIcons.compatibility) +
            ",\"playerSkillIconAssets\":" + std::to_string(skillIcons.total) +
            ",\"playerSkillMissingIconAssets\":" + std::to_string(skillIcons.missing) +
            ",\"recipeUnlockCatalogCompatibility\":" + std::to_string(recipeCatalog.compatibility) +
            ",\"recipeUnlockCatalogRecipes\":" + std::to_string(recipeCatalog.recipes) +
            ",\"recipeUnlockDependencyEdges\":" + std::to_string(recipeCatalog.dependencyEdges) +
            ",\"recipeUnlockCycles\":" + std::to_string(recipeCatalog.cycles) +
            ",\"recipeUnlockUnknownTriggers\":" + std::to_string(recipeCatalog.unknownTriggers) +
            ",\"recipeUnlockCatalogGenerated\":" + std::to_string(recipeCatalog.generated) +
            ",\"recipeUnlockSettingsCompatibility\":" + std::to_string(recipeSettingsCompatible?1:-1) +
            ",\"recipeUnlockHiddenRoutes\":" + std::to_string(recipeSettings.disabled) +
            ",\"playerRecipeCompatibility\":"+std::to_string(playerRecipes.compatibility)+
            ",\"playerRecipeDispatcherCompatibility\":"+std::to_string(adminStorage.dispatcherCompatibility)+
            ",\"playerRecipeCatalogCompatibility\":"+std::to_string(playerRecipes.catalogCompatibility)+
            ",\"playerRecipeCatalogHistoryCompatibility\":"+std::to_string(RecipeCatalogHistoryCompatibility())+
            ",\"playerRecipeProgressLayoutCompatibility\":"+std::to_string(playerRecipes.progressLayoutCompatibility)+
            ",\"playerRecipeQueryCompatibility\":"+std::to_string(playerRecipes.queryCompatibility)+
            ",\"playerRecipeCatalogEntries\":"+std::to_string(playerRecipes.catalogEntries)+
            ",\"playerRecipeResolvedEntries\":"+std::to_string(playerRecipes.resolvedEntries)+
            ",\"playerRecipeSnapshots\":"+std::to_string(recipeSnapshotStore?recipeSnapshotStore->Count():0)+
            ",\"playerRecipeScansCompleted\":"+std::to_string(playerRecipes.scansCompleted)+
            ",\"playerRecipeQueryFailures\":"+std::to_string(playerRecipes.queryFailures)+
            ",\"playerRecipeLastScan\":"+std::to_string(playerRecipes.lastScan)+
            ",\"playerRecipeMaximumProgressLookupMicroseconds\":"+std::to_string(playerRecipes.maximumProgressLookupMicroseconds)+
            ",\"playerRecipeMaximumSingleQueryMicroseconds\":"+std::to_string(playerRecipes.maximumSingleQueryMicroseconds)+
            ",\"playerRecipeSlowQueries\":"+std::to_string(playerRecipes.slowQueries)+
            ",\"playerRecipeQueryIntervalMilliseconds\":"+std::to_string(playerRecipes.queryIntervalMilliseconds)+
            ",\"playerQuestEnabled\":"+std::to_string(playerQuests.enabled)+",\"playerQuestCompatibility\":"+std::to_string(playerQuests.compatibility)+",\"playerQuestDispatcherCompatibility\":"+std::to_string(adminStorage.dispatcherCompatibility)+",\"playerQuestCatalogCompatibility\":"+std::to_string(playerQuests.catalogCompatibility)+",\"playerQuestComponentCompatibility\":"+std::to_string(playerQuests.componentCompatibility)+",\"playerQuestQueryCompatibility\":"+std::to_string(playerQuests.queryCompatibility)+",\"playerQuestCatalogEntries\":"+std::to_string(playerQuests.catalogEntries)+",\"playerQuestResolvedEntries\":"+std::to_string(playerQuests.resolvedEntries)+",\"playerQuestSnapshots\":"+std::to_string(questSnapshotStore?questSnapshotStore->Count():0)+",\"playerQuestScansCompleted\":"+std::to_string(playerQuests.scansCompleted)+",\"playerQuestQueryFailures\":"+std::to_string(playerQuests.queryFailures)+",\"playerQuestMaximumQueryMicroseconds\":"+std::to_string(playerQuests.maximumQueryMicroseconds)+",\"playerQuestSlowQueries\":"+std::to_string(playerQuests.slowQueries)+
            ",\"playerActivityCompatibility\":"+std::to_string(activity.compatibility)+",\"playerActivityRegionCompatibility\":"+std::to_string(activityDiscovery.regionCompatibility)+",\"playerActivityMaterialCompatibility\":"+std::to_string(activityDiscovery.materialCompatibility)+",\"playerActivityEvents\":"+std::to_string(activity.events)+",\"playerActivitySnapshots\":"+std::to_string(activity.snapshots+activityDiscovery.snapshots)+",\"playerActivityDroppedSamples\":"+std::to_string(activity.droppedSamples+activityDiscovery.droppedSamples)+",\"playerActivityPersistenceFailures\":"+std::to_string(activity.persistenceFailures)+
            ",\"adminAuthorizationStorage\":" + std::string(adminAuthorizationCompatible ? "true" : "false") +
            ",\"adminConsoleCompatibility\":" + std::to_string(adminConsoleCompatible ? 1 : 0) +
            ",\"adminTeleportCompatibility\":" + std::to_string(adminTeleport.compatibility) +
            ",\"adminTeleportGroundTraceCompatibility\":" + std::to_string(adminTeleport.groundTraceCompatibility) +
            ",\"adminTeleportTickCalls\":" + std::to_string(adminTeleport.tickCalls) +
            ",\"adminTeleportMaximumTickMicroseconds\":" + std::to_string(adminTeleport.maximumTickMicroseconds) +
            ",\"adminTeleportSlowTickCalls\":" + std::to_string(adminTeleport.slowTickCalls) +
            ",\"adminStorageTeleportCompatibility\":" + std::to_string(adminTeleport.storagePipelineCompatibility) +
            ",\"adminStorageTeleportTransportCompatibility\":" + std::to_string(adminTeleport.storageTransportCompatibility) +
            ",\"adminStorageTeleportMaximumNativeMicroseconds\":" + std::to_string(adminTeleport.maximumStorageTransportMicroseconds) +
            ",\"adminStorageTeleportSlowNativeCalls\":" + std::to_string(adminTeleport.slowStorageTransportCalls) +
            ",\"adminStorageCompatibility\":" + std::to_string(adminStorage.compatibility) +
            ",\"adminStorageDispatcherCompatibility\":" + std::to_string(adminStorage.dispatcherCompatibility) +
            ",\"adminEngineTickCalls\":" + std::to_string(adminStorage.tickCalls) +
            ",\"adminEngineMaximumTickMicroseconds\":" + std::to_string(adminStorage.maximumTickMicroseconds) +
            ",\"adminEngineSlowTickCalls\":" + std::to_string(adminStorage.slowTickCalls) +
            ",\"adminRecipeMaximumTickMicroseconds\":" + std::to_string(adminStorage.maximumRecipeTickMicroseconds) +
            ",\"adminCustomNameMaximumTickMicroseconds\":" + std::to_string(adminStorage.maximumCustomNameTickMicroseconds) +
            ",\"adminStorageMaximumTickMicroseconds\":" + std::to_string(adminStorage.maximumStorageTickMicroseconds) +
            ",\"adminStorageSnapshotCompatibility\":" + std::to_string(adminStorage.snapshotCompatibility) +
            ",\"adminStorageSingleItemCompatibility\":" + std::to_string(adminStorage.singleItemCompatibility) +
            ",\"adminStorageSlotMovementCompatibility\":" + std::to_string(adminStorage.slotMovementCompatibility) +
            ",\"adminStorageBatchAdditionCompatibility\":" + std::to_string(adminStorage.batchAdditionCompatibility) +
            ",\"adminStorageBatchMovementCompatibility\":" + std::to_string(adminStorage.batchMovementCompatibility) +
            ",\"adminStorageDeletionCompatibility\":" + std::to_string(adminStorage.deletionCompatibility) +
            ",\"adminStorageRollbackCompatibility\":" + std::to_string(adminStorage.rollbackCompatibility) +
            ",\"adminStorageSnapshotGenerations\":" + std::to_string(adminStorage.snapshotGenerations) +
            ",\"adminStorageSnapshotCacheHits\":" + std::to_string(adminStorage.snapshotCacheHits) +
            ",\"adminStorageSnapshotCacheMisses\":" + std::to_string(adminStorage.snapshotCacheMisses) +
            ",\"adminStorageSnapshotInvalidations\":" + std::to_string(adminStorage.snapshotInvalidations) +
            ",\"adminStorageMaximumSnapshotPreparationMicroseconds\":" + std::to_string(adminStorage.maximumSnapshotPreparationMicroseconds) +
            ",\"adminStorageQueueDepth\":" + std::to_string(adminStorage.queueDepth) +
            ",\"adminStorageQueueDrops\":" + std::to_string(adminStorage.queueDrops) +
            ",\"adminStorageRunningJobs\":" + std::to_string(adminStorage.runningJobs) +
            ",\"adminStorageCompletedJobs\":" + std::to_string(adminStorage.completedJobs) +
            ",\"adminStorageFailedJobs\":" + std::to_string(adminStorage.failedJobs) +
            ",\"adminStorageStaleIdentities\":" + std::to_string(adminStorage.staleIdentities) +
            ",\"adminStorageWorldChanges\":" + std::to_string(adminStorage.worldChanges) +
            ",\"adminStorageRollbackAttempts\":" + std::to_string(adminStorage.rollbackAttempts) +
            ",\"adminStorageRollbackFailures\":" + std::to_string(adminStorage.rollbackFailures) +
            ",\"adminStorageCircuitBreakers\":" + std::to_string(adminStorage.circuitBreakers) +
            ",\"adminStorageSingleItemMaximumMicroseconds\":" + std::to_string(adminStorage.singleItemMaximumMicroseconds) +
            ",\"adminStorageSlotMovementMaximumMicroseconds\":" + std::to_string(adminStorage.slotMovementMaximumMicroseconds) +
            ",\"adminStorageBatchAdditionMaximumMicroseconds\":" + std::to_string(adminStorage.batchAdditionMaximumMicroseconds) +
            ",\"adminStorageBatchMovementMaximumMicroseconds\":" + std::to_string(adminStorage.batchMovementMaximumMicroseconds) +
            ",\"adminStorageDeletionMaximumMicroseconds\":" + std::to_string(adminStorage.deletionMaximumMicroseconds) +
            ",\"adminStorageRenameMaximumMicroseconds\":" + std::to_string(adminStorage.renameMaximumMicroseconds) +
            ",\"adminStorageRollbackMaximumMicroseconds\":" + std::to_string(adminStorage.rollbackMaximumMicroseconds) +
            ",\"adminGravestoneMaximumTickMicroseconds\":" + std::to_string(adminStorage.maximumGravestoneTickMicroseconds) +
            ",\"adminStorageBatchCompatibility\":" + std::to_string(adminStorage.batchCompatibility) +
            ",\"adminStorageRenameCompatibility\":" + std::to_string(adminStorage.renameCompatibility) +
            ",\"adminStorageRemovalCompatibility\":" + std::to_string(adminStorage.removalTrackingCompatibility) +
            ",\"adminStorageUnlockCatalogCompatibility\":" + std::to_string(adminStorage.unlockCatalogCompatibility) +
            ",\"adminStoragePlanCatalogCompatibility\":" + std::to_string(adminStorage.planCatalogCompatibility) +
            ",\"adminStoragePlanCatalogItems\":" + std::to_string(adminStorage.planCatalogItems) +
            ",\"adminBuildingTaxonomyCompatibility\":" + std::to_string(adminStorage.buildingTaxonomyCompatibility) +
            ",\"adminBuildingRequirementsCompatibility\":" + std::to_string(adminStorage.buildingRequirementsCompatibility) +
            ",\"adminBuildingIconsCompatibility\":" + std::to_string(adminStorage.buildingIconsCompatibility) +
            ",\"adminCraftingRecipeCompatibility\":" + std::to_string(adminStorage.craftingRecipeCompatibility) +
            ",\"adminBuildingExpectedPieces\":" + std::to_string(adminStorage.expectedBuildingPieces) +
            ",\"adminBuildingResolvedPieces\":" + std::to_string(adminStorage.resolvedBuildingPieces) +
            ",\"adminBuildingUnavailablePieces\":" + std::to_string(adminStorage.unavailableBuildingPieces) +
            ",\"adminBuildingTabs\":" + std::to_string(adminStorage.buildingTabs) +
            ",\"adminBuildingSections\":" + std::to_string(adminStorage.buildingSections) +
            ",\"adminBuildingReferences\":" + std::to_string(adminStorage.buildingReferences) +
            ",\"adminGravestoneCompatibility\":" + std::to_string(adminGraves.compatibility) +
            ",\"adminGravestoneRegistryCompatibility\":" + std::to_string(adminGraves.registryCompatibility) +
            ",\"adminGravestoneStreamingCompatibility\":" + std::to_string(adminGraves.streamingCompatibility) +
            ",\"adminGravestonePersistenceCompatibility\":" + std::to_string(adminGraves.persistenceCompatibility) +
            ",\"onlinePlayers\":" + std::to_string(onlinePlayers) +
            ",\"spawnedPawns\":" + std::to_string(players.spawnedPawns) +
            ",\"worldLocations\":" + std::to_string(players.worldLocations) +
            ",\"updater\":" + UpdateStateJson() +
            ",\"gameUpdate\":" + GameUpdateStateJson() +
            ",\"backupRetention\":" + BackupRetentionStateJson() + "}");
    } else {
        Respond(fd, 404, "application/json; charset=utf-8", "{\"error\":\"Not found\"}");
    }
}

void Run() {
    const char* testHost=std::getenv("DW_STATUS_TEST_HOST");
    if(MaintenanceMode()){
        int port=Port();if(!port){LogMessage("DW_STATUS_PORT is required (1-65535)");return;}
        int listener=socket(AF_INET,SOCK_STREAM,0);if(listener<0){LogMessage("Could not create maintenance HTTP socket");return;}
        int reuse=1;setsockopt(listener,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));sockaddr_in address{};address.sin_family=AF_INET;address.sin_port=htons(static_cast<uint16_t>(port));address.sin_addr.s_addr=INADDR_ANY;
        if(bind(listener,reinterpret_cast<sockaddr*>(&address),sizeof(address))!=0||listen(listener,16)!=0){LogMessage("Could not bind/listen on maintenance port "+std::to_string(port)+": "+std::strerror(errno));close(listener);return;}
        LogMessage("Zanik maintenance-only website listening; no native game integration was initialized");
        while(!stopping){fd_set set;FD_ZERO(&set);FD_SET(listener,&set);timeval wait{0,100000};if(select(listener+1,&set,nullptr,nullptr,&wait)<=0)continue;int client=accept(listener,nullptr,nullptr);if(client<0)continue;timeval io{5,0};setsockopt(client,SOL_SOCKET,SO_RCVTIMEO,&io,sizeof(io));setsockopt(client,SOL_SOCKET,SO_SNDTIMEO,&io,sizeof(io));Handle(client);close(client);}close(listener);return;
    }
    const bool needsGameThreadDispatcher=ZanikConfig::PlayerRecipeTracking()||ZanikConfig::PlayerQuestTracking()||ZanikConfig::AdminTeleport()||ZanikConfig::AdminStorageMutation()||ZanikConfig::AdminGravestoneMutation();
    const bool storageTickReady=needsGameThreadDispatcher&&(!testHost||std::strcmp(testHost,"1")!=0)&&DomEngine::InitializeAdminStorageHookEarly();
    if((!testHost||std::strcmp(testHost,"1")!=0)&&needsGameThreadDispatcher)LogMessage(storageTickReady?"Zanik bounded game-thread dispatcher installed before world startup":"Zanik game-thread dispatcher compatibility unavailable");
    int port = Port();
    if (!port) { LogMessage("DW_STATUS_PORT is required (1-65535)"); return; }
    Mem::Init();
    DomEngine::g_Engine = new DomEngine::Engine();
    playerStore = std::make_unique<PlayerStore>(StorePath());
    playerStore->Load();
    adminStore = std::make_unique<AdminStore>(ZanikRoot()+"/admins.txt",DataRoot()+"/admin-sessions.db");
    adminStore->Load();
    const char* recipeSettingsPath=std::getenv("DW_RECIPE_UNLOCK_SETTINGS");
    recipeSettingsStore=std::make_unique<RecipeSettingsStore>(recipeSettingsPath&&*recipeSettingsPath?recipeSettingsPath:DataRoot()+"/recipe-unlock-settings.db");
    recipeSettingsStore->Load();
    recipeSnapshotStore=std::make_unique<RecipeSnapshotStore>(DataRoot()+"/player-recipe-snapshots.db");
    recipeSnapshotStore->Load();
    questSnapshotStore=std::make_unique<QuestSnapshotStore>(DataRoot()+"/player-quest-snapshots.db");questSnapshotStore->Load();
    playerActivityStore=std::make_unique<PlayerActivityStore>(DataRoot()+"/player-activity.db");playerActivityStore->Load();
    // Migrate names confirmed from records created before native FText lookup.
    // Future internal names are migrated automatically when first observed.
    playerStore->RenameEnemy("AI GiantRat Character", "Giant Rat");
    playerStore->RenameEnemy("AI RangedGoblin Character", "Goblin Archer");
    playerStore->RenameEnemy("AI RuntGoblin Character", "Goblin Runt");

    int listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) { LogMessage("Could not create HTTP socket"); return; }
    int reuse = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(static_cast<uint16_t>(port));
    address.sin_addr.s_addr = INADDR_ANY;
    if (bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0 ||
        listen(listener, 16) != 0) {
        LogMessage("Could not bind/listen on port " + std::to_string(port) + ": " + std::strerror(errno));
        close(listener);
        return;
    }
    LogMessage("Public status page listening on TCP port " + std::to_string(port));
    auto nextDiscovery = std::chrono::steady_clock::now();
    auto nextSync = std::chrono::steady_clock::now();
    bool nativeKillHookInstalled = false;
    bool nativeKillHookAttempted = false;
    bool adminTeleportInitialized = false;
    bool adminStorageInitialized = false;
    bool adminStorageAttempted = false;
    bool resourceTrackingInitialized = false;
    bool npcTrackingInitialized = false;
    bool npcTrackingAttempted = false;
    bool playerRecipeTrackingAttempted=false;
    bool playerQuestTrackingAttempted=false;
    auto nextResourceInit = std::chrono::steady_clock::now();
    auto nextResourceRefresh = std::chrono::steady_clock::now();
    auto nextNpcInit = std::chrono::steady_clock::now();
    while (!stopping) {
        auto now = std::chrono::steady_clock::now();
        if ((!testHost||std::strcmp(testHost,"1")!=0) && !DomEngine::g_Engine->IsInitialized() && now >= nextDiscovery) {
            DomEngine::g_Engine->Initialize();
            nextDiscovery = now + std::chrono::seconds(5);
        }
        if (ZanikConfig::KillTracking() && DomEngine::g_Engine->IsInitialized() && !nativeKillHookAttempted) {
            nativeKillHookAttempted = true;
            nativeKillHookInstalled = DomEngine::InstallNativeKillHook(
                DomEngine::g_Engine->GetModuleBase(), DomEngine::g_Engine->GetModuleEnd());
            LogMessage(nativeKillHookInstalled ? "Native AI kill hook installed" :
                                              "Native AI kill hook signature mismatch");
        }
        if(ZanikConfig::AdminTeleport()&&DomEngine::g_Engine->IsInitialized()&&!adminTeleportInitialized){
            adminTeleportInitialized=DomEngine::InitializeAdminTeleport(DomEngine::g_Engine->GetModuleBase(),DomEngine::g_Engine->GetModuleEnd());
            LogMessage(adminTeleportInitialized?"Admin lodestone teleport game-thread hook installed":"Admin lodestone teleport compatibility unavailable");
        }
        if((ZanikConfig::AdminStorageMutation()||ZanikConfig::AdminGravestoneMutation()||ZanikConfig::AdminTeleport())&&DomEngine::g_Engine->IsInitialized()&&!adminStorageAttempted){adminStorageAttempted=true;adminStorageInitialized=DomEngine::InitializeAdminStorage(DomEngine::g_Engine->GetModuleBase(),DomEngine::g_Engine->GetModuleEnd());LogMessage(adminStorageInitialized?"Admin storage compatibility verified":"Admin storage compatibility unavailable");if(ZanikConfig::AdminGravestoneMutation()){const bool graves=DomEngine::InitializeAdminGravestones(DomEngine::g_Engine->GetModuleBase(),DomEngine::g_Engine->GetModuleEnd());LogMessage(graves?"Admin gravestone compatibility verified":"Admin gravestone compatibility unavailable");}}
        if(ZanikConfig::PlayerRecipeTracking()&&DomEngine::g_Engine->IsInitialized()&&!playerRecipeTrackingAttempted){playerRecipeTrackingAttempted=true;const bool recipes=DomEngine::InitializePlayerRecipeTracking(WebRoot()+"/recipe-runtime-catalog.tsv",DomEngine::g_Engine->GetModuleBase(),DomEngine::g_Engine->GetModuleEnd());LogMessage(recipes?"Player recipe tracking compatibility verified":"Player recipe tracking compatibility unavailable");}
        if(ZanikConfig::PlayerRecipeTracking()&&DomEngine::g_Engine->IsInitialized()&&playerRecipeTrackingAttempted)DomEngine::RefreshPlayerRecipeTrackingCompatibility();
        if(ZanikConfig::PlayerQuestTracking()&&DomEngine::g_Engine->IsInitialized()&&!playerQuestTrackingAttempted){playerQuestTrackingAttempted=true;const bool quests=DomEngine::InitializePlayerQuestTracking(WebRoot()+"/quest-runtime-catalog.tsv",DomEngine::g_Engine->GetModuleBase(),DomEngine::g_Engine->GetModuleEnd());LogMessage(quests?"Player quest tracking compatibility verified":"Player quest tracking unavailable or waiting for quest objects");}
        if(ZanikConfig::PlayerQuestTracking()&&DomEngine::g_Engine->IsInitialized()&&playerQuestTrackingAttempted)DomEngine::RefreshPlayerQuestTrackingCompatibility();
        uintptr_t activeWorld=DomEngine::g_Engine->IsInitialized()?DomEngine::g_Engine->GetWorld():0;
        const bool mainWorldReady=activeWorld&&DomEngine::g_Engine->GetObjectName(activeWorld).find("L_World")!=std::string::npos;
        if(ZanikConfig::ResourceTracking()&&mainWorldReady&&!resourceTrackingInitialized&&now>=nextResourceInit){
            resourceTrackingInitialized=DomEngine::InitializeResourceTracking(
                DomEngine::g_Engine->GetModuleBase(),DomEngine::g_Engine->GetModuleEnd());
            LogMessage(resourceTrackingInitialized?"World resource tracking initialized":"World resource tracking compatibility unavailable");
            nextResourceInit=now+std::chrono::seconds(5);
        }
        if(ZanikConfig::ResourceTracking()&&mainWorldReady&&resourceTrackingInitialized&&now>=nextResourceRefresh){DomEngine::RefreshResourceStates();nextResourceRefresh=now+std::chrono::seconds(2);}
        if(ZanikConfig::NpcTracking()&&mainWorldReady&&!npcTrackingAttempted&&now>=nextNpcInit){
            npcTrackingAttempted=true;
            npcTrackingInitialized=DomEngine::InitializeNpcTracking(DomEngine::g_Engine->GetModuleBase(),DomEngine::g_Engine->GetModuleEnd());
            nextNpcInit=now+std::chrono::seconds(5);
        }
        if(ZanikConfig::NpcTracking()&&mainWorldReady&&npcTrackingInitialized)DomEngine::RefreshNpcStates();
        if (nativeKillHookInstalled) {
            auto events = DomEngine::DrainNativeKillEvents();
            for (const auto& event : events) {
                auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
                if (event.internalEnemy != event.enemy)
                    playerStore->RenameEnemy(event.internalEnemy, event.enemy);
                if (playerStore->RecordKill(event.killerCharacterGuid, event.enemy, timestamp))
                    LogMessage("Recorded native kill: " + event.enemy);
            }
        }
        if (ZanikConfig::PlayerStatus() && DomEngine::g_Engine->IsInitialized() && now >= nextSync) {
            uintptr_t gameState = DomEngine::g_Engine->GetGameState();
            if (gameState && DomEngine::g_Engine->IsA(gameState, "DominionGameStateBase")) {
                auto livePlayers=DomEngine::g_Engine->GetAllPlayers();
                DomEngine::UpdateKillPlayerCache(livePlayers);
                if(ZanikConfig::PlayerRecipeTracking())DomEngine::UpdatePlayerRecipeTargets(livePlayers);
                if(ZanikConfig::PlayerQuestTracking())DomEngine::UpdatePlayerQuestTargets(livePlayers);
                auto recipeSnapshots=ZanikConfig::PlayerRecipeTracking()?DomEngine::ReadPlayerRecipeSnapshots():std::vector<DomEngine::PlayerRecipeSnapshot>{};
                auto questSnapshots=ZanikConfig::PlayerQuestTracking()?DomEngine::ReadPlayerQuestSnapshots():std::vector<DomEngine::PlayerQuestSnapshot>{};
                auto discoverySnapshots=DomEngine::ReadPlayerActivityDiscoveries(livePlayers);
                if(playerActivityStore)playerActivityStore->Sync(livePlayers,recipeSnapshots,questSnapshots,discoverySnapshots);
                playerStore->Sync(livePlayers);
                if(ZanikConfig::PlayerRecipeTracking()&&recipeSnapshotStore)recipeSnapshotStore->Sync(recipeSnapshots);
                if(ZanikConfig::PlayerQuestTracking()&&questSnapshotStore&&!questSnapshotStore->Sync(questSnapshots))DomEngine::RecordPlayerQuestPersistenceFailure();
            } else DomEngine::UpdateKillPlayerCache({});
            nextSync = now + std::chrono::seconds(2);
        }
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(listener, &readSet);
        // During NPC discovery, resume small bounded slices frequently enough
        // to discover newly streamed cells within a few seconds. Each slice
        // still yields before HTTP work can be starved.
        timeval wait{0, DomEngine::NpcScanInProgress()?10000:50000};
        int ready = select(listener + 1, &readSet, nullptr, nullptr, &wait);
        if (ready <= 0) continue;
        int client = accept(listener, nullptr, nullptr);
        if (client < 0) continue;
        // One slow or abandoned browser connection must not monopolize the
        // single HTTP worker and make every map/player request appear offline.
        timeval ioTimeout{5, 0};
        setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &ioTimeout, sizeof(ioTimeout));
        setsockopt(client, SOL_SOCKET, SO_SNDTIMEO, &ioTimeout, sizeof(ioTimeout));
        Handle(client);
        close(client);
    }
    playerStore->Save();
    if(recipeSnapshotStore)recipeSnapshotStore->Save();
    if(questSnapshotStore)questSnapshotStore->Save();
    if(playerActivityStore)playerActivityStore->Save();
    close(listener);
}
}

extern "C" __attribute__((constructor)) void DWStatusLoad() {
    char executable[4096]{};
    const ssize_t length=readlink("/proc/self/exe",executable,sizeof(executable)-1);
    if(length<=0)return;
    executable[length]='\0';
    const char* leaf=std::strrchr(executable,'/');
    leaf=leaf?leaf+1:executable;
    constexpr const char* serverPrefix="RSDragonwildsServer-Linux-Shipping";
    const char* testHost=std::getenv("DW_STATUS_TEST_HOST");
    if(std::strncmp(leaf,serverPrefix,std::strlen(serverPrefix))!=0&&(!testHost||std::strcmp(testHost,"1")!=0))return;
    worker = std::thread(Run);
}

extern "C" __attribute__((destructor)) void DWStatusUnload() {
    stopping = true;
    if (worker.joinable()) worker.join();
}

