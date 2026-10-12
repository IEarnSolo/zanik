#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
struct R{std::uint64_t address,unused;std::uint32_t nameOffset;} __attribute__((packed));
int main(int argc,char**argv){
 if(argc!=3)return 2;std::ifstream f(argv[1],std::ios::binary);std::uint32_t count=0;f.read((char*)&count,4);
 std::vector<R> records(count);f.read((char*)records.data(),records.size()*sizeof(R));
 if(std::string(argv[2]).rfind("offset:",0)==0){auto wanted=std::stoul(argv[2]+7);for(const auto&r:records)if(r.nameOffset==wanted)std::cout<<std::hex<<r.address<<'\n';return 0;}
 std::string names((std::istreambuf_iterator<char>(f)),{}),needle=argv[2];
 for(const auto&r:records){if(r.nameOffset>=names.size())continue;auto e=names.find('\n',r.nameOffset);auto n=names.substr(r.nameOffset,e-r.nameOffset);if(n.find(needle)!=std::string::npos)std::cout<<std::hex<<r.address<<'\t'<<n<<'\n';}
}
