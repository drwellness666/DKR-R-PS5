// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5_internet_update.hpp"
#include "ps5_native_update.hpp"
#include <unistd.h>
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <thread>
#include <sys/stat.h>
namespace {
std::string mode,channel,old_channel,payload,sent;
struct Record {std::string url;std::size_t at=0;};std::map<int,Record> records;
int next_id=10,connections=0,preparations=0;
std::string file(const char* p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
bool is_catalog(int id){return records[id].url.find("raw.githubusercontent.com/")!=std::string::npos;}
bool is_redirect(int id){return (mode=="redirect"||mode=="bad-redirect")&&!is_catalog(id)&&records[id].url.find("github.com/")!=std::string::npos;}
}
extern "C" {
int __real_lstat(const char*,struct stat*);
int __real_fstat(int,struct stat*);
int __wrap_lstat(const char* path,struct stat* st){int r=__real_lstat(path,st);if(r==0&&mode=="zero-links")st->st_nlink=0;return r;}
int __wrap_fstat(int fd,struct stat* st){int r=__real_fstat(fd,st);if(r==0&&mode=="zero-links")st->st_nlink=0;return r;}
int sceNetPoolCreate(const char*,int,int){return 1;} int sceNetPoolDestroy(int){return 0;}
int sceSslInit(std::size_t){return 2;} int sceSslTerm(int){return 0;}
int sceHttpInit(int,int,std::size_t){return 3;} int sceHttpTerm(int){return 0;}
int sceHttpCreateTemplate(int,const char*,int,int){return 4;} int sceHttpDeleteTemplate(int){return 0;}
int sceHttpCreateConnectionWithURL(int,const char* url,int){int id=next_id++;records[id]={url,0};++connections;return id;}
int sceHttpDeleteConnection(int){return 0;}
int sceHttpCreateRequestWithURL(int id,int,const char*,std::uint64_t){return id;}
int sceHttpDeleteRequest(int){return 0;} int sceHttpSetAutoRedirect(int,int enabled){assert(enabled==0);return 0;}
int sceHttpSetResolveTimeOut(int,std::uint32_t n){assert(n==5000000);return 0;}
int sceHttpSetConnectTimeOut(int,std::uint32_t n){assert(n==5000000);return 0;}
int sceHttpSetSendTimeOut(int,std::uint32_t n){assert(n==5000000);return 0;}
int sceHttpSetRecvTimeOut(int,std::uint32_t n){assert(n==5000000);return 0;}
int sceHttpsEnableOption(int,std::uint32_t flags){assert(flags==0xBD);return 0;}
int sceHttpSendRequest(int,const void*,std::size_t){return 0;}
int sceHttpGetStatusCode(int id,int* status){*status=mode=="offline"?404:is_redirect(id)?302:200;return 0;}
int sceHttpGetAllResponseHeaders(int id,char** out,std::size_t* size){
    assert(is_redirect(id));static std::string headers;
    headers="HTTP/1.1 302 Found\r\nLocation: "+std::string(mode=="bad-redirect"?"https://evil.example/x":"https://release-assets.githubusercontent.com/asset?x=1&sig=2")+"\r\n\r\n";
    *out=headers.data();*size=headers.size();return 0;
}
int sceHttpReadData(int id,void* data,std::size_t size){
    auto& r=records[id];const auto& body=is_catalog(id)?(mode=="current"?old_channel:channel):payload;
    if(!is_catalog(id)&&mode=="cancel")std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto n=std::min({size,body.size()-r.at,std::size_t(17)});std::memcpy(data,body.data()+r.at,n);r.at+=n;return int(n);
}

}
namespace dkrr::updates::native {
bool storage_budget(std::uint64_t bytes,Space& space,char* error,std::size_t capacity) {
    assert(bytes==256);space={16*1024*1024,mode=="space-low"?1024U:64U*1024*1024,mode!="space-unknown"};
    if(mode=="space-low"){std::snprintf(error,capacity,"Not enough free space; nothing downloaded.");return false;}
    return true;
}
bool prepare(int fd,const Offer& offer,char* error,std::size_t capacity) {
    ++preparations;assert(offer.native_package&&payload_digest(fd,offer));
    std::string readback(payload.size(),'\0');assert(read(fd,readback.data(),readback.size())==static_cast<ssize_t>(readback.size())&&readback==payload);
    if(mode=="prepare-fail"){std::snprintf(error,capacity,"Native preparation failed.");return false;}return true;
}
}
int main(int argc,char** argv){
    assert(argc==5);mode=argv[1];channel=file(argv[2]);old_channel=file(argv[3]);payload=file(argv[4]);
    if(mode=="signature")channel[channel.size()-2]^=1;
    if(mode=="digest")payload[64]^=1;
    if(mode=="oversize")payload+='x';
    using State=dkrr::updates::State;
    const auto wait=[&](auto predicate){auto end=std::chrono::steady_clock::now()+std::chrono::seconds(5);while(std::chrono::steady_clock::now()<end){if(predicate())return;std::this_thread::sleep_for(std::chrono::milliseconds(2));}assert(false);};
    const bool automatic=mode=="auto"||mode=="zero-links";
    assert(dkrr::updates::set_automatic_check(automatic));
    const std::string auto_path=std::string(DKRR_UPDATE_ROOT)+"/preferences/update-client-"+std::to_string(unsigned(getuid()))+"/auto-check";
    if(mode=="fifo-setting"){assert(!unlink(auto_path.c_str()));assert(!mkfifo(auto_path.c_str(),0600));}
    dkrr::updates::initialize();
    assert(dkrr::updates::automatic_check()==automatic);
    if(!automatic)dkrr::updates::check();
    wait([]{return dkrr::updates::view().state!=State::Checking;});
    auto state=dkrr::updates::view().state;
    if(mode=="offline"){assert(state==State::Unavailable);}
    else if(mode=="current"){assert(state==State::Current);}
    else if(mode=="signature"){assert(state==State::Failed);}
    else {
        assert(state==State::Available);dkrr::updates::install();
        if(mode=="cancel"){wait([]{return dkrr::updates::view().received>0;});dkrr::updates::cancel();}
        wait([]{auto s=dkrr::updates::view().state;return s==State::Closing||s==State::Failed;});
        const bool good=mode=="good"||mode=="redirect"||mode=="space-unknown"||mode=="fifo-setting"||automatic;
        assert(dkrr::updates::close_requested()==good);
        if(good){assert(dkrr::updates::view().state==State::Closing&&preparations==1);}
        else assert(dkrr::updates::view().state==State::Failed);
    }
    dkrr::updates::shutdown();
    if(mode=="fifo-setting"){assert(!dkrr::updates::set_automatic_check(false));assert(!unlink(auto_path.c_str()));}
    assert(dkrr::updates::set_automatic_check(false));
    assert(!dkrr::updates::automatic_check());
    if(mode=="digest"||mode=="oversize"||mode=="signature"||mode=="offline"||mode=="current"||mode=="cancel"||mode=="bad-redirect")assert(preparations==0);
    if(mode=="prepare-fail")assert(preparations==1&&!dkrr::updates::close_requested());
    if(mode=="space-low")assert(preparations==0&&connections==1&&dkrr::updates::view().received==0);
    assert(connections>=1);std::printf("PASS native update worker: %s\n",mode.c_str());
}
