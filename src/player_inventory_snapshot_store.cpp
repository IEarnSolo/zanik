#include "player_inventory_snapshot_store.h"

#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace {
constexpr std::size_t MaximumSnapshots = 4096;
constexpr std::size_t MaximumContainers = 4;
constexpr std::size_t MaximumItems = 512;
constexpr off_t MaximumFileBytes = 64 * 1024 * 1024;
constexpr std::int64_t TimestampWriteInterval = 60;
struct Lock { pthread_mutex_t* value; explicit Lock(pthread_mutex_t& mutex):value(&mutex){pthread_mutex_lock(value);} ~Lock(){pthread_mutex_unlock(value);} };

std::vector<std::string> Fields(const std::string& line) {
    std::vector<std::string> result; std::size_t start = 0;
    for (;;) { const auto at=line.find('\t',start);result.push_back(line.substr(start,at==std::string::npos?std::string::npos:at-start));if(at==std::string::npos)return result;start=at+1; }
}
bool ParseInteger(const std::string& text,long long minimum,long long maximum,long long& value) {
    if(text.empty()||text.size()>20)return false;char*end=nullptr;errno=0;const auto parsed=std::strtoll(text.c_str(),&end,10);if(errno||!end||*end||parsed<minimum||parsed>maximum)return false;value=parsed;return true;
}
char HexDigit(unsigned value){return value<10?char('0'+value):char('a'+value-10);}
std::string HexEncode(const std::string&value){if(value.empty())return "-";std::string result;result.reserve(value.size()*2);for(unsigned char byte:value){result.push_back(HexDigit(byte>>4));result.push_back(HexDigit(byte&15));}return result;}
int HexValue(char value){if(value>='0'&&value<='9')return value-'0';if(value>='a'&&value<='f')return value-'a'+10;if(value>='A'&&value<='F')return value-'A'+10;return -1;}
bool HexDecode(const std::string&encoded,std::string&value,std::size_t maximum=1024){if(encoded=="-"){value.clear();return true;}if(encoded.empty()||encoded.size()%2||encoded.size()>maximum*2)return false;value.clear();value.reserve(encoded.size()/2);for(std::size_t i=0;i<encoded.size();i+=2){const int high=HexValue(encoded[i]),low=HexValue(encoded[i+1]);if(high<0||low<0)return false;const char byte=char((high<<4)|low);if(static_cast<unsigned char>(byte)<32||byte=='\t')return false;value.push_back(byte);}return true;}
bool ValidContainerId(const std::string&id){return id=="0000000000000001"||id=="0000000000000002"||id=="0000000000000003"||id=="0000000000000004";}
bool Valid(const PlayerInventoryDisplaySnapshot&value){
    if(value.characterGuid.empty()||value.characterGuid.size()>128||value.capturedAt<0||value.containers.size()>MaximumContainers)return false;
    std::vector<std::string> containers;
    for(const auto&container:value.containers){
        if(!ValidContainerId(container.id)||container.totalSlots<1||container.totalSlots>int(MaximumItems)||container.hotbarStart< -1||container.hotbarStart>=container.totalSlots||container.hotbarCount<0||container.hotbarCount>container.totalSlots||container.items.size()>MaximumItems||std::find(containers.begin(),containers.end(),container.id)!=containers.end())return false;
        containers.push_back(container.id);std::vector<bool> slots(std::size_t(container.totalSlots),false);
        for(const auto&item:container.items){if(item.slot<0||item.slot>=container.totalSlots||slots[std::size_t(item.slot)]||item.quantity<1||item.quantity>1000000||item.maxStack<1||item.maxStack>1000000||item.id.empty()||item.id.size()>256||item.key.empty()||item.key.size()>256||item.name.empty()||item.name.size()>512||item.icon.size()>1024)return false;slots[std::size_t(item.slot)]=true;}
    }
    return true;
}
bool SameContents(const PlayerInventoryDisplaySnapshot&left,const PlayerInventoryDisplaySnapshot&right){
    if(left.characterGuid!=right.characterGuid||left.containers.size()!=right.containers.size())return false;
    for(std::size_t i=0;i<left.containers.size();++i){const auto&a=left.containers[i],&b=right.containers[i];if(a.id!=b.id||a.totalSlots!=b.totalSlots||a.hotbarStart!=b.hotbarStart||a.hotbarCount!=b.hotbarCount||a.items.size()!=b.items.size())return false;for(std::size_t j=0;j<a.items.size();++j){const auto&x=a.items[j],&y=b.items[j];if(x.id!=y.id||x.key!=y.key||x.name!=y.name||x.icon!=y.icon||x.slot!=y.slot||x.quantity!=y.quantity||x.maxStack!=y.maxStack||x.equippable!=y.equippable||x.canUnequip!=y.canUnequip)return false;}}
    return true;
}
std::string Parent(const std::string&path){const auto slash=path.find_last_of('/');return slash==std::string::npos?".":(slash?path.substr(0,slash):"/");}
bool WriteAll(int descriptor,const std::string&value){std::size_t offset=0;while(offset<value.size()){const auto written=write(descriptor,value.data()+offset,value.size()-offset);if(written<=0)return false;offset+=std::size_t(written);}return true;}
}

PlayerInventorySnapshotStore::PlayerInventorySnapshotStore(std::string path):path_(std::move(path)){}

bool PlayerInventorySnapshotStore::Load(){
    Lock lock(mutex_);struct stat file{};if(stat(path_.c_str(),&file)==0&&file.st_size>MaximumFileBytes){snapshots_.clear();loaded_=false;malformedRecords_=1;return false;}
    std::ifstream input(path_);if(!input){snapshots_.clear();loaded_=true;malformedRecords_=0;return true;}
    std::string line;if(!std::getline(input,line)||line!="# zanik-player-inventory-snapshots\t1"){snapshots_.clear();loaded_=false;malformedRecords_=1;return false;}
    std::vector<PlayerInventoryDisplaySnapshot> parsed;std::size_t malformed=0;
    while(std::getline(input,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();const auto fields=Fields(line);long long captured=0,count=0;PlayerInventoryDisplaySnapshot value;if(fields.size()!=4||fields[0]!="P"||parsed.size()>=MaximumSnapshots||!HexDecode(fields[1],value.characterGuid,128)||!ParseInteger(fields[2],0,LLONG_MAX,captured)||!ParseInteger(fields[3],0,MaximumContainers,count)){++malformed;break;}value.capturedAt=captured;
        for(long long c=0;c<count&&!malformed;++c){if(!std::getline(input,line)){++malformed;break;}if(!line.empty()&&line.back()=='\r')line.pop_back();const auto containerFields=Fields(line);long long slots=0,hotbarStart=0,hotbarCount=0,itemCount=0;PlayerInventorySnapshotContainer container;if(containerFields.size()!=6||containerFields[0]!="C"||!ValidContainerId(containerFields[1])||!ParseInteger(containerFields[2],1,MaximumItems,slots)||!ParseInteger(containerFields[3],-1,MaximumItems-1,hotbarStart)||!ParseInteger(containerFields[4],0,MaximumItems,hotbarCount)||!ParseInteger(containerFields[5],0,MaximumItems,itemCount)){++malformed;break;}container.id=containerFields[1];container.totalSlots=int(slots);container.hotbarStart=int(hotbarStart);container.hotbarCount=int(hotbarCount);
            for(long long i=0;i<itemCount;++i){if(!std::getline(input,line)){++malformed;break;}if(!line.empty()&&line.back()=='\r')line.pop_back();const auto itemFields=Fields(line);long long slot=0,quantity=0,maxStack=0,equippable=0,canUnequip=0;PlayerInventorySnapshotItem item;if(itemFields.size()!=10||itemFields[0]!="I"||!ParseInteger(itemFields[1],0,MaximumItems-1,slot)||!ParseInteger(itemFields[2],1,1000000,quantity)||!ParseInteger(itemFields[3],1,1000000,maxStack)||!ParseInteger(itemFields[4],0,1,equippable)||!ParseInteger(itemFields[5],0,1,canUnequip)||!HexDecode(itemFields[6],item.id,256)||!HexDecode(itemFields[7],item.key,256)||!HexDecode(itemFields[8],item.name,512)||!HexDecode(itemFields[9],item.icon,1024)){++malformed;break;}item.slot=int(slot);item.quantity=int(quantity);item.maxStack=int(maxStack);item.equippable=equippable!=0;item.canUnequip=canUnequip!=0;container.items.push_back(std::move(item));}
            value.containers.push_back(std::move(container));
        }
        if(malformed||!Valid(value)||std::any_of(parsed.begin(),parsed.end(),[&](const auto&prior){return prior.characterGuid==value.characterGuid;})){if(!malformed)++malformed;break;}parsed.push_back(std::move(value));
    }
    if(!input.eof()&&input.fail())++malformed;malformedRecords_=malformed;if(malformed){snapshots_.clear();loaded_=false;return false;}snapshots_=std::move(parsed);loaded_=true;return true;
}

bool PlayerInventorySnapshotStore::SaveLocked(){
    auto ordered=snapshots_;std::sort(ordered.begin(),ordered.end(),[](const auto&a,const auto&b){return a.characterGuid<b.characterGuid;});std::ostringstream content;content<<"# zanik-player-inventory-snapshots\t1\n";
    for(auto&value:ordered){std::sort(value.containers.begin(),value.containers.end(),[](const auto&a,const auto&b){return a.id<b.id;});content<<"P\t"<<HexEncode(value.characterGuid)<<'\t'<<value.capturedAt<<'\t'<<value.containers.size()<<'\n';for(auto&container:value.containers){std::sort(container.items.begin(),container.items.end(),[](const auto&a,const auto&b){return a.slot<b.slot;});content<<"C\t"<<container.id<<'\t'<<container.totalSlots<<'\t'<<container.hotbarStart<<'\t'<<container.hotbarCount<<'\t'<<container.items.size()<<'\n';for(const auto&item:container.items)content<<"I\t"<<item.slot<<'\t'<<item.quantity<<'\t'<<item.maxStack<<'\t'<<(item.equippable?1:0)<<'\t'<<(item.canUnequip?1:0)<<'\t'<<HexEncode(item.id)<<'\t'<<HexEncode(item.key)<<'\t'<<HexEncode(item.name)<<'\t'<<HexEncode(item.icon)<<'\n';}}
    const std::string bytes=content.str();if(bytes.size()>std::size_t(MaximumFileBytes)){lastSaveSucceeded_=false;return false;}const std::string temporary=path_+".tmp";const int descriptor=open(temporary.c_str(),O_WRONLY|O_CREAT|O_TRUNC|O_CLOEXEC,0600);if(descriptor<0){lastSaveSucceeded_=false;return false;}bool ok=WriteAll(descriptor,bytes)&&fsync(descriptor)==0;if(close(descriptor)!=0)ok=false;if(ok&&std::rename(temporary.c_str(),path_.c_str())==0){chmod(path_.c_str(),0600);const int parent=open(Parent(path_).c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC);if(parent>=0){ok=fsync(parent)==0;close(parent);}}else ok=false;if(!ok)unlink(temporary.c_str());lastSaveSucceeded_=ok;return ok;
}

bool PlayerInventorySnapshotStore::Replace(const PlayerInventoryDisplaySnapshot&value){
    if(!Valid(value))return false;Lock lock(mutex_);if(!loaded_)return false;const auto previous=snapshots_;auto found=std::find_if(snapshots_.begin(),snapshots_.end(),[&](const auto&item){return item.characterGuid==value.characterGuid;});if(found==snapshots_.end()){if(snapshots_.size()>=MaximumSnapshots)return false;snapshots_.push_back(value);}else if(SameContents(*found,value)&&value.capturedAt-found->capturedAt<TimestampWriteInterval)return true;else *found=value;if(!SaveLocked()){snapshots_=previous;return false;}return true;
}

bool PlayerInventorySnapshotStore::Find(const std::string&characterGuid,PlayerInventoryDisplaySnapshot&value)const{Lock lock(mutex_);const auto found=std::find_if(snapshots_.begin(),snapshots_.end(),[&](const auto&item){return item.characterGuid==characterGuid;});if(found==snapshots_.end())return false;value=*found;return true;}

PlayerInventorySnapshotDiagnostics PlayerInventorySnapshotStore::Diagnostics()const{Lock lock(mutex_);PlayerInventorySnapshotDiagnostics result;result.loaded=loaded_;result.lastSaveSucceeded=lastSaveSucceeded_;result.snapshots=snapshots_.size();result.malformedRecords=malformedRecords_;struct stat file{};const bool exists=stat(path_.c_str(),&file)==0;result.readable=!exists||access(path_.c_str(),R_OK)==0;result.writable=(exists?access(path_.c_str(),W_OK):access(Parent(path_).c_str(),W_OK))==0;result.restricted=!exists||(file.st_mode&0077)==0;return result;}
