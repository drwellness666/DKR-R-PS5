// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5_internet_update.hpp"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <fcntl.h>
#include <unistd.h>
static std::string read(const char* path){std::ifstream f(path,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
int main(int argc,char** argv){
    assert(argc==4||argc==5);auto message=read(argv[1]),key=read(argv[2]);assert(key.size()==32);
    dkrr::updates::Offer offer{};auto* public_key=reinterpret_cast<const unsigned char*>(key.data());
    if(argc==5&&!strcmp(argv[4],"--invalid-channel")) {
        assert(!dkrr::updates::verify_offer(message.data(),message.size(),public_key,offer));
        puts("PASS signed oversized update offer rejected");return 0;
    }
    assert(dkrr::updates::verify_offer(message.data(),message.size(),public_key,offer));
    if(argc==5){
        assert(!strcmp(argv[4],"--channel")||!strcmp(argv[4],"--release"));
        if(!strcmp(argv[4],"--release")){int fd=open(argv[3],O_RDONLY|O_NOFOLLOW);assert(fd>=0&&dkrr::updates::payload_digest(fd,offer));close(fd);}
        std::printf("PASS actual release channel: sequence=%u bytes=%llu tag=%s\n",offer.sequence,static_cast<unsigned long long>(offer.bytes),offer.tag);return 0;
    }
    assert(offer.sequence==33&&offer.bytes==256&&!strcmp(offer.tag,"ps5-v1.0.0"));
    const auto good=offer;
    for(std::size_t n=0;n<message.size();++n)assert(!dkrr::updates::verify_offer(message.data(),n,public_key,offer));
    for(std::size_t n=0;n<message.size();++n){auto changed=message;changed[n]^=1;assert(!dkrr::updates::verify_offer(changed.data(),changed.size(),public_key,offer));}
    assert(offer.sequence==good.sequence&&offer.bytes==good.bytes&&!strcmp(offer.tag,good.tag));
    auto wrong=key;wrong[0]^=1;assert(!dkrr::updates::verify_offer(message.data(),message.size(),reinterpret_cast<const unsigned char*>(wrong.data()),offer));
    auto extra=message+"\n";assert(!dkrr::updates::verify_offer(extra.data(),extra.size(),public_key,offer));
    unsigned seed=9329;
    for(unsigned k=0;k<10000;++k){char fuzz[1025];for(auto& c:fuzz){seed=seed*1664525+1013904223;c=static_cast<char>(seed>>24);}assert(!dkrr::updates::verify_offer(fuzz,k%sizeof(fuzz),public_key,offer));}
    assert(dkrr::updates::allowed_asset_url("https://github.com/drwellness666/DKR-R-PS5/releases/download/ps5-v1.0.0/DKRR-Network-Update.elf",false));
    assert(dkrr::updates::allowed_asset_url("https://release-assets.githubusercontent.com/asset?sig=abc&x=y",true));
    const char* bad[]={"http://github.com/drwellness666/DKR-R-PS5/releases/download/x","https://github.com.evil/x","https://release-assets.githubusercontent.com.evil/x","https://release-assets.githubusercontent.com@evil/x","https://evil/x","https://github.com/drwellness666/DKR-R-PS5/releases/download/\r\nx","https://github.com/drwellness666/DKR-R-PS5/releases/download/x#fragment","https://github.com/drwellness666/DKR-R-PS5/releases/download/\\x"};
    for(auto* url:bad)assert(!dkrr::updates::allowed_asset_url(url,true));
    assert(!dkrr::updates::allowed_asset_url("https://release-assets.githubusercontent.com/x",false));
    int fd=open(argv[3],O_RDWR|O_NOFOLLOW);assert(fd>=0&&dkrr::updates::payload_digest(fd,offer));
    auto wrong_size=offer;--wrong_size.bytes;assert(!dkrr::updates::payload_digest(fd,wrong_size));
    unsigned char changed=0;assert(pwrite(fd,&changed,1,64)==1);assert(!dkrr::updates::payload_digest(fd,offer));close(fd);
    puts("PASS signed update policy: all truncations and byte mutations, wrong key, 10000 malformed inputs, redirect allowlist and payload integrity");
}
