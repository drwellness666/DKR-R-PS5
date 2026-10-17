// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5_internet_update.hpp"
#include <charconv>
#include <cstring>
#include <string_view>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
extern "C" {
#include "monocypher-ed25519.h"
#include "mbedtls/sha256.h"
}
namespace dkrr::updates {
namespace {
int hex(char c) {return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:-1;}
bool unhex(std::string_view s,unsigned char* out,std::size_t size) {
    if(s.size()!=size*2)return false;
    for(std::size_t i=0;i<size;++i){int a=hex(s[i*2]),b=hex(s[i*2+1]);if(a<0||b<0)return false;out[i]=static_cast<unsigned char>(a*16+b);}
    return true;
}
bool number(std::string_view s,std::uint64_t& value) {
    if(s.empty()||s.size()>12||s.front()<'1'||s.front()>'9')return false;
    auto end=s.data()+s.size();auto result=std::from_chars(s.data(),end,value);
    return result.ec==std::errc{}&&result.ptr==end;
}
}
bool verify_offer(const char* text,std::size_t size,const unsigned char key[32],Offer& offer) {
    if(!text||!key||!size||size>manifest_limit)return false;
    std::string_view rest(text,size),lines[8];std::size_t signed_size=0;
    for(unsigned i=0;i<8;++i){auto at=rest.find('\n');if(at==rest.npos)return false;lines[i]=rest.substr(0,at);rest.remove_prefix(at+1);if(i==6)signed_size=size-rest.size();}
    const bool native=lines[0]=="DKRR-UPDATE-2"&&lines[6]=="DKRR-Update.dkru";
    const bool legacy=lines[0]=="DKRR-UPDATE-1"&&lines[6]=="DKRR-Network-Update.elf";
    if(!rest.empty()||(!native&&!legacy)||lines[1]!="PPSA94678")return false;
    Offer result{};std::uint64_t sequence=0;
    result.native_package=native;
    if(!number(lines[2],sequence)||sequence>2147483647||!number(lines[4],result.bytes)||result.bytes<64||result.bytes>payload_limit)return false;
    result.sequence=static_cast<unsigned>(sequence);
    auto tag=lines[3];if(tag.size()<10||tag.size()>=sizeof(result.tag)||!tag.starts_with("ps5-"))return false;
    for(char c:tag)if(!((c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='-'||c=='.'))return false;
    if(tag.find("..")!=tag.npos)return false;
    std::memcpy(result.tag,tag.data(),tag.size());
    unsigned char signature[64];
    if(!unhex(lines[5],result.sha256,32)||!unhex(lines[7],signature,64))return false;
    if(crypto_ed25519_check(signature,key,reinterpret_cast<const unsigned char*>(text),signed_size)!=0)return false;
    offer=result;return true;
}
bool allowed_asset_url(const char* url,bool redirected) {
    if(!url)return false;
    const auto n=strnlen(url,2048);if(!n||n>=2048)return false;
    std::string_view value(url,n);
    for(unsigned char c:value)if(c<=32||c>126||c=='\\'||c=='#')return false;
    const std::string_view root="https://github.com/drwellness666/DKR-R-PS5/releases/download/";
    const std::string_view assets="https://release-assets.githubusercontent.com/";
    return value.starts_with(root)||(redirected&&value.starts_with(assets));
}
bool payload_digest(int fd,const Offer& offer) {
    struct stat st{};unsigned char header[20],digest[32],buffer[65536];
    if(fd<0||fstat(fd,&st)<0||!S_ISREG(st.st_mode)||st.st_nlink>1||st.st_size<0||static_cast<std::uint64_t>(st.st_size)!=offer.bytes||lseek(fd,0,SEEK_SET)<0)return false;
    if(read(fd,header,sizeof(header))!=sizeof(header))return false;
    if(offer.native_package) {if(std::memcmp(header,"DKRRPK2\n",8))return false;}
    else if(std::memcmp(header,"\177ELF\2\1\1",7)||header[18]!=62||header[19])return false;
    if(lseek(fd,0,SEEK_SET)<0)return false;
    mbedtls_sha256_context sha;mbedtls_sha256_init(&sha);bool ok=mbedtls_sha256_starts(&sha,0)==0;std::uint64_t count=0;
    while(ok){auto n=read(fd,buffer,sizeof(buffer));if(n<0&&errno==EINTR)continue;if(n<0){ok=false;break;}if(!n)break;count+=static_cast<std::uint64_t>(n);if(count>offer.bytes||mbedtls_sha256_update(&sha,buffer,static_cast<std::size_t>(n))!=0)ok=false;}
    if(ok)ok=mbedtls_sha256_finish(&sha,digest)==0&&count==offer.bytes&&std::memcmp(digest,offer.sha256,32)==0;
    mbedtls_sha256_free(&sha);return ok&&lseek(fd,0,SEEK_SET)==0;
}
}
