#pragma once

#include <cstddef>
#include <array>
#include <cstdint>
#include <string_view>

namespace Unicode {

// Converts strict UTF-8 to Unreal's UTF-16 TCHAR representation. Capacity
// includes the trailing NUL. Invalid, overlong, surrogate, truncated, and
// out-of-range sequences fail without exposing a partial string to callers.
inline bool Utf8ToUtf16(std::string_view input,char16_t* output,std::size_t capacity,std::size_t* outputUnits=nullptr){
    if(!output||capacity==0)return false;
    std::size_t in=0,out=0;
    while(in<input.size()){
        const auto first=static_cast<std::uint8_t>(input[in]);
        std::uint32_t scalar=0;std::size_t bytes=0;
        if(first<=0x7f){scalar=first;bytes=1;}
        else if(first>=0xc2&&first<=0xdf){scalar=first&0x1f;bytes=2;}
        else if(first>=0xe0&&first<=0xef){scalar=first&0x0f;bytes=3;}
        else if(first>=0xf0&&first<=0xf4){scalar=first&0x07;bytes=4;}
        else {output[0]=0;return false;}
        if(in+bytes>input.size()){output[0]=0;return false;}
        for(std::size_t i=1;i<bytes;++i){const auto next=static_cast<std::uint8_t>(input[in+i]);if((next&0xc0)!=0x80){output[0]=0;return false;}scalar=(scalar<<6)|(next&0x3f);}
        if((bytes==2&&scalar<0x80)||(bytes==3&&scalar<0x800)||(bytes==4&&scalar<0x10000)||scalar>0x10ffff||(scalar>=0xd800&&scalar<=0xdfff)){output[0]=0;return false;}
        if(scalar<=0xffff){if(out+1>=capacity){output[0]=0;return false;}output[out++]=static_cast<char16_t>(scalar);}
        else {if(out+2>=capacity){output[0]=0;return false;}scalar-=0x10000;output[out++]=static_cast<char16_t>(0xd800+(scalar>>10));output[out++]=static_cast<char16_t>(0xdc00+(scalar&0x3ff));}
        in+=bytes;
    }
    output[out]=0;if(outputUnits)*outputUnits=out;return true;
}

inline bool IsValidUtf8(std::string_view input,std::size_t maximumUtf16Units){
    if(maximumUtf16Units>64)return false;
    std::array<char16_t,65> scratch{};return Utf8ToUtf16(input,scratch.data(),maximumUtf16Units+1);
}

}
