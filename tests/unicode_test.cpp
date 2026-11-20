#include "utils/unicode.h"

#include <cassert>
#include <string>

int main(){
    char16_t output[97]{};std::size_t units=0;
    assert(Unicode::Utf8ToUtf16("\xe2\x88\x9e",output,97,&units));
    assert(units==1&&output[0]==u'\u221e'&&output[1]==0);
    assert(Unicode::Utf8ToUtf16("crate \xf0\x9f\x90\x89",output,97,&units));
    assert(units==8&&output[6]==0xd83d&&output[7]==0xdc09&&output[8]==0);
    assert(!Unicode::Utf8ToUtf16(std::string("\xc0\x80",2),output,97));
    assert(!Unicode::Utf8ToUtf16(std::string("\xed\xa0\x80",3),output,97));
    assert(!Unicode::Utf8ToUtf16(std::string("\xf4\x90\x80\x80",4),output,97));
    assert(!Unicode::Utf8ToUtf16(std::string("\xe2\x88",2),output,97));
    assert(!Unicode::IsValidUtf8(std::string(65,'a'),64));
    assert(Unicode::IsValidUtf8(std::string(64,'a'),64));
    std::string infinity64,infinity65;for(int i=0;i<64;++i)infinity64+="\xe2\x88\x9e";infinity65=infinity64+"\xe2\x88\x9e";
    assert(Unicode::IsValidUtf8(infinity64,64));
    assert(!Unicode::IsValidUtf8(infinity65,64));
}
