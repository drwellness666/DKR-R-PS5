// SPDX-License-Identifier: GPL-3.0-or-later
// Native HTTPS sequence follows BlackBearReloaded's GPL-3.0-or-later
// ps5-native-app-boilerplate, examples/update-check, commit 44ae1dab529654ea.
// Copyright (C) 2026 BlackBearReloaded (HTTPS service sequence reference).
#include "ps5_internet_update.hpp"
#include "ps5_native_update.hpp"
#ifndef DKRR_UPDATE_KEY_HEADER
#define DKRR_UPDATE_KEY_HEADER "ps5_update_channel.h"
#endif
#include DKRR_UPDATE_KEY_HEADER
#ifndef DKRR_UPDATE_ROOT
#define DKRR_UPDATE_ROOT "/data/dkrr"
#endif
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <thread>
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

extern "C" {
int sceNetPoolCreate(const char*,int,int) __attribute__((weak)); int sceNetPoolDestroy(int) __attribute__((weak));
int sceSslInit(std::size_t) __attribute__((weak)); int sceSslTerm(int) __attribute__((weak));
int sceHttpInit(int,int,std::size_t) __attribute__((weak)); int sceHttpTerm(int) __attribute__((weak));
int sceHttpCreateTemplate(int,const char*,int,int) __attribute__((weak)); int sceHttpDeleteTemplate(int) __attribute__((weak));
int sceHttpCreateConnectionWithURL(int,const char*,int) __attribute__((weak)); int sceHttpDeleteConnection(int) __attribute__((weak));
int sceHttpCreateRequestWithURL(int,int,const char*,std::uint64_t) __attribute__((weak)); int sceHttpDeleteRequest(int) __attribute__((weak));
int sceHttpSetAutoRedirect(int,int) __attribute__((weak)); int sceHttpSetResolveTimeOut(int,std::uint32_t) __attribute__((weak));
int sceHttpSetConnectTimeOut(int,std::uint32_t) __attribute__((weak)); int sceHttpSetSendTimeOut(int,std::uint32_t) __attribute__((weak));
int sceHttpSetRecvTimeOut(int,std::uint32_t) __attribute__((weak)); int sceHttpsEnableOption(int,std::uint32_t) __attribute__((weak));
int sceHttpSendRequest(int,const void*,std::size_t) __attribute__((weak)); int sceHttpGetStatusCode(int,int*) __attribute__((weak));
int sceHttpGetAllResponseHeaders(int,char**,std::size_t*) __attribute__((weak)); int sceHttpReadData(int,void*,std::size_t) __attribute__((weak));

}
namespace dkrr::updates {
namespace {
using Clock=std::chrono::steady_clock;
std::mutex mutex;
View current{};Offer offer{};std::thread worker;
std::atomic_bool stopping{},busy{},exit_ready{},services_ready{},automatic_enabled{};
#ifndef DKRR_RELEASE_SEQUENCE
#error "Compile with the installer release sequence"
#endif
constexpr char catalog[]="https://raw.githubusercontent.com/drwellness666/DKR-R-PS5/main/updates/native-channel.txt";
void state(State value,const char* message) {
    std::lock_guard lock(mutex);current.state=value;
    std::snprintf(current.message,sizeof(current.message),"%s",message);
}
struct Services {
    int pool=-1,ssl=-1,http=-1,tmpl=-1;
    bool start() {
        if((pool=sceNetPoolCreate("dkrr_update",1024*1024,0))<0||
           (ssl=sceSslInit(304*1024))<0||(http=sceHttpInit(pool,ssl,4*1024*1024))<0||
           (tmpl=sceHttpCreateTemplate(http,"DKR-R-PS5/update-1",2,0))<0)return false;
        return sceHttpSetAutoRedirect(tmpl,0)>=0&&sceHttpSetResolveTimeOut(tmpl,5000000)>=0&&
            sceHttpSetConnectTimeOut(tmpl,5000000)>=0&&sceHttpSetSendTimeOut(tmpl,5000000)>=0&&
            sceHttpSetRecvTimeOut(tmpl,5000000)>=0&&sceHttpsEnableOption(tmpl,0xBD)>=0;
    }
    ~Services(){if(tmpl>=0)sceHttpDeleteTemplate(tmpl);if(http>=0)sceHttpTerm(http);if(ssl>=0)sceSslTerm(ssl);if(pool>=0)sceNetPoolDestroy(pool);}
};
struct Request {
    int connection=-1,id=-1,code=0;
    bool start(Services& services,const char* url) {
        connection=sceHttpCreateConnectionWithURL(services.tmpl,url,0);
        if(connection<0)return false;
        id=sceHttpCreateRequestWithURL(connection,0,url,0);
        return id>=0&&sceHttpSendRequest(id,nullptr,0)>=0&&sceHttpGetStatusCode(id,&code)>=0;
    }
    ~Request(){if(id>=0)sceHttpDeleteRequest(id);if(connection>=0)sceHttpDeleteConnection(connection);}
};
bool redirect(int request,char (&next)[2048]) {
    char* headers=nullptr;std::size_t size=0;
    if(sceHttpGetAllResponseHeaders(request,&headers,&size)<0||!headers||!size||size>32768)return false;
    // Response-header storage belongs to sceHttp and remains valid until deletion.
    unsigned matches=0;
    for(std::size_t at=0;at<size;) {
        auto begin=at;while(at<size&&headers[at]!='\n')++at;auto n=at-begin;if(at<size)++at;
        if(n&&headers[begin+n-1]=='\r')--n;
        if(n>=9){const char name[]="location:";bool same=true;for(unsigned i=0;i<9;++i){char c=headers[begin+i];if(c>='A'&&c<='Z')c+=32;if(c!=name[i])same=false;}
            if(same){if(++matches!=1)return false;begin+=9;n-=9;while(n&&(headers[begin]==' '||headers[begin]=='\t')){++begin;--n;}if(!n||n>=sizeof(next))return false;std::memcpy(next,headers+begin,n);next[n]=0;}}
    }
    return matches==1&&allowed_asset_url(next,true);
}
bool write_all(int fd,const unsigned char* data,std::size_t size) {
    std::size_t at=0;while(at<size&&!stopping.load()){auto n=write(fd,data+at,size-at);if(n<0&&errno==EINTR)continue;if(n<=0)return false;at+=static_cast<std::size_t>(n);}return at==size;
}
bool update_directory(char (&folder)[160]) {
    char parents[]=DKRR_UPDATE_ROOT "/preferences";struct stat st{};
    for(std::size_t i=1;i<=sizeof(parents)-1;++i)if(parents[i]=='/'||!parents[i]){
        char saved=parents[i];parents[i]=0;int result=lstat(parents,&st);parents[i]=saved;
        if(result<0||!S_ISDIR(st.st_mode))return false;
    }
    std::snprintf(folder,sizeof(folder),DKRR_UPDATE_ROOT "/preferences/update-client-%u",unsigned(getuid()));
    if(mkdir(folder,0700)<0&&errno!=EEXIST)return false;
    if(lstat(folder,&st)<0||!S_ISDIR(st.st_mode)||st.st_uid!=getuid()||(st.st_mode&077)!=0)return false;
    // Persist the private directory's own entry, including after an earlier
    // interrupted first creation. Syncing only files inside it is insufficient.
    int parent=open(DKRR_UPDATE_ROOT "/preferences",O_RDONLY|O_DIRECTORY|O_NOFOLLOW);if(parent<0)return false;
    bool good=fsync(parent)==0;if(close(parent)<0)good=false;return good;
}
bool load_automatic_check() {
    char folder[160],path[240],text[2];if(!update_directory(folder))return false;
    std::snprintf(path,sizeof(path),"%s/auto-check",folder);
    int fd=open(path,O_RDONLY|O_NOFOLLOW|O_NONBLOCK);if(fd<0)return false;struct stat st{};
    // Native PS5 stat may report zero for unavailable link-count metadata.
    // Read-only access and atomic entry replacement never mutate that inode;
    // match the existing native-save policy without fabricating a link count.
    bool enabled=fstat(fd,&st)==0&&S_ISREG(st.st_mode)&&st.st_nlink<=1&&st.st_uid==getuid()&&
        st.st_size==1&&read(fd,text,sizeof(text))==1&&text[0]=='1';close(fd);return enabled;
}
int temporary_file() {
    char folder[160];if(!update_directory(folder))return -1;
    char path[240];std::snprintf(path,sizeof(path),"%s/download-%u-%llu.elf",folder,unsigned(getpid()),static_cast<unsigned long long>(Clock::now().time_since_epoch().count()));
    int fd=open(path,O_RDWR|O_CREAT|O_EXCL|O_NOFOLLOW,0600);if(fd<0)return -1;
    // The open inode disappears on close, including after a crash. No stale
    // package is trusted or executed on a subsequent game launch.
    if(unlink(path)<0){close(fd);return -1;}return fd;
}
bool download(Services& services,const Offer& release,int fd) {
    char url[2048];std::snprintf(url,sizeof(url),"https://github.com/drwellness666/DKR-R-PS5/releases/download/%s/DKRR-Update.dkru",release.tag);
    const auto deadline=Clock::now()+std::chrono::minutes(10);
    for(unsigned hop=0;hop<4&&!stopping.load();++hop){
        if(!allowed_asset_url(url,hop!=0))return false;
        Request request;if(!request.start(services,url))return false;
        if(request.code==301||request.code==302||request.code==303||request.code==307||request.code==308){char next[2048];if(!redirect(request.id,next))return false;std::memcpy(url,next,std::strlen(next)+1);continue;}
        if(request.code!=200)return false;
        unsigned char buffer[65536];std::uint64_t received=0;
        while(!stopping.load()&&Clock::now()<deadline){
            const int n=sceHttpReadData(request.id,buffer,sizeof(buffer));if(n<0||n>int(sizeof(buffer)))return false;
            if(!n)return received==release.bytes&&payload_digest(fd,release);
            if(static_cast<std::uint64_t>(n)>release.bytes-received||!write_all(fd,buffer,static_cast<std::size_t>(n)))return false;
            received+=static_cast<std::uint64_t>(n);std::lock_guard lock(mutex);current.received=received;
        }return false;
    }return false;
}
void run_check() {
    Services services;if(!services.start()){state(State::Unavailable,"Update service unavailable. The installed game is unchanged.");return;}
    Request request;if(!request.start(services,catalog)||request.code!=200){state(State::Unavailable,"No public update channel is available. The installed game is unchanged.");return;}
    char text[manifest_limit];std::size_t used=0;
    const auto deadline=Clock::now()+std::chrono::seconds(30);
    while(!stopping.load()&&Clock::now()<deadline) {char spare;int n=sceHttpReadData(request.id,used<sizeof(text)?text+used:&spare,used<sizeof(text)?sizeof(text)-used:1);
        if(n<0||static_cast<std::size_t>(n)>sizeof(text)-used){state(State::Failed,"Update information could not be verified.");return;}
        if(!n)break;used+=static_cast<std::size_t>(n);
    }
    Offer result;if(stopping.load())return;
    if(Clock::now()>=deadline){state(State::Unavailable,"Update check timed out.");return;}
    if(!verify_offer(text,used,dkrr_update_public_key,result)||!result.native_package){state(State::Failed,"Update signature is invalid. Nothing was downloaded or installed.");return;}
    native::Space space{};char storage_error[192]{};
    if(result.sequence>DKRR_RELEASE_SEQUENCE)native::storage_budget(result.bytes,space,storage_error,sizeof(storage_error));
    {std::lock_guard lock(mutex);offer=result;current.sequence=result.sequence;current.bytes=result.bytes;current.received=0;
        current.space_required=space.required;current.space_available=space.available;current.space_known=space.known;}
    state(result.sequence>DKRR_RELEASE_SEQUENCE?State::Available:State::Current,result.sequence>DKRR_RELEASE_SEQUENCE?"A verified update is available.":"You have the latest available version.");
}
void run_install() {
    Offer release;{std::lock_guard lock(mutex);release=offer;}
    if(release.sequence<=DKRR_RELEASE_SEQUENCE){state(State::Failed,"No newer verified release is selected.");return;}
    native::Space space{};char error[192]{};
    if(!native::storage_budget(release.bytes,space,error,sizeof(error))){state(State::Failed,error);return;}
    Services services;int fd=temporary_file();
    struct Close {int fd;~Close(){if(fd>=0)close(fd);}} close_file{fd};
    if(fd<0||!services.start()||!download(services,release,fd)){state(State::Failed,"Download cancelled or verification failed. The installed game is unchanged.");return;}
    if(stopping.load())return;
    state(State::HandingOff,"Preparing installation on this PS5...");
    if(!native::prepare(fd,release,error,sizeof(error))){state(State::Failed,error[0]?error:"The native update could not be prepared. The installed game is unchanged.");return;}
    state(State::Closing,"Saving and closing DKR-R. The game installs this update itself.");
    exit_ready.store(true,std::memory_order_release);
}
void launch(bool installing) {
    if(!services_ready.load())return;
    bool expected=false;if(!busy.compare_exchange_strong(expected,true))return;
    if(worker.joinable())worker.join();stopping.store(false);
    if(installing){std::lock_guard lock(mutex);if(current.state!=State::Available){busy.store(false);return;}current.state=State::Downloading;current.received=0;}
    else state(State::Checking,"Checking for updates...");
    try {worker=std::thread([installing]{try {if(installing)run_install();else run_check();}catch(...){state(State::Failed,"Update stopped. The installed game is unchanged.");}busy.store(false);});}
    catch(...){busy.store(false);state(State::Unavailable,"Update worker could not start.");}
}
}
void initialize(){
    if(!sceNetPoolCreate||!sceNetPoolDestroy||!sceSslInit||!sceSslTerm||!sceHttpInit||!sceHttpTerm||
       !sceHttpCreateTemplate||!sceHttpDeleteTemplate||!sceHttpCreateConnectionWithURL||!sceHttpDeleteConnection||
       !sceHttpCreateRequestWithURL||!sceHttpDeleteRequest||!sceHttpSetAutoRedirect||!sceHttpSetResolveTimeOut||
       !sceHttpSetConnectTimeOut||!sceHttpSetSendTimeOut||!sceHttpSetRecvTimeOut||!sceHttpsEnableOption||
       !sceHttpSendRequest||!sceHttpGetStatusCode||!sceHttpGetAllResponseHeaders||!sceHttpReadData){
        state(State::Unavailable,"Native update services are unavailable. ELF updates remain available.");return;
    }
    services_ready.store(true);automatic_enabled.store(load_automatic_check());
    state(State::Idle,"Check for updates to download and install directly on this PS5.");
    if(automatic_enabled.load())check();
}
void check(){launch(false);}
void install(){launch(true);}
void cancel(){stopping.store(true);}
void shutdown(){cancel();if(worker.joinable())worker.join();}
View view(){std::lock_guard lock(mutex);return current;}
bool close_requested(){return exit_ready.load(std::memory_order_acquire);}
bool automatic_check(){return automatic_enabled.load();}
bool set_automatic_check(bool enabled){
    char folder[160],path[240],pending[260];if(!update_directory(folder))return false;
    std::snprintf(path,sizeof(path),"%s/auto-check",folder);
    struct stat st{};if(lstat(path,&st)==0){if(!S_ISREG(st.st_mode)||st.st_uid!=getuid()||st.st_nlink>1)return false;}else if(errno!=ENOENT)return false;
    std::snprintf(pending,sizeof(pending),"%s/auto-check-%u-%llu",folder,unsigned(getpid()),static_cast<unsigned long long>(Clock::now().time_since_epoch().count()));
    int fd=open(pending,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);if(fd<0)return false;
    char value=enabled?'1':'0';bool good=write(fd,&value,1)==1&&fsync(fd)==0;if(close(fd)<0)good=false;
    if(good)good=rename(pending,path)==0;
    if(!good){unlink(pending);return false;}
    fd=open(folder,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);if(fd<0)return false;good=fsync(fd)==0;if(close(fd)<0)good=false;
    if(good)automatic_enabled.store(enabled);return good;
}
}
