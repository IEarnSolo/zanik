#include "admin_store.h"
#include "utils/logger.h"

#include <array>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <sys/random.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace {
struct Lock{pthread_mutex_t* mutex;explicit Lock(pthread_mutex_t& value):mutex(&value){pthread_mutex_lock(mutex);}~Lock(){pthread_mutex_unlock(mutex);}};
std::int64_t Now(){return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();}
std::string Hex(const unsigned char* p,std::size_t n){static const char h[]="0123456789abcdef";std::string s;s.reserve(n*2);for(std::size_t i=0;i<n;++i){s+=h[p[i]>>4];s+=h[p[i]&15];}return s;}
std::string Enc(const std::string&s){return Hex(reinterpret_cast<const unsigned char*>(s.data()),s.size());}
int Nib(char c){if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;return -1;}
bool Dec(const std::string&s,std::string&o){if(s.size()%2)return false;o.clear();for(std::size_t i=0;i<s.size();i+=2){int a=Nib(s[i]),b=Nib(s[i+1]);if(a<0||b<0)return false;o+=char((a<<4)|b);}return true;}
std::array<unsigned char,32> Sha256(const std::string&s){
 static const std::uint32_t k[64]={0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
 std::vector<unsigned char>d(s.begin(),s.end());std::uint64_t bits=d.size()*8;d.push_back(0x80);while(d.size()%64!=56)d.push_back(0);for(int i=7;i>=0;--i)d.push_back((bits>>(i*8))&255);
 std::uint32_t h[8]={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
 auto rr=[](std::uint32_t x,int n){return(x>>n)|(x<<(32-n));};for(std::size_t p=0;p<d.size();p+=64){std::uint32_t w[64];for(int i=0;i<16;++i)w[i]=(std::uint32_t(d[p+4*i])<<24)|(std::uint32_t(d[p+4*i+1])<<16)|(std::uint32_t(d[p+4*i+2])<<8)|d[p+4*i+3];for(int i=16;i<64;++i){auto a=rr(w[i-15],7)^rr(w[i-15],18)^(w[i-15]>>3),b=rr(w[i-2],17)^rr(w[i-2],19)^(w[i-2]>>10);w[i]=w[i-16]+a+w[i-7]+b;}auto a=h[0],b=h[1],c=h[2],e=h[4],f=h[5],g=h[6],q=h[7],dd=h[3];for(int i=0;i<64;++i){auto s1=rr(e,6)^rr(e,11)^rr(e,25),ch=(e&f)^((~e)&g),t1=q+s1+ch+k[i]+w[i],s0=rr(a,2)^rr(a,13)^rr(a,22),maj=(a&b)^(a&c)^(b&c),t2=s0+maj;q=g;g=f;f=e;e=dd+t1;dd=c;c=b;b=a;a=t1+t2;}h[0]+=a;h[1]+=b;h[2]+=c;h[3]+=dd;h[4]+=e;h[5]+=f;h[6]+=g;h[7]+=q;}
 std::array<unsigned char,32>o{};for(int i=0;i<8;++i)for(int j=0;j<4;++j)o[i*4+j]=(h[i]>>(24-j*8))&255;return o;
}
}
AdminStore::AdminStore(std::string a,std::string s):allowlistPath_(std::move(a)),sessionPath_(std::move(s)){auto slash=sessionPath_.find_last_of('/');auditPath_=(slash==std::string::npos?"":sessionPath_.substr(0,slash+1))+"admin-audit.log";}
std::string AdminStore::Hash(const std::string&v)const{auto h=Sha256(v);return Hex(h.data(),h.size());}
std::string AdminStore::RandomToken(std::size_t n)const{std::vector<unsigned char>b(n);std::size_t done=0;while(done<n){ssize_t r=getrandom(b.data()+done,n-done,0);if(r<=0)return {};done+=r;}return Hex(b.data(),b.size());}
void AdminStore::ReloadAllowlistLocked(){struct stat st{};std::int64_t mt=stat(allowlistPath_.c_str(),&st)==0?st.st_mtime:-1;if(mt==allowlistMtime_)return;allowlistMtime_=mt;allowed_.clear();std::ifstream in(allowlistPath_);std::string line;while(std::getline(in,line)){auto hash=line.find('#');if(hash!=std::string::npos)line.resize(hash);auto a=line.find_first_not_of(" \t\r"),b=line.find_last_not_of(" \t\r");if(a!=std::string::npos)allowed_[line.substr(a,b-a+1)]=true;}for(auto it=sessions_.begin();it!=sessions_.end();)if(!allowed_.count(it->second.session.publicId)){removedSessions_[it->first]=true;it=sessions_.erase(it);}else ++it;SaveLocked();}
bool AdminStore::Load(){Lock g(mutex_);std::ifstream in(sessionPath_);std::string line;malformedSessionRecords_=0;while(std::getline(in,line)){std::istringstream s(line);std::string tag,hash,pid,guid,name,csrf,created;if(!std::getline(s,tag,'\t')||tag!="S"||!std::getline(s,hash,'\t')||!std::getline(s,pid,'\t')||!std::getline(s,guid,'\t')||!std::getline(s,name,'\t')||!std::getline(s,csrf,'\t')||!std::getline(s,created)){++malformedSessionRecords_;continue;}Stored x;x.hash=hash;if(!Dec(pid,x.session.publicId)||!Dec(guid,x.session.characterGuid)||!Dec(name,x.session.characterName)||!Dec(csrf,x.session.csrf)){++malformedSessionRecords_;continue;}try{x.created=std::stoll(created);}catch(...){++malformedSessionRecords_;continue;}sessions_[hash]=std::move(x);}ReloadAllowlistLocked();return true;}
bool AdminStore::SaveLocked()const{std::string tmp=sessionPath_+".tmp";std::ofstream out(tmp,std::ios::trunc);if(!out){lastSaveSucceeded_=false;return false;}for(const auto&kv:sessions_){const auto&x=kv.second;out<<"S\t"<<x.hash<<'\t'<<Enc(x.session.publicId)<<'\t'<<Enc(x.session.characterGuid)<<'\t'<<Enc(x.session.characterName)<<'\t'<<Enc(x.session.csrf)<<'\t'<<x.created<<'\n';}out.close();chmod(tmp.c_str(),0600);if(std::rename(tmp.c_str(),sessionPath_.c_str())){lastSaveSucceeded_=false;return false;}chmod(sessionPath_.c_str(),0600);lastSaveSucceeded_=true;return true;}
bool AdminStore::IsAllowed(const std::string&id){Lock g(mutex_);ReloadAllowlistLocked();return allowed_.count(id);}
std::vector<std::string> AdminStore::AllowedIds(){Lock g(mutex_);ReloadAllowlistLocked();std::vector<std::string> ids;for(const auto&entry:allowed_)ids.push_back(entry.first);return ids;}
std::string AdminStore::CreateClaim(const std::string&id,const std::string&guid,const std::string&name){Lock g(mutex_);ReloadAllowlistLocked();if(!allowed_.count(id))return {};std::string t=RandomToken(32);if(t.empty())return {};AdminSession s{id,guid,name,RandomToken(32)};claims_[Hash(t)]={s,Now()+300};return t;}
bool AdminStore::RedeemClaim(const std::string&t,std::string&raw,AdminSession&s){Lock g(mutex_);ReloadAllowlistLocked();auto h=Hash(t);auto it=claims_.find(h);if(it==claims_.end()||it->second.expires<Now()||!allowed_.count(it->second.session.publicId)){if(it!=claims_.end())claims_.erase(it);return false;}s=it->second.session;claims_.erase(it);raw=RandomToken(32);if(raw.empty())return false;Stored x{s,Hash(raw),Now()};sessions_[x.hash]=x;return SaveLocked();}
bool AdminStore::Authenticate(const std::string&t,AdminSession&s,bool*removed){Lock g(mutex_);ReloadAllowlistLocked();auto hash=Hash(t);auto it=sessions_.find(hash);if(it==sessions_.end()){if(removed)*removed=removedSessions_.count(hash);return false;}if(!allowed_.count(it->second.session.publicId)){if(removed)*removed=true;removedSessions_[hash]=true;sessions_.erase(it);SaveLocked();return false;}s=it->second.session;s.sessionHashPrefix=hash.substr(0,12);return true;}
bool AdminStore::Logout(const std::string&t){Lock g(mutex_);bool ok=sessions_.erase(Hash(t));if(ok)SaveLocked();return ok;}
std::size_t AdminStore::RevokeCharacter(const std::string&id){Lock g(mutex_);std::size_t n=0;for(auto it=sessions_.begin();it!=sessions_.end();)if(it->second.session.publicId==id){it=sessions_.erase(it);++n;}else ++it;SaveLocked();return n;}
std::string AdminStore::SessionHashPrefix(const std::string&t)const{return Hash(t).substr(0,12);}
void AdminStore::Audit(const AdminSession&s,const std::string&dest,const std::string&result){Lock g(mutex_);std::ofstream out(auditPath_,std::ios::app);out<<Now()<<'\t'<<s.publicId<<'\t'<<s.sessionHashPrefix<<'\t'<<dest<<'\t'<<result<<'\n';chmod(auditPath_.c_str(),0600);}
AdminStoreDiagnostics AdminStore::Diagnostics(){Lock g(mutex_);ReloadAllowlistLocked();AdminStoreDiagnostics d;struct stat allow{},sessions{};d.allowlistReadable=stat(allowlistPath_.c_str(),&allow)==0&&access(allowlistPath_.c_str(),R_OK)==0;const bool exists=stat(sessionPath_.c_str(),&sessions)==0;d.sessionStorageReadable=!exists||access(sessionPath_.c_str(),R_OK)==0;auto slash=sessionPath_.find_last_of('/');const std::string parent=slash==std::string::npos?".":sessionPath_.substr(0,slash);d.sessionStorageWritable=(exists?access(sessionPath_.c_str(),W_OK):access(parent.c_str(),W_OK))==0;d.sessionStorageRestricted=!exists||(sessions.st_mode&0077)==0;d.lastSaveSucceeded=lastSaveSucceeded_;d.allowedCharacters=allowed_.size();d.activeSessions=sessions_.size();d.malformedSessionRecords=malformedSessionRecords_;return d;}
