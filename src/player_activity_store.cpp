#include "player_activity_store.h"
#include "status.h"
#include "utils/logger.h"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <fcntl.h>
#include <fstream>
#include <limits>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace {
std::int64_t Now(){return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();}
std::vector<std::string> Split(const std::string&v,char s){std::vector<std::string>o;size_t p=0;for(;;){size_t a=v.find(s,p);o.push_back(v.substr(p,a==std::string::npos?a:a-p));if(a==std::string::npos)return o;p=a+1;}}
std::string Hex(const std::string&v){static const char*d="0123456789abcdef";std::string o;o.reserve(v.size()*2);for(unsigned char c:v){o+=d[c>>4];o+=d[c&15];}return o;}
int N(char c){return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:-1;}
bool Unhex(const std::string&v,std::string&o){if(v.size()%2)return false;o.clear();o.reserve(v.size()/2);for(size_t i=0;i<v.size();i+=2){int a=N(v[i]),b=N(v[i+1]);if(a<0||b<0)return false;o+=char((a<<4)|b);}return true;}
bool Safe(const std::string&v){return !v.empty()&&v.size()<=128&&v.find_first_not_of("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_-.:")==std::string::npos;}
std::string Json(const std::string&v){std::ostringstream o;for(unsigned char c:v){switch(c){case '\\':o<<"\\\\";break;case '"':o<<"\\\"";break;case '\n':o<<"\\n";break;case '\r':o<<"\\r";break;case '\t':o<<"\\t";break;default:if(c<0x20)o<<"\\u00"<<"0123456789abcdef"[c>>4]<<"0123456789abcdef"[c&15];else o<<char(c);}}return o.str();}
std::string CleanInternal(std::string v){for(char&c:v)if(c=='_')c=' ';for(const char*p:{"RECIPE ","BUILDPIECE ","DA "})if(v.rfind(p,0)==0)v.erase(0,std::char_traits<char>::length(p));return v;}
std::string SkillIcon(const std::string& name){std::string key;for(unsigned char c:name)if(std::isalpha(c))key+=char(std::tolower(c));if(key=="crafting")key="artisan";return key.empty()?std::string{}:"/assets/skills/"+key+".png";}
bool JsonStringAt(const std::string&json,size_t start,std::string&out,size_t&after){out.clear();bool escaped=false;for(size_t i=start;i<json.size();++i){char c=json[i];if(escaped){switch(c){case 'n':out+='\n';break;case 'r':out+='\r';break;case 't':out+='\t';break;default:out+=c;}escaped=false;continue;}if(c=='\\'){escaped=true;continue;}if(c=='"'){after=i+1;return true;}out+=c;}return false;}
bool JsonFieldAfter(const std::string&json,const char*field,size_t start,std::string&out,size_t&after){std::string needle=std::string("\"")+field+"\":\"";size_t at=json.find(needle,start);return at!=std::string::npos&&JsonStringAt(json,at+needle.size(),out,after);}
int QuestRank(const std::string&status){return status=="not_started"?0:status=="in_progress"?1:status=="completed"?2:-1;}
}

PlayerActivityStore::PlayerActivityStore(std::string path):path_(std::move(path)){LoadCatalogs();}

bool PlayerActivityStore::LoadCatalogs(){
    const char* web=std::getenv("DW_WEB_ROOT");std::string webRoot=web&&*web?web:"/home/container/zanik/web";
    std::ifstream quests(webRoot+"/quest-runtime-catalog.tsv");std::string line;
    while(std::getline(quests,line)){if(line.empty()||line[0]=='#')continue;auto f=Split(line,'\t');if(f.size()>=2)questNames_[f[0]]=f[1];}
    std::ifstream recipes(webRoot+"/recipe-runtime-catalog.tsv");
    while(std::getline(recipes,line)){if(line.empty()||line[0]=='#')continue;auto f=Split(line,'\t');if(f.size()<3)continue;RecipeMeta m;m.name=f.size()>=4&&!f[3].empty()?f[3]:CleanInternal(f[2]);m.icon=f.size()>=5?f[4]:std::string{};m.group=f[1]+"\x1f"+m.name+"\x1f"+m.icon;recipes_[f[0]]=std::move(m);}
    std::ifstream publicCatalog(webRoot+"/recipe-unlocks.json");if(publicCatalog){std::ostringstream raw;raw<<publicCatalog.rdbuf();const std::string json=raw.str();size_t at=json.find("\"entries\":[");if(at!=std::string::npos){for(;;){at=json.find("{\"id\":\"",at+1);if(at==std::string::npos)break;std::string id,name,kind,icon;size_t next=0;if(!JsonFieldAfter(json,"id",at,id,next)||!JsonFieldAfter(json,"name",next,name,next)||!JsonFieldAfter(json,"kind",next,kind,next)||!JsonFieldAfter(json,"icon",next,icon,next))continue;auto found=recipes_.find(id);if(found==recipes_.end())continue;found->second.name=name;found->second.icon=icon;found->second.group=kind+"\x1f"+name+"\x1f"+icon;at=next;}}}
    return true;
}

std::string PlayerActivityStore::QuestName(const std::string&id)const{auto i=questNames_.find(id);return i==questNames_.end()?"Quest":i->second;}
std::string PlayerActivityStore::RecipeGroup(const std::string&id)const{auto i=recipes_.find(id);return i==recipes_.end()?id:i->second.group;}
std::string PlayerActivityStore::RecipeName(const std::string&id)const{auto i=recipes_.find(id);return i==recipes_.end()?"New recipe":i->second.name;}
std::string PlayerActivityStore::RecipeIcon(const std::string&id)const{auto i=recipes_.find(id);return i==recipes_.end()?std::string{}:i->second.icon;}

void PlayerActivityStore::Add(const std::string&g,std::int64_t t,const std::string&type,const std::string&title,const std::string&detail,const std::string&icon){events_.push_back({nextId_++,g,t,type,title,detail,icon});}

bool PlayerActivityStore::Sync(const std::vector<DomEngine::PlayerInfo>& players,
 const std::vector<DomEngine::PlayerRecipeSnapshot>& recipeSnapshots,
 const std::vector<DomEngine::PlayerQuestSnapshot>& questSnapshots,
 const std::vector<DomEngine::PlayerActivityDiscoverySnapshot>& discoveries,std::int64_t timestamp){
    if(!timestamp)timestamp=Now();
    bool changed=false;
    for(const auto&p:players){if(p.characterGuid.empty())continue;auto&b=baselines_[p.characterGuid];++snapshots_;
        std::map<std::string,int> levels;bool validSkills=!p.skills.empty();for(const auto&s:p.skills){if(s.skill.empty()||s.currentXP<0||!levels.emplace(s.skill,Status::SkillLevel(s.currentXP)).second){validSkills=false;break;}}
        if(validSkills&&b.skillsInitialized){for(const auto&old:b.skillLevels){auto current=levels.find(old.first);if(current==levels.end()||current->second<old.second){validSkills=false;break;}}}
        if(!validSkills&&b.skillsInitialized)++droppedSamples_;
        if(validSkills&&(!b.skillsInitialized||levels!=b.skillLevels)){
            if(b.skillsInitialized){
                for(const auto&s:levels){
                    auto old=b.skillLevels.find(s.first);
                    if(old==b.skillLevels.end()||s.second<=old->second)continue;
                    const int gained=s.second-old->second;
                    std::string name=s.first;const std::string prefix="SKILL_";
                    if(name.rfind(prefix,0)==0)name.erase(0,prefix.size());
                    if(!name.empty())name[0]=char(std::toupper(static_cast<unsigned char>(name[0])));
                    Add(p.characterGuid,timestamp,"skill_level","Reached "+name+" level "+std::to_string(s.second),
                        gained>1?"Gained "+std::to_string(gained)+" levels":std::string{},SkillIcon(name));
                }
            }else b.skillsInitialized=true;
            b.skillLevels=std::move(levels);changed=true;
        }
        if(!p.location.empty()&&p.location!=b.lastKnownLocation){b.lastKnownLocation=p.location;changed=true;}
        if(p.hasHealth&&(!b.deathInitialized||b.dead!=p.isDead)){if(b.deathInitialized&&!b.dead&&p.isDead){const std::string location=p.location.empty()?b.lastKnownLocation:p.location;Add(p.characterGuid,timestamp,"death","Died",location.empty()?std::string{}:"In "+location);}else if(!b.deathInitialized)b.deathInitialized=true;b.dead=p.isDead;changed=true;}
    }
    for(const auto&s:recipeSnapshots){if(!s.complete||s.characterGuid.empty()||s.catalogFingerprint.empty()){if(!s.characterGuid.empty())++droppedSamples_;continue;}auto&b=baselines_[s.characterGuid];++snapshots_;std::map<std::string,std::string> groups;for(const auto&id:s.unlockedIds)groups.emplace(RecipeGroup(id),id);std::set<std::string> current;for(const auto&p:groups)current.insert(p.first);
        if(!b.recipesInitialized||b.recipeFingerprint!=s.catalogFingerprint){b.recipesInitialized=true;b.recipeFingerprint=s.catalogFingerprint;b.recipeGroups=std::move(current);changed=true;continue;}
        if(!std::includes(current.begin(),current.end(),b.recipeGroups.begin(),b.recipeGroups.end())){++droppedSamples_;continue;}
        for(const auto&g:current)if(!b.recipeGroups.count(g)){const auto id=groups[g];Add(s.characterGuid,timestamp,"recipe_unlocked","Unlocked "+RecipeName(id),{},RecipeIcon(id));}
        if(current!=b.recipeGroups){b.recipeGroups=std::move(current);changed=true;}
    }
    for(const auto&s:questSnapshots){if(!s.complete||s.characterGuid.empty()||s.catalogFingerprint.empty()){if(!s.characterGuid.empty())++droppedSamples_;continue;}auto&b=baselines_[s.characterGuid];++snapshots_;std::map<std::string,QuestState> current;bool valid=true;for(const auto&q:s.quests)if(q.id.empty()||QuestRank(q.status)<0||!current.emplace(q.id,QuestState{q.status,q.objective}).second){valid=false;break;}if(!valid){++droppedSamples_;continue;}
        if(!b.questsInitialized||b.questFingerprint!=s.catalogFingerprint){b.questsInitialized=true;b.questFingerprint=s.catalogFingerprint;b.quests=std::move(current);changed=true;continue;}
        for(const auto&old:b.quests){auto now=current.find(old.first);if(now==current.end()||QuestRank(now->second.status)<QuestRank(old.second.status)){valid=false;break;}}
        if(!valid){++droppedSamples_;continue;}
        for(const auto&q:current){auto old=b.quests.find(q.first);if(old==b.quests.end())continue;const auto&before=old->second;const auto&after=q.second;const std::string name=QuestName(q.first);
            if(before.status!="completed"&&after.status=="completed")Add(s.characterGuid,timestamp,"quest_completed","Completed "+name);
            else if(before.status=="not_started"&&after.status=="in_progress")Add(s.characterGuid,timestamp,"quest_started","Started "+name,after.objective);
            else if(before.status=="in_progress"&&after.status=="in_progress"&&!before.objective.empty()&&!after.objective.empty()&&before.objective!=after.objective)Add(s.characterGuid,timestamp,"quest_progressed","Progressed "+name,after.objective);
        }
        for(auto&q:current){auto old=b.quests.find(q.first);if(q.second.status=="in_progress"&&q.second.objective.empty()&&old!=b.quests.end()&&old->second.status=="in_progress")q.second.objective=old->second.objective;}
        if(current!=b.quests){b.quests=std::move(current);changed=true;}
    }
    for(const auto&s:discoveries){if(s.characterGuid.empty())continue;auto&b=baselines_[s.characterGuid];++snapshots_;
        if(!s.regionsComplete)++droppedSamples_;
        else if(b.regionsInitialized&&(s.regionBitmap&b.regions)!=b.regions)++droppedSamples_;
        else if(!b.regionsInitialized||b.regions!=s.regionBitmap){if(!b.regionsInitialized){b.regionsInitialized=true;b.regions=s.regionBitmap;b.everRegions|=s.regionBitmap;changed=true;}else{std::uint32_t added=s.regionBitmap&~b.regions;for(const auto&r:s.regions)if(r.id<32&&(added&(1u<<r.id))&&!(b.everRegions&(1u<<r.id)))Add(s.characterGuid,timestamp,"region_discovered","Discovered "+r.name);b.regions=s.regionBitmap;b.everRegions|=s.regionBitmap;changed=true;}}
        if(!s.materialsComplete)++droppedSamples_;
        else {std::set<std::string> current;bool validMaterials=true;for(const auto&m:s.materials)if(m.key.empty()||!current.insert(m.key).second){validMaterials=false;break;}if(!validMaterials||(b.materialsInitialized&&!std::includes(current.begin(),current.end(),b.materials.begin(),b.materials.end())))++droppedSamples_;else if(!b.materialsInitialized){b.materialsInitialized=true;b.materials=current;b.everMaterials.insert(current.begin(),current.end());changed=true;}else if(current!=b.materials){for(const auto&m:s.materials)if(!b.materials.count(m.key)&&!b.everMaterials.count(m.key))Add(s.characterGuid,timestamp,"material_discovered","Discovered "+m.name,{},m.icon);b.materials=std::move(current);b.everMaterials.insert(b.materials.begin(),b.materials.end());changed=true;}}
    }
    if(changed)dirty_=true;
    if(!dirty_)return false;
    if(!Save()){++persistenceFailures_;persistenceHealthy_=false;return false;}
    dirty_=false;persistenceHealthy_=true;return true;
}

bool PlayerActivityStore::Save()const{
    const std::string tmp=path_+".tmp";std::ostringstream o;o<<"N\t"<<nextId_<<'\n';
    for(const auto&p:baselines_){const auto&g=p.first;const auto&b=p.second;o<<"B\t"<<g<<'\t'<<b.skillsInitialized<<'\t'<<b.recipesInitialized<<'\t'<<Hex(b.recipeFingerprint)<<'\t'<<b.questsInitialized<<'\t'<<Hex(b.questFingerprint)<<'\t'<<b.deathInitialized<<'\t'<<b.dead<<'\t'<<b.regionsInitialized<<'\t'<<b.regions<<'\t'<<b.everRegions<<'\t'<<b.materialsInitialized<<'\t'<<Hex(b.lastKnownLocation)<<'\n';for(const auto&s:b.skillLevels)o<<"S\t"<<g<<'\t'<<Hex(s.first)<<'\t'<<s.second<<'\n';for(const auto&r:b.recipeGroups)o<<"R\t"<<g<<'\t'<<Hex(r)<<'\n';for(const auto&q:b.quests)o<<"Q\t"<<g<<'\t'<<q.first<<'\t'<<Hex(q.second.status)<<'\t'<<Hex(q.second.objective)<<'\n';for(const auto&m:b.materials)o<<"M\t"<<g<<'\t'<<Hex(m)<<"\t0\n";for(const auto&m:b.everMaterials)o<<"M\t"<<g<<'\t'<<Hex(m)<<"\t1\n";}
    for(const auto&e:events_)o<<"E\t"<<e.id<<'\t'<<e.characterGuid<<'\t'<<e.timestamp<<'\t'<<Hex(e.type)<<'\t'<<Hex(e.title)<<'\t'<<Hex(e.detail)<<'\t'<<Hex(e.icon)<<'\n';
    const std::string bytes=o.str();int fd=::open(tmp.c_str(),O_WRONLY|O_CREAT|O_TRUNC,0600);if(fd<0)return false;if(fchmod(fd,0600)){::close(fd);return false;}size_t offset=0;while(offset<bytes.size()){ssize_t written=::write(fd,bytes.data()+offset,bytes.size()-offset);if(written<0&&errno==EINTR)continue;if(written<=0){::close(fd);return false;}offset+=static_cast<size_t>(written);}const bool synced=fsync(fd)==0;const bool closed=::close(fd)==0;if(!synced||!closed)return false;if(std::rename(tmp.c_str(),path_.c_str()))return false;return chmod(path_.c_str(),0600)==0;
}

bool PlayerActivityStore::Load(){if(chmod(path_.c_str(),0600)&&errno!=ENOENT){++persistenceFailures_;persistenceHealthy_=false;return false;}std::ifstream in(path_);if(!in)return true;std::string line;while(std::getline(in,line)){auto f=Split(line,'\t');try{if(f.size()==2&&f[0]=="N"){auto id=std::stoull(f[1]);if(id<std::numeric_limits<std::uint64_t>::max())nextId_=std::max<std::uint64_t>(1,id);}else if((f.size()==13||f.size()==14)&&f[0]=="B"&&Safe(f[1])){auto&b=baselines_[f[1]];b.skillsInitialized=f[2]=="1";Unhex(f[4],b.recipeFingerprint);b.recipesInitialized=f[3]=="1";b.questsInitialized=f[5]=="1";Unhex(f[6],b.questFingerprint);b.deathInitialized=f[7]=="1";b.dead=f[8]=="1";b.regionsInitialized=f[9]=="1";b.regions=std::stoul(f[10]);b.everRegions=std::stoul(f[11]);b.materialsInitialized=f[12]=="1";if(f.size()==14)Unhex(f[13],b.lastKnownLocation);}else if(f.size()==4&&f[0]=="S"&&baselines_.count(f[1])){std::string n;if(Unhex(f[2],n)&&n.size()<=256)baselines_[f[1]].skillLevels[n]=std::stoi(f[3]);}else if(f.size()==3&&f[0]=="R"&&baselines_.count(f[1])){std::string v;if(Unhex(f[2],v)&&v.size()<=2048)baselines_[f[1]].recipeGroups.insert(v);}else if(f.size()==5&&f[0]=="Q"&&baselines_.count(f[1])&&Safe(f[2])){std::string s,o;if(Unhex(f[3],s)&&Unhex(f[4],o)&&s.size()<=64&&o.size()<=2048)baselines_[f[1]].quests[f[2]]={s,o};}else if(f.size()==4&&f[0]=="M"&&baselines_.count(f[1])){std::string v;if(Unhex(f[2],v)&&v.size()<=256){if(f[3]=="0")baselines_[f[1]].materials.insert(v);else if(f[3]=="1")baselines_[f[1]].everMaterials.insert(v);}}else if(f.size()==8&&f[0]=="E"&&Safe(f[2])){PlayerActivityEvent e;e.id=std::stoull(f[1]);e.characterGuid=f[2];e.timestamp=std::stoll(f[3]);if(Unhex(f[4],e.type)&&Unhex(f[5],e.title)&&Unhex(f[6],e.detail)&&Unhex(f[7],e.icon)&&e.id&&e.id<std::numeric_limits<std::uint64_t>::max()&&e.timestamp>0&&e.timestamp<=4102444800LL&&e.type.size()<=64&&e.title.size()<=1024&&e.detail.size()<=2048&&e.icon.size()<=512)events_.push_back(std::move(e));}}catch(...){continue;}}
    std::sort(events_.begin(),events_.end(),[](const auto&a,const auto&b){return a.id<b.id;});
    events_.erase(std::unique(events_.begin(),events_.end(),[](const auto&a,const auto&b){return a.id==b.id;}),events_.end());
    for(const auto&e:events_)nextId_=std::max(nextId_,e.id+1);
    LogMessage("Loaded "+std::to_string(events_.size())+" player activity events");return true;}

PlayerActivityPage PlayerActivityStore::Page(const std::string&g,std::size_t limit,std::uint64_t before)const{PlayerActivityPage p;limit=std::max<std::size_t>(1,std::min<std::size_t>(limit,50));for(auto i=events_.rbegin();i!=events_.rend();++i){if(i->characterGuid!=g||(before&&i->id>=before))continue;if(p.events.size()==limit){p.hasMore=true;break;}p.events.push_back(*i);}if(!p.events.empty())p.nextBefore=p.events.back().id;return p;}
std::string PlayerActivityStore::PageJson(const std::string&g,std::size_t limit,std::uint64_t before)const{auto p=Page(g,limit,before);std::ostringstream o;o<<"{\"events\":[";for(size_t i=0;i<p.events.size();++i){if(i)o<<',';const auto&e=p.events[i];o<<"{\"id\":"<<e.id<<",\"type\":\""<<Json(e.type)<<"\",\"timestamp\":"<<e.timestamp<<",\"title\":\""<<Json(e.title)<<"\",\"detail\":\""<<Json(e.detail)<<"\",\"icon\":"<<(e.icon.empty()?"null":"\""+Json(e.icon)+"\"")<<'}';}o<<"],\"hasMore\":"<<(p.hasMore?"true":"false")<<",\"nextBefore\":"<<(p.nextBefore?std::to_string(p.nextBefore):"null")<<",\"persistenceError\":"<<(persistenceHealthy_?"false":"true")<<'}';return o.str();}
PlayerActivityDiagnostics PlayerActivityStore::Diagnostics()const{return {1,events_.size(),baselines_.size(),snapshots_,droppedSamples_,persistenceFailures_};}
