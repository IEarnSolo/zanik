#include "recipe_snapshot_store.h"
#include "utils/logger.h"
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

namespace {
std::vector<std::string> Split(const std::string& value, char separator) {
    std::vector<std::string> out; size_t start=0;
    while(true){size_t at=value.find(separator,start);out.push_back(value.substr(start,at==std::string::npos?at:at-start));if(at==std::string::npos)break;start=at+1;}return out;
}
bool Safe(const std::string& value){return !value.empty()&&value.size()<=128&&value.find_first_not_of("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_-")==std::string::npos;}
}
RecipeSnapshotStore::RecipeSnapshotStore(std::string path):path_(std::move(path)){}
bool RecipeSnapshotStore::Load(){std::ifstream in(path_);if(!in)return true;std::string line;while(std::getline(in,line)){auto f=Split(line,'\t');if(f.size()!=6||f[0]!="P"||!Safe(f[1])||!Safe(f[2]))continue;StoredRecipeSnapshot s;s.characterGuid=f[1];s.catalogFingerprint=f[2];try{s.capturedAt=std::stoll(f[3]);s.complete=f[4]=="1";}catch(...){continue;}if(!f[5].empty())for(auto&id:Split(f[5],','))if(id.size()==16&&Safe(id))s.unlockedIds.push_back(id);std::sort(s.unlockedIds.begin(),s.unlockedIds.end());s.unlockedIds.erase(std::unique(s.unlockedIds.begin(),s.unlockedIds.end()),s.unlockedIds.end());if(!s.complete||s.unlockedIds.empty())continue;snapshots_[s.characterGuid]=std::move(s);}LogMessage("Loaded "+std::to_string(snapshots_.size())+" player recipe snapshots");return true;}
bool RecipeSnapshotStore::Save()const{const std::string temporary=path_+".tmp";std::ofstream out(temporary,std::ios::trunc);if(!out)return false;for(const auto&pair:snapshots_){const auto&s=pair.second;out<<"P\t"<<s.characterGuid<<'\t'<<s.catalogFingerprint<<'\t'<<s.capturedAt<<'\t'<<(s.complete?1:0)<<'\t';for(size_t i=0;i<s.unlockedIds.size();++i){if(i)out<<',';out<<s.unlockedIds[i];}out<<'\n';}out.close();chmod(temporary.c_str(),0600);if(std::rename(temporary.c_str(),path_.c_str())!=0)return false;chmod(path_.c_str(),0600);return true;}
bool RecipeSnapshotStore::Sync(const std::vector<DomEngine::PlayerRecipeSnapshot>& incoming){bool changed=false;for(const auto&live:incoming){if(!live.complete||live.characterGuid.empty()||live.unlockedIds.empty())continue;auto found=snapshots_.find(live.characterGuid);if(found!=snapshots_.end()&&found->second.catalogFingerprint==live.catalogFingerprint){const auto&prior=found->second.unlockedIds;if(live.unlockedIds.size()<prior.size()&&std::includes(prior.begin(),prior.end(),live.unlockedIds.begin(),live.unlockedIds.end()))continue;if(found->second.capturedAt==live.capturedAt&&prior==live.unlockedIds)continue;}snapshots_[live.characterGuid]={live.characterGuid,live.catalogFingerprint,live.unlockedIds,live.capturedAt,true};changed=true;}return !changed||Save();}
bool RecipeSnapshotStore::Find(const std::string& guid,StoredRecipeSnapshot& out)const{auto it=snapshots_.find(guid);if(it==snapshots_.end())return false;out=it->second;return true;}
