#include "recipe_settings_store.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace {
struct Lock { pthread_mutex_t* value; explicit Lock(pthread_mutex_t& mutex):value(&mutex){pthread_mutex_lock(value);} ~Lock(){pthread_mutex_unlock(value);} };
bool Valid(const std::string& id){if(id.empty()||id.size()>72)return false;return std::all_of(id.begin(),id.end(),[](unsigned char c){return std::isalnum(c)||c=='-'||c=='_'||c=='.';});}
}

RecipeSettingsStore::RecipeSettingsStore(std::string path):path_(std::move(path)){}
bool RecipeSettingsStore::Load(){Lock lock(mutex_);disabled_.clear();malformed_=0;std::ifstream input(path_);if(!input)return true;std::string line;while(std::getline(input,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.empty())continue;if(!Valid(line)){++malformed_;continue;}disabled_.insert(line);}return true;}
bool RecipeSettingsStore::SaveLocked(){const std::string temporary=path_+".tmp";std::ofstream output(temporary,std::ios::trunc);if(!output){lastSaveSucceeded_=false;return false;}std::vector<std::string> ids(disabled_.begin(),disabled_.end());std::sort(ids.begin(),ids.end());for(const auto& id:ids)output<<id<<'\n';output.close();chmod(temporary.c_str(),0600);if(std::rename(temporary.c_str(),path_.c_str())){lastSaveSucceeded_=false;return false;}chmod(path_.c_str(),0600);lastSaveSucceeded_=true;return true;}
bool RecipeSettingsStore::SetEnabled(const std::string& id,bool enabled){if(!Valid(id))return false;Lock lock(mutex_);if(enabled)disabled_.erase(id);else disabled_.insert(id);return SaveLocked();}
bool RecipeSettingsStore::SetManyEnabled(const std::vector<std::string>& ids,bool enabled){if(ids.empty()||ids.size()>4096||!std::all_of(ids.begin(),ids.end(),Valid))return false;Lock lock(mutex_);for(const auto& id:ids)if(enabled)disabled_.erase(id);else disabled_.insert(id);return SaveLocked();}
bool RecipeSettingsStore::Enabled(const std::string& id){Lock lock(mutex_);return !disabled_.count(id);}
std::string RecipeSettingsStore::Json(){Lock lock(mutex_);std::vector<std::string> ids(disabled_.begin(),disabled_.end());std::sort(ids.begin(),ids.end());std::ostringstream out;out<<"{\"schemaVersion\":1,\"disabled\":[";for(std::size_t i=0;i<ids.size();++i){if(i)out<<',';out<<'\"'<<ids[i]<<'\"';}out<<"],\"disabledCount\":"<<ids.size()<<'}';return out.str();}
RecipeSettingsDiagnostics RecipeSettingsStore::Diagnostics(){Lock lock(mutex_);RecipeSettingsDiagnostics result;struct stat file{};const bool exists=stat(path_.c_str(),&file)==0;result.readable=!exists||access(path_.c_str(),R_OK)==0;auto slash=path_.find_last_of('/');const std::string parent=slash==std::string::npos?".":path_.substr(0,slash);result.writable=(exists?access(path_.c_str(),W_OK):access(parent.c_str(),W_OK))==0;result.restricted=!exists||(file.st_mode&0077)==0;result.lastSaveSucceeded=lastSaveSucceeded_;result.disabled=disabled_.size();result.malformed=malformed_;return result;}
