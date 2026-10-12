#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
struct R{std::uint64_t address,unused;std::uint32_t nameOffset;} __attribute__((packed));
int main(int argc,char**argv){
 if(argc<3)return 2;int fd=open(argv[1],O_RDONLY);struct stat st{};if(fd<0||fstat(fd,&st))return 3;
 auto*p=(const unsigned char*)mmap(nullptr,st.st_size,PROT_READ,MAP_PRIVATE,fd,0);if(p==MAP_FAILED)return 4;
 std::uint32_t count=0;std::memcpy(&count,p,4);std::uint64_t namesAt=4ull+std::uint64_t(count)*sizeof(R);if(namesAt>=std::uint64_t(st.st_size))return 5;
 auto*r=(const R*)(p+4);const char*names=(const char*)p+namesAt;std::size_t namesSize=st.st_size-namesAt;
 for(std::uint32_t i=0;i<count;++i){if(r[i].nameOffset>=namesSize)continue;const char*n=names+r[i].nameOffset;const void*end=std::memchr(n,'\n',namesSize-r[i].nameOffset);std::size_t len=end?static_cast<const char*>(end)-n:namesSize-r[i].nameOffset;std::string_view s(n,len);for(int a=2;a<argc;++a)if(s.find(argv[a])!=std::string_view::npos){std::cout<<std::hex<<r[i].address<<'\t'<<s<<'\n';break;}}
 munmap((void*)p,st.st_size);close(fd);
}
