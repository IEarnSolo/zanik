#include "player_settings_store.h"

#include <algorithm>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace {
constexpr std::size_t MaximumCharacters = 4096;
struct Lock { pthread_mutex_t* value; explicit Lock(pthread_mutex_t& mutex):value(&mutex){pthread_mutex_lock(value);} ~Lock(){pthread_mutex_unlock(value);} };
bool ValidGuid(const std::string& value){return !value.empty()&&value.size()<=128&&std::none_of(value.begin(),value.end(),[](unsigned char c){return c<33||c=='\t';});}
std::string Parent(const std::string& path){const auto slash=path.find_last_of('/');return slash==std::string::npos?".":(slash?path.substr(0,slash):"/");}
bool WriteAll(int descriptor,const std::string& value){std::size_t offset=0;while(offset<value.size()){const auto written=write(descriptor,value.data()+offset,value.size()-offset);if(written<=0)return false;offset+=std::size_t(written);}return true;}
}

PlayerSettingsStore::PlayerSettingsStore(std::string path):path_(std::move(path)){}

bool PlayerSettingsStore::Load(){
    Lock lock(mutex_);
    std::ifstream input(path_);
    if(!input){freeBuilding_.clear();loaded_=true;malformedRecords_=0;return true;}
    std::string line;if(!std::getline(input,line)||line!="# zanik-player-settings\t1"){freeBuilding_.clear();loaded_=false;malformedRecords_=1;return false;}
    std::unordered_set<std::string> parsed;std::size_t malformed=0;
    while(std::getline(input,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();const auto tab=line.find('\t');if(tab==std::string::npos||line.substr(0,tab)!="free-building"||!ValidGuid(line.substr(tab+1))||parsed.size()>=MaximumCharacters||!parsed.insert(line.substr(tab+1)).second){++malformed;break;}}
    if(!input.eof()&&input.fail())++malformed;
    malformedRecords_=malformed;
    if(malformed){freeBuilding_.clear();loaded_=false;return false;}
    freeBuilding_=std::move(parsed);loaded_=true;return true;
}

bool PlayerSettingsStore::SaveLocked(){
    std::vector<std::string> ordered(freeBuilding_.begin(),freeBuilding_.end());std::sort(ordered.begin(),ordered.end());std::ostringstream content;content<<"# zanik-player-settings\t1\n";for(const auto&guid:ordered)content<<"free-building\t"<<guid<<'\n';
    const std::string temporary=path_+".tmp";const int descriptor=open(temporary.c_str(),O_WRONLY|O_CREAT|O_TRUNC|O_CLOEXEC,0600);if(descriptor<0){lastSaveSucceeded_=false;return false;}const std::string bytes=content.str();bool ok=WriteAll(descriptor,bytes)&&fsync(descriptor)==0;if(close(descriptor)!=0)ok=false;
    if(ok&&std::rename(temporary.c_str(),path_.c_str())==0){chmod(path_.c_str(),0600);const int parent=open(Parent(path_).c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC);if(parent>=0){ok=fsync(parent)==0;close(parent);}}else ok=false;if(!ok)unlink(temporary.c_str());lastSaveSucceeded_=ok;return ok;
}

bool PlayerSettingsStore::SetFreeBuilding(const std::string& characterGuid,bool enabled){
    if(!ValidGuid(characterGuid))return false;
    Lock lock(mutex_);
    if(!loaded_)return false;
    const auto previous=freeBuilding_;
    if(enabled){if(freeBuilding_.size()>=MaximumCharacters&&!freeBuilding_.count(characterGuid))return false;freeBuilding_.insert(characterGuid);}else freeBuilding_.erase(characterGuid);
    if(!SaveLocked()){freeBuilding_=previous;return false;}
    return true;
}

bool PlayerSettingsStore::FreeBuilding(const std::string& characterGuid)const{Lock lock(mutex_);return freeBuilding_.count(characterGuid)!=0;}
std::unordered_set<std::string> PlayerSettingsStore::FreeBuildingCharacters()const{Lock lock(mutex_);return freeBuilding_;}
PlayerSettingsDiagnostics PlayerSettingsStore::Diagnostics()const{Lock lock(mutex_);PlayerSettingsDiagnostics result;result.loaded=loaded_;result.lastSaveSucceeded=lastSaveSucceeded_;result.freeBuildingCharacters=freeBuilding_.size();result.malformedRecords=malformedRecords_;struct stat file{};const bool exists=stat(path_.c_str(),&file)==0;result.readable=!exists||access(path_.c_str(),R_OK)==0;result.writable=(exists?access(path_.c_str(),W_OK):access(Parent(path_).c_str(),W_OK))==0;result.restricted=!exists||(file.st_mode&0077)==0;return result;}
