#include "player_activity_store.h"
#include "utils/runtime_paths.h"
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
constexpr const char* kRegionDiscoveryIcon="/assets/region-discovery-icon.png?v=places-selected-gold-1";
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
bool JsonFieldAfter(const std::string&json,const char*field,size_t start,std::string&out,size_t&after){std::string needle=std::string("\"")+field+"\"";size_t at=json.find(needle,start);if(at==std::string::npos)return false;at=json.find(':',at+needle.size());if(at==std::string::npos)return false;do ++at;while(at<json.size()&&std::isspace(static_cast<unsigned char>(json[at])));return at<json.size()&&json[at]=='\"'&&JsonStringAt(json,at+1,out,after);}
int QuestRank(const std::string&status){return status=="not_started"?0:status=="in_progress"?1:status=="completed"?2:-1;}
bool AuthoritativeRegion(const std::string&name){static const std::set<std::string> regions={"Temple Woods","Bramblemead Valley","Whispering Swamp","Fractured Plains","Stormtouched Highlands","Bloodblight Swamp","Bleakfields Valley","Hope's Fall","Lake of Lost Souls","Coalridge Pass","Silverthorn Keep","Dragons' Run","Witchwillow Range","Emberwood","Forgotten Temple","The Approach","The Library","The Courtyard","The Menagerie","The Garrison","The Nexus","The Pastures","The Cathedral","The Bastion","The Grand Hall","Alcarrid Oasis","Dunes of Uzzer","Manafem Plains","The Burning Spire","Rasthin's Rest","Scorned Wilderness"};return regions.count(name)!=0;}
}

PlayerActivityStore::PlayerActivityStore(std::string path):path_(std::move(path)){LoadCatalogs();}

bool PlayerActivityStore::LoadCatalogs(){
    const std::string webRoot=RuntimePaths::WebRoot();
    std::ifstream quests(webRoot+"/quest-runtime-catalog.tsv");std::string line;
    while(std::getline(quests,line)){if(line.empty()||line[0]=='#')continue;auto f=Split(line,'\t');if(f.size()>=2)quests_[f[0]].name=f[1];}
std::ifstream questCatalog(webRoot+"/quest-catalog.json");if(questCatalog){std::ostringstream raw;raw<<questCatalog.rdbuf();const std::string json=raw.str();size_t at=json.find("\"quests\"");if(at!=std::string::npos){for(;;){at=json.find("\"id\"",at+1);if(at==std::string::npos)break;std::string id,name,type,icon;size_t next=0;if(!JsonFieldAfter(json,"id",at,id,next)||!JsonFieldAfter(json,"name",next,name,next)||!JsonFieldAfter(json,"type",next,type,next)||!JsonFieldAfter(json,"icon",next,icon,next))continue;if(type!="main"&&type!="side")continue;auto&meta=quests_[id];meta.name=name;meta.icon=icon.empty()?(type=="main"?"/assets/quests/main-quest.png":"/assets/quests/side-quest.png"):icon;at=next;}}}
    std::ifstream recipes(webRoot+"/recipe-runtime-catalog.tsv");
    while(std::getline(recipes,line)){if(line.empty()||line[0]=='#')continue;auto f=Split(line,'\t');if(f.size()<3)continue;RecipeMeta m;m.name=f.size()>=4&&!f[3].empty()?f[3]:CleanInternal(f[2]);m.icon=f.size()>=5?f[4]:std::string{};m.group=f[1]+"\x1f"+m.name+"\x1f"+m.icon;recipes_[f[0]]=std::move(m);}
    std::ifstream publicCatalog(webRoot+"/recipe-unlocks.json");if(publicCatalog){std::ostringstream raw;raw<<publicCatalog.rdbuf();const std::string json=raw.str();size_t at=json.find("\"entries\":[");if(at!=std::string::npos){for(;;){at=json.find("{\"id\":\"",at+1);if(at==std::string::npos)break;std::string id,name,kind,icon;size_t next=0;if(!JsonFieldAfter(json,"id",at,id,next)||!JsonFieldAfter(json,"name",next,name,next)||!JsonFieldAfter(json,"kind",next,kind,next)||!JsonFieldAfter(json,"icon",next,icon,next))continue;auto found=recipes_.find(id);if(found==recipes_.end())continue;found->second.name=name;found->second.icon=icon;found->second.group=kind+"\x1f"+name+"\x1f"+icon;at=next;}}}
    return true;
}

std::string PlayerActivityStore::QuestName(const std::string&id)const{auto i=quests_.find(id);return i==quests_.end()||i->second.name.empty()?"Quest":i->second.name;}
std::string PlayerActivityStore::QuestIcon(const std::string&id)const{auto i=quests_.find(id);return i==quests_.end()?std::string{}:i->second.icon;}
std::string PlayerActivityStore::RecipeGroup(const std::string&id)const{auto i=recipes_.find(id);return i==recipes_.end()?id:i->second.group;}
std::string PlayerActivityStore::RecipeName(const std::string&id)const{auto i=recipes_.find(id);return i==recipes_.end()?"New recipe":i->second.name;}
std::string PlayerActivityStore::RecipeIcon(const std::string&id)const{auto i=recipes_.find(id);return i==recipes_.end()?std::string{}:i->second.icon;}

void PlayerActivityStore::Add(const std::string&g,std::int64_t t,const std::string&type,const std::string&title,const std::string&detail,const std::string&icon){events_.push_back({nextId_++,g,t,type,title,detail,icon});}

bool PlayerActivityStore::Sync(const std::vector<DomEngine::PlayerInfo>& players,
 const std::vector<DomEngine::PlayerRecipeSnapshot>& recipeSnapshots,
 const std::vector<DomEngine::PlayerQuestSnapshot>& questSnapshots,
 const std::vector<DomEngine::PlayerActivityDiscoverySnapshot>& discoveries,std::int64_t timestamp){
    if(!timestamp)timestamp=Now();
    bool changed=false;const std::size_t eventStart=events_.size();
    for(const auto&p:players){if(p.characterGuid.empty())continue;auto&b=baselines_[p.characterGuid];++snapshots_;
        auto arm=retests_.find(p.characterGuid);const bool skillRetest=arm!=retests_.end()&&(arm->second.family=="skill"||arm->second.family=="perk"||arm->second.family=="spell");
        std::map<std::string,int> levels;bool validSkills=!p.skills.empty();for(const auto&s:p.skills){if(s.skill.empty()||s.currentXP<0||!levels.emplace(s.skill,Status::SkillLevel(s.currentXP)).second){validSkills=false;break;}}
        if(validSkills&&b.skillsInitialized){for(const auto&old:b.skillLevels){auto current=levels.find(old.first);if(current==levels.end()||current->second<old.second){if(skillRetest&&old.first==arm->second.related)continue;validSkills=false;break;}}}
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
        if(skillRetest&&validSkills){auto current=b.skillLevels.find(arm->second.related);if(current!=b.skillLevels.end()&&current->second<arm->second.target&&arm->second.preparationConfirmed){arm->second.gameResetObserved=true;changed=true;}}
        if(!p.location.empty()&&p.location!=b.lastKnownLocation){b.lastKnownLocation=p.location;changed=true;}
        if(p.hasHealth&&(!b.deathInitialized||b.dead!=p.isDead)){if(b.deathInitialized&&!b.dead&&p.isDead){const std::string location=p.location.empty()?b.lastKnownLocation:p.location;Add(p.characterGuid,timestamp,"death","Died",location.empty()?std::string{}:"In "+location,"/assets/gravestone-icon.png");}else if(!b.deathInitialized)b.deathInitialized=true;b.dead=p.isDead;changed=true;}
    }
    for(const auto&s:recipeSnapshots){if(!s.complete||s.characterGuid.empty()||s.catalogFingerprint.empty()){if(!s.characterGuid.empty())++droppedSamples_;continue;}auto&b=baselines_[s.characterGuid];++snapshots_;auto arm=retests_.find(s.characterGuid);const bool progressRetest=arm!=retests_.end()&&(arm->second.family=="recipe"||arm->second.family=="material");std::map<std::string,std::string> groups;for(const auto&id:s.unlockedIds)groups.emplace(RecipeGroup(id),id);std::set<std::string> current;for(const auto&p:groups)current.insert(p.first);
        if(!b.recipesInitialized||b.recipeFingerprint!=s.catalogFingerprint){b.recipesInitialized=true;b.recipeFingerprint=s.catalogFingerprint;b.recipeGroups=std::move(current);b.everRecipeGroups.insert(b.recipeGroups.begin(),b.recipeGroups.end());changed=true;continue;}
        if(!std::includes(current.begin(),current.end(),b.recipeGroups.begin(),b.recipeGroups.end())&&!progressRetest){++droppedSamples_;continue;}
        for(const auto&g:current)if(!b.recipeGroups.count(g)&&!b.everRecipeGroups.count(g)){const auto id=groups[g];Add(s.characterGuid,timestamp,"recipe_unlocked","Unlocked "+RecipeName(id),{},RecipeIcon(id));}
        if(current!=b.recipeGroups){b.recipeGroups=std::move(current);b.everRecipeGroups.insert(b.recipeGroups.begin(),b.recipeGroups.end());changed=true;}
        if(progressRetest&&arm->second.family=="recipe"){const auto target=RecipeGroup(arm->second.key);if(!b.recipeGroups.count(target)){b.everRecipeGroups.erase(target);if(arm->second.preparationConfirmed){arm->second.gameResetObserved=true;arm->second.dependentResetObserved=true;}changed=true;}}
    }
    for(const auto&s:questSnapshots){if(!s.complete||s.characterGuid.empty()||s.catalogFingerprint.empty()){if(!s.characterGuid.empty())++droppedSamples_;continue;}auto&b=baselines_[s.characterGuid];++snapshots_;auto arm=retests_.find(s.characterGuid);const bool questRetest=arm!=retests_.end()&&arm->second.family=="quest";std::map<std::string,QuestState> current;bool valid=true;for(const auto&q:s.quests)if(q.id.empty()||QuestRank(q.status)<0||!current.emplace(q.id,QuestState{q.status,q.objective}).second){valid=false;break;}if(!valid){++droppedSamples_;continue;}
        if(!b.questsInitialized||b.questFingerprint!=s.catalogFingerprint){b.questsInitialized=true;b.questFingerprint=s.catalogFingerprint;b.quests=std::move(current);for(const auto&q:b.quests){if(q.second.status=="in_progress"||q.second.status=="completed")b.everQuestEvents.insert(q.first+"\x1f""started");if(q.second.status=="completed")b.everQuestEvents.insert(q.first+"\x1f""completed");}changed=true;continue;}
        for(const auto&old:b.quests){auto now=current.find(old.first);if(now==current.end()||QuestRank(now->second.status)<QuestRank(old.second.status)){if(questRetest)continue;valid=false;break;}}
        if(!valid){++droppedSamples_;continue;}
        for(const auto&q:current){auto old=b.quests.find(q.first);if(old==b.quests.end())continue;const auto&before=old->second;const auto&after=q.second;const std::string name=QuestName(q.first);
            const std::string icon=QuestIcon(q.first);
            const std::string completedKey=q.first+"\x1f""completed",startedKey=q.first+"\x1f""started",progressKey=q.first+"\x1f""progressed""\x1f"+after.objective;
            if(before.status!="completed"&&after.status=="completed"&&!b.everQuestEvents.count(completedKey)){Add(s.characterGuid,timestamp,"quest_completed","Completed "+name,{},icon);b.everQuestEvents.insert(completedKey);}
            else if(before.status=="not_started"&&after.status=="in_progress"&&!b.everQuestEvents.count(startedKey)){Add(s.characterGuid,timestamp,"quest_started","Started "+name,after.objective,icon);b.everQuestEvents.insert(startedKey);}
            else if(before.status=="in_progress"&&after.status=="in_progress"&&!before.objective.empty()&&!after.objective.empty()&&before.objective!=after.objective&&!b.everQuestEvents.count(progressKey)){Add(s.characterGuid,timestamp,"quest_progressed","Progressed "+name,after.objective,icon);b.everQuestEvents.insert(progressKey);}
        }
        for(auto&q:current){auto old=b.quests.find(q.first);if(q.second.status=="in_progress"&&q.second.objective.empty()&&old!=b.quests.end()&&old->second.status=="in_progress")q.second.objective=old->second.objective;}
        if(current!=b.quests){b.quests=std::move(current);changed=true;}
        if(questRetest){auto target=b.quests.find(arm->second.key);if(target==b.quests.end()||target->second.status=="not_started"){for(auto it=b.everQuestEvents.begin();it!=b.everQuestEvents.end();)if(it->rfind(arm->second.key+"\x1f",0)==0)it=b.everQuestEvents.erase(it);else ++it;if(arm->second.preparationConfirmed){arm->second.gameResetObserved=true;arm->second.dependentResetObserved=true;}changed=true;}}
    }
    for(const auto&s:discoveries){if(s.characterGuid.empty())continue;auto&b=baselines_[s.characterGuid];auto arm=retests_.find(s.characterGuid);const bool progressRetest=arm!=retests_.end()&&(arm->second.family=="recipe"||arm->second.family=="material");const bool regionRetest=arm!=retests_.end()&&arm->second.family=="region";const bool skillRetest=arm!=retests_.end()&&(arm->second.family=="skill"||arm->second.family=="perk"||arm->second.family=="spell");++snapshots_;
        if(!s.regionsComplete)++droppedSamples_;
        else if(b.regionsInitialized&&(s.regionBitmap&b.regions)!=b.regions&&!regionRetest)++droppedSamples_;
        else if(!b.regionsInitialized||b.regions!=s.regionBitmap){if(!b.regionsInitialized){b.regionsInitialized=true;b.regions=s.regionBitmap;b.everRegions|=s.regionBitmap;changed=true;}else{std::uint32_t added=s.regionBitmap&~b.regions;bool namesComplete=true;for(unsigned bit=0;bit<32;++bit)if((added&(1u<<bit))&&!(b.everRegions&(1u<<bit))){auto named=std::find_if(s.regions.begin(),s.regions.end(),[&](const auto&r){return r.id==bit&&!r.name.empty();});if(named==s.regions.end()){namesComplete=false;break;}Add(s.characterGuid,timestamp,"region_discovered","Discovered "+named->name,{},kRegionDiscoveryIcon);}if(namesComplete){b.regions=s.regionBitmap;b.everRegions|=s.regionBitmap;changed=true;}else ++droppedSamples_;}}
        if(regionRetest){unsigned bit=0;try{bit=unsigned(std::stoul(arm->second.key));}catch(...){bit=32;}if(bit<32&&!(b.regions&(std::uint32_t(1)<<bit))){b.everRegions&=~(std::uint32_t(1)<<bit);if(arm->second.preparationConfirmed){arm->second.gameResetObserved=true;arm->second.dependentResetObserved=true;}changed=true;}}
        if(!s.materialsComplete||s.materialFingerprint.empty())++droppedSamples_;
        else {std::set<std::string> current;bool validMaterials=true;for(const auto&m:s.materials)if(m.key.empty()||!current.insert(m.key).second){validMaterials=false;break;}if(!validMaterials)++droppedSamples_;else if(!b.materialsInitialized||b.materialFingerprint!=s.materialFingerprint){b.materialsInitialized=true;b.materialFingerprint=s.materialFingerprint;b.materials=current;b.everMaterials.insert(current.begin(),current.end());changed=true;}else if(!std::includes(current.begin(),current.end(),b.materials.begin(),b.materials.end())&&!progressRetest)++droppedSamples_;else if(current!=b.materials){for(const auto&m:s.materials)if(!b.materials.count(m.key)&&!b.everMaterials.count(m.key))Add(s.characterGuid,timestamp,"material_discovered","Discovered "+m.name,{},m.icon);b.materials=std::move(current);b.everMaterials.insert(b.materials.begin(),b.materials.end());changed=true;}}
        if(progressRetest&&arm->second.family=="material"&&s.materialsComplete){const bool present=std::any_of(s.materials.begin(),s.materials.end(),[&](const auto&m){return m.key==arm->second.key;});if(!present){b.materials.clear();for(const auto&m:s.materials)b.materials.insert(m.key);b.everMaterials.erase(arm->second.key);if(arm->second.preparationConfirmed){arm->second.gameResetObserved=true;arm->second.dependentResetObserved=true;}changed=true;}}
        if(!s.unlocksComplete||s.unlockFingerprint.empty())++droppedSamples_;
        else {std::set<std::string> perks,spells;bool valid=true;for(const auto&u:s.unlocks){auto&set=u.type=="perk"?perks:spells;if((u.type!="perk"&&u.type!="spell")||u.key.empty()||!set.insert(u.key).second){valid=false;break;}}if(!valid)++droppedSamples_;else if(!b.unlocksInitialized){b.unlocksInitialized=true;b.unlockFingerprint=s.unlockFingerprint;b.perks=perks;b.spells=spells;b.everPerks.insert(perks.begin(),perks.end());b.everSpells.insert(spells.begin(),spells.end());changed=true;}
        else if(b.unlockFingerprint!=s.unlockFingerprint){
            for(const auto&u:s.unlocks){const auto&ever=u.type=="perk"?b.everPerks:b.everSpells;if(ever.count(u.key)||u.skill.empty()||u.requiredLevel<1)continue;std::string skill=u.skill;const std::string prefix="SKILL_";if(skill.rfind(prefix,0)==0)skill.erase(0,prefix.size());if(!skill.empty())skill[0]=char(std::toupper(static_cast<unsigned char>(skill[0])));const std::string title="Reached "+skill+" level "+std::to_string(u.requiredLevel);const bool recentLevel=std::any_of(events_.rbegin(),events_.rend(),[&](const auto&e){return e.characterGuid==s.characterGuid&&e.type=="skill_level"&&e.title==title&&e.timestamp<=timestamp&&e.timestamp>=timestamp-21600;});if(recentLevel)Add(s.characterGuid,timestamp,u.type+"_unlocked","Unlocked "+u.name,{},u.icon);}
            b.unlockFingerprint=s.unlockFingerprint;b.perks=perks;b.spells=spells;b.everPerks.insert(perks.begin(),perks.end());b.everSpells.insert(spells.begin(),spells.end());changed=true;
        }else if((!std::includes(perks.begin(),perks.end(),b.perks.begin(),b.perks.end())||!std::includes(spells.begin(),spells.end(),b.spells.begin(),b.spells.end()))&&!progressRetest&&!skillRetest)++droppedSamples_;else if(perks!=b.perks||spells!=b.spells){for(const auto&u:s.unlocks){const auto&before=u.type=="perk"?b.perks:b.spells;const auto&ever=u.type=="perk"?b.everPerks:b.everSpells;if(!before.count(u.key)&&!ever.count(u.key))Add(s.characterGuid,timestamp,u.type+"_unlocked","Unlocked "+u.name,{},u.icon);}b.perks=std::move(perks);b.spells=std::move(spells);b.everPerks.insert(b.perks.begin(),b.perks.end());b.everSpells.insert(b.spells.begin(),b.spells.end());changed=true;}}
        if(skillRetest&&s.unlocksComplete){
            const bool present=std::any_of(s.unlocks.begin(),s.unlocks.end(),[&](const auto&u){return u.type==arm->second.family&&u.key==arm->second.key;});
            if(arm->second.family=="skill"){
                if(arm->second.preparationConfirmed&&arm->second.gameResetObserved&&!arm->second.dependentResetObserved){
                    for(auto it=b.everPerks.begin();it!=b.everPerks.end();)if(!b.perks.count(*it))it=b.everPerks.erase(it);else ++it;
                    for(auto it=b.everSpells.begin();it!=b.everSpells.end();)if(!b.spells.count(*it))it=b.everSpells.erase(it);else ++it;
                    arm->second.dependentResetObserved=true;changed=true;
                }
            }else if(!present){
                if(arm->second.family=="perk")b.everPerks.erase(arm->second.key);else b.everSpells.erase(arm->second.key);
                if(arm->second.preparationConfirmed&&arm->second.gameResetObserved)arm->second.dependentResetObserved=true;
                changed=true;
            }
        }
    }
    static const std::map<std::string,std::string> eventTypes={{"skill","skill_level"},{"perk","perk_unlocked"},{"spell","spell_unlocked"},{"recipe","recipe_unlocked"},{"material","material_discovered"},{"region","region_discovered"},{"death","death"}};
    for(std::size_t index=eventStart;index<events_.size();++index){const auto&event=events_[index];auto arm=retests_.find(event.characterGuid);if(arm==retests_.end())continue;std::string expected;if(arm->second.family=="quest")expected=arm->second.operation=="complete"?"quest_completed":arm->second.operation=="progress"?"quest_progressed":"quest_started";else{auto type=eventTypes.find(arm->second.family);if(type!=eventTypes.end())expected=type->second;}if(!expected.empty()&&event.type==expected){retests_.erase(arm);changed=true;}}
    if(changed)dirty_=true;
    if(!dirty_)return false;
    if(!Save()){++persistenceFailures_;persistenceHealthy_=false;return false;}
    dirty_=false;persistenceHealthy_=true;return true;
}

bool PlayerActivityStore::RecordRegionDiscoveries(const std::vector<DomEngine::NativeRegionDiscovery>& discoveries){
    bool changed=false;
    for(const auto&discovery:discoveries){
        if(discovery.characterGuid.empty()||discovery.id>=32||!AuthoritativeRegion(discovery.name))continue;
        auto&baseline=baselines_[discovery.characterGuid];const std::uint32_t bit=std::uint32_t(1)<<discovery.id;
        if(baseline.everRegions&bit)continue;
        Add(discovery.characterGuid,discovery.capturedAt?discovery.capturedAt:Now(),"region_discovered","Discovered "+discovery.name,{},kRegionDiscoveryIcon);
        baseline.regionsInitialized=true;baseline.regions|=bit;baseline.everRegions|=bit;changed=true;
        auto arm=retests_.find(discovery.characterGuid);if(arm!=retests_.end()&&arm->second.family=="region"&&arm->second.key==std::to_string(discovery.id)){retests_.erase(arm);changed=true;}
    }
    if(changed)dirty_=true;
    if(!dirty_)return false;
    if(!Save()){++persistenceFailures_;persistenceHealthy_=false;return false;}
    dirty_=false;persistenceHealthy_=true;return true;
}

bool PlayerActivityStore::BeginRetest(const std::string&guid,const std::string&family,const std::string&key,const std::string&related,int target,const std::string&operation){
    static const std::set<std::string> allowed={"skill","perk","spell","recipe","quest","material","region","death"};
    if(!Safe(guid)||!allowed.count(family)||key.empty()||key.size()>512||related.size()>128||target<0||target>200)return false;
    auto existing=retests_.find(guid);if(existing!=retests_.end()&&Now()-existing->second.created<=900)return false;
    if(existing!=retests_.end())retests_.erase(existing);
    const std::string effectiveOperation=family=="quest"&&operation.empty()?"start":operation;
    if(family=="quest"&&effectiveOperation!="start"&&effectiveOperation!="progress"&&effectiveOperation!="complete")return false;
    RetestArm arm;arm.family=family;arm.key=key;arm.related=related;arm.operation=effectiveOperation;arm.target=target;arm.created=Now();
    retests_[guid]=std::move(arm);return true;
}

bool PlayerActivityStore::RetestReady(const std::string&guid,const std::string&family,const std::string&key){
    auto found=retests_.find(guid);if(found==retests_.end()||found->second.family!=family||found->second.key!=key||Now()-found->second.created>900||!found->second.gameResetObserved||!found->second.dependentResetObserved||found->second.triggerSubmitted)return false;found->second.triggerSubmitted=true;return true;
}

void PlayerActivityStore::ReleaseRetestTrigger(const std::string&guid){auto found=retests_.find(guid);if(found!=retests_.end())found->second.triggerSubmitted=false;}

bool PlayerActivityStore::ConfirmRetestPreparation(const std::string&guid){
    auto found=retests_.find(guid);if(found==retests_.end()||Now()-found->second.created>900)return false;
    auto&arm=found->second;arm.preparationConfirmed=true;arm.gameResetObserved=arm.family=="death";arm.dependentResetObserved=arm.family=="death";
    // Region preparation has already passed ResetRegion's native game-query
    // readback and the authoritative character save before this callback is
    // emitted. Commit that stronger evidence directly instead of requiring a
    // later periodic snapshot to rediscover the same cleared bit.
    if(arm.family=="region"){
        unsigned bit=32;try{bit=unsigned(std::stoul(arm.key));}catch(...){}
        if(bit>=32)return false;
        auto&baseline=baselines_[guid];const auto mask=std::uint32_t(1)<<bit;
        baseline.regionsInitialized=true;baseline.regions&=~mask;baseline.everRegions&=~mask;
        arm.gameResetObserved=true;arm.dependentResetObserved=true;dirty_=true;
        if(!Save()){arm.gameResetObserved=false;arm.dependentResetObserved=false;++persistenceFailures_;persistenceHealthy_=false;return false;}
        dirty_=false;persistenceHealthy_=true;
    }
    return true;
}

void PlayerActivityStore::CancelRetest(const std::string&guid){retests_.erase(guid);}

std::string PlayerActivityStore::RetestJson(const std::string&guid)const{
    auto found=retests_.find(guid);if(found==retests_.end()||Now()-found->second.created>900)return "{\"active\":false}";const auto&a=found->second;
    return "{\"active\":true,\"family\":\""+Json(a.family)+"\",\"key\":\""+Json(a.key)+"\",\"related\":\""+Json(a.related)+"\",\"operation\":\""+Json(a.operation)+"\",\"target\":"+std::to_string(a.target)+",\"preparationConfirmed\":"+(a.preparationConfirmed?"true":"false")+",\"gameResetObserved\":"+(a.gameResetObserved?"true":"false")+",\"dependentResetObserved\":"+(a.dependentResetObserved?"true":"false")+",\"triggerSubmitted\":"+(a.triggerSubmitted?"true":"false")+",\"ready\":"+(a.gameResetObserved&&a.dependentResetObserved&&!a.triggerSubmitted?"true":"false")+"}";
}

bool PlayerActivityStore::Save()const{
    const std::string tmp=path_+".tmp";std::ostringstream o;o<<"N\t"<<nextId_<<'\n';
    for(const auto&p:baselines_){const auto&g=p.first;const auto&b=p.second;o<<"B\t"<<g<<'\t'<<b.skillsInitialized<<'\t'<<b.recipesInitialized<<'\t'<<Hex(b.recipeFingerprint)<<'\t'<<b.questsInitialized<<'\t'<<Hex(b.questFingerprint)<<'\t'<<b.deathInitialized<<'\t'<<b.dead<<'\t'<<b.regionsInitialized<<'\t'<<b.regions<<'\t'<<b.everRegions<<'\t'<<b.materialsInitialized<<'\t'<<Hex(b.lastKnownLocation)<<'\n';for(const auto&s:b.skillLevels)o<<"S\t"<<g<<'\t'<<Hex(s.first)<<'\t'<<s.second<<'\n';for(const auto&r:b.recipeGroups)o<<"R\t"<<g<<'\t'<<Hex(r)<<'\n';for(const auto&r:b.everRecipeGroups)o<<"Y\t"<<g<<'\t'<<Hex(r)<<'\n';for(const auto&q:b.quests)o<<"Q\t"<<g<<'\t'<<q.first<<'\t'<<Hex(q.second.status)<<'\t'<<Hex(q.second.objective)<<'\n';for(const auto&q:b.everQuestEvents)o<<"V\t"<<g<<'\t'<<Hex(q)<<'\n';o<<"X\t"<<g<<'\t'<<Hex(b.materialFingerprint)<<'\t'<<b.unlocksInitialized<<'\t'<<Hex(b.unlockFingerprint)<<'\n';for(const auto&m:b.materials)o<<"M\t"<<g<<'\t'<<Hex(m)<<"\t0\n";for(const auto&m:b.everMaterials)o<<"M\t"<<g<<'\t'<<Hex(m)<<"\t1\n";for(const auto&u:b.perks)o<<"U\t"<<g<<"\tperk\t"<<Hex(u)<<"\t0\n";for(const auto&u:b.spells)o<<"U\t"<<g<<"\tspell\t"<<Hex(u)<<"\t0\n";for(const auto&u:b.everPerks)o<<"U\t"<<g<<"\tperk\t"<<Hex(u)<<"\t1\n";for(const auto&u:b.everSpells)o<<"U\t"<<g<<"\tspell\t"<<Hex(u)<<"\t1\n";}
    for(const auto&e:events_)o<<"E\t"<<e.id<<'\t'<<e.characterGuid<<'\t'<<e.timestamp<<'\t'<<Hex(e.type)<<'\t'<<Hex(e.title)<<'\t'<<Hex(e.detail)<<'\t'<<Hex(e.icon)<<'\n';
    const std::string bytes=o.str();int fd=::open(tmp.c_str(),O_WRONLY|O_CREAT|O_TRUNC,0600);if(fd<0)return false;if(fchmod(fd,0600)){::close(fd);return false;}size_t offset=0;while(offset<bytes.size()){ssize_t written=::write(fd,bytes.data()+offset,bytes.size()-offset);if(written<0&&errno==EINTR)continue;if(written<=0){::close(fd);return false;}offset+=static_cast<size_t>(written);}const bool synced=fsync(fd)==0;const bool closed=::close(fd)==0;if(!synced||!closed)return false;if(std::rename(tmp.c_str(),path_.c_str()))return false;return chmod(path_.c_str(),0600)==0;
}

bool PlayerActivityStore::Load(){if(chmod(path_.c_str(),0600)&&errno!=ENOENT){++persistenceFailures_;persistenceHealthy_=false;return false;}std::ifstream in(path_);if(!in)return true;std::string line;while(std::getline(in,line)){auto f=Split(line,'\t');try{if(f.size()==2&&f[0]=="N"){auto id=std::stoull(f[1]);if(id<std::numeric_limits<std::uint64_t>::max())nextId_=std::max<std::uint64_t>(1,id);}else if((f.size()==13||f.size()==14)&&f[0]=="B"&&Safe(f[1])){auto&b=baselines_[f[1]];b.skillsInitialized=f[2]=="1";Unhex(f[4],b.recipeFingerprint);b.recipesInitialized=f[3]=="1";b.questsInitialized=f[5]=="1";Unhex(f[6],b.questFingerprint);b.deathInitialized=f[7]=="1";b.dead=f[8]=="1";b.regionsInitialized=f[9]=="1";b.regions=std::stoul(f[10]);b.everRegions=std::stoul(f[11]);b.materialsInitialized=f[12]=="1";if(f.size()==14)Unhex(f[13],b.lastKnownLocation);}else if(f.size()==4&&f[0]=="S"&&baselines_.count(f[1])){std::string n;if(Unhex(f[2],n)&&n.size()<=256)baselines_[f[1]].skillLevels[n]=std::stoi(f[3]);}else if(f.size()==3&&f[0]=="R"&&baselines_.count(f[1])){std::string v;if(Unhex(f[2],v)&&v.size()<=2048)baselines_[f[1]].recipeGroups.insert(v);}else if(f.size()==3&&f[0]=="Y"&&baselines_.count(f[1])){std::string v;if(Unhex(f[2],v)&&v.size()<=2048)baselines_[f[1]].everRecipeGroups.insert(v);}else if(f.size()==5&&f[0]=="Q"&&baselines_.count(f[1])&&Safe(f[2])){std::string s,o;if(Unhex(f[3],s)&&Unhex(f[4],o)&&s.size()<=64&&o.size()<=2048)baselines_[f[1]].quests[f[2]]={s,o};}else if(f.size()==3&&f[0]=="V"&&baselines_.count(f[1])){std::string v;if(Unhex(f[2],v)&&v.size()<=4096)baselines_[f[1]].everQuestEvents.insert(v);}else if(f.size()==3&&f[0]=="G"&&baselines_.count(f[1])){std::string v;if(Unhex(f[2],v)&&AuthoritativeRegion(v)){auto&b=baselines_[f[1]];b.observedRegionsInitialized=true;b.observedRegions.insert(std::move(v));}}else if(f.size()==5&&f[0]=="X"&&baselines_.count(f[1])&&(f[3]=="0"||f[3]=="1")){std::string material,unlock;if(Unhex(f[2],material)&&Unhex(f[4],unlock)&&material.size()<=128&&unlock.size()<=128){auto&b=baselines_[f[1]];b.materialFingerprint=std::move(material);b.unlocksInitialized=f[3]=="1";b.unlockFingerprint=std::move(unlock);}}else if(f.size()==4&&f[0]=="M"&&baselines_.count(f[1])){std::string v;if(Unhex(f[2],v)&&v.size()<=256){if(f[3]=="0")baselines_[f[1]].materials.insert(v);else if(f[3]=="1")baselines_[f[1]].everMaterials.insert(v);}}else if(f.size()==5&&f[0]=="U"&&baselines_.count(f[1])&&(f[2]=="perk"||f[2]=="spell")&&(f[4]=="0"||f[4]=="1")){std::string v;if(Unhex(f[3],v)&&v.size()<=256){auto&b=baselines_[f[1]];auto&set=f[2]=="perk"?(f[4]=="1"?b.everPerks:b.perks):(f[4]=="1"?b.everSpells:b.spells);set.insert(v);}}else if(f.size()==8&&f[0]=="E"&&Safe(f[2])){PlayerActivityEvent e;e.id=std::stoull(f[1]);e.characterGuid=f[2];e.timestamp=std::stoll(f[3]);if(Unhex(f[4],e.type)&&Unhex(f[5],e.title)&&Unhex(f[6],e.detail)&&Unhex(f[7],e.icon)&&e.id&&e.id<std::numeric_limits<std::uint64_t>::max()&&e.timestamp>0&&e.timestamp<=4102444800LL&&e.type.size()<=64&&e.title.size()<=1024&&e.detail.size()<=2048&&e.icon.size()<=512)events_.push_back(std::move(e));}}catch(...){continue;}}
    // Version-two retest history is additive. Older files have no Y/V rows;
    // seed their permanent duplicate guards from the authoritative baselines.
    for(auto&pair:baselines_){auto&b=pair.second;b.everRecipeGroups.insert(b.recipeGroups.begin(),b.recipeGroups.end());for(const auto&q:b.quests){if(q.second.status=="in_progress"||q.second.status=="completed")b.everQuestEvents.insert(q.first+"\x1f""started");if(q.second.status=="completed")b.everQuestEvents.insert(q.first+"\x1f""completed");}}
    std::sort(events_.begin(),events_.end(),[](const auto&a,const auto&b){return a.id<b.id;});
    events_.erase(std::unique(events_.begin(),events_.end(),[](const auto&a,const auto&b){return a.id==b.id;}),events_.end());
    for(const auto&e:events_)nextId_=std::max(nextId_,e.id+1);
    LogMessage("Loaded "+std::to_string(events_.size())+" player activity events");return true;}

PlayerActivityPage PlayerActivityStore::Page(const std::string&g,std::size_t limit,std::uint64_t before)const{PlayerActivityPage p;limit=std::max<std::size_t>(1,std::min<std::size_t>(limit,50));for(auto i=events_.rbegin();i!=events_.rend();++i){if(i->characterGuid!=g||(before&&i->id>=before))continue;if(p.events.size()==limit){p.hasMore=true;break;}p.events.push_back(*i);}if(!p.events.empty())p.nextBefore=p.events.back().id;return p;}
std::string PlayerActivityStore::PageJson(const std::string&g,std::size_t limit,std::uint64_t before)const{auto p=Page(g,limit,before);std::ostringstream o;o<<"{\"events\":[";for(size_t i=0;i<p.events.size();++i){if(i)o<<',';const auto&e=p.events[i];o<<"{\"id\":"<<e.id<<",\"type\":\""<<Json(e.type)<<"\",\"timestamp\":"<<e.timestamp<<",\"title\":\""<<Json(e.title)<<"\",\"detail\":\""<<Json(e.detail)<<"\",\"icon\":"<<(e.icon.empty()?"null":"\""+Json(e.icon)+"\"")<<'}';}o<<"],\"hasMore\":"<<(p.hasMore?"true":"false")<<",\"nextBefore\":"<<(p.nextBefore?std::to_string(p.nextBefore):"null")<<",\"persistenceError\":"<<(persistenceHealthy_?"false":"true")<<'}';return o.str();}
PlayerActivityDiagnostics PlayerActivityStore::Diagnostics()const{return {persistenceHealthy_?1:-1,events_.size(),baselines_.size(),snapshots_,droppedSamples_,persistenceFailures_};}
