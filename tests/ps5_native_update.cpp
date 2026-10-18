// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5_native_update.hpp"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <dirent.h>
#include <map>
#include <vector>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <string>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>
static const char* test_lease_path;
#define DKRR_GAME_LEASE_PATH test_lease_path
#include "ps5_installation_lease.hpp"
static std::string file(const char* p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
extern "C" void dkrr_native_test_hook(const char* event,unsigned index) {
    const char* cut=std::getenv("CUT_EVENT");
    if(cut&&!std::strcmp(cut,event)&&index==unsigned(std::atoi(std::getenv("CUT_INDEX")?std::getenv("CUT_INDEX"):"0")))_exit(77);
}
extern "C" int __real_lstat(const char*,struct stat*);
extern "C" int __real_fstat(int,struct stat*);
extern "C" int __wrap_lstat(const char* p,struct stat* st){
    const std::string path=p;
    if(std::getenv("FAIL_STAT_ROOT")&&path.ends_with("/native-install")){errno=EIO;return -1;}
    if(std::getenv("FAIL_STAT_APPLY_ONCE")&&path.ends_with("/native-install/applying")){static bool failed=false;if(!failed){failed=true;errno=EIO;return -1;}}
    int r=__real_lstat(p,st);if(!r&&std::getenv("ZERO_LINKS"))st->st_nlink=0;return r;
}
extern "C" int __wrap_fstat(int fd,struct stat* st){int r=__real_fstat(fd,st);if(!r&&std::getenv("ZERO_LINKS"))st->st_nlink=0;return r;}
extern "C" int __real_rename(const char*,const char*);
static unsigned replacements;
extern "C" int __real_statvfs(const char*,struct statvfs*);
extern "C" int __wrap_statvfs(const char* path,struct statvfs* out) {
    if(std::getenv("SPACE_UNAVAILABLE")){errno=ENOSYS;return -1;}
    if(std::getenv("SPACE_LOW")){std::memset(out,0,sizeof(*out));out->f_frsize=4096;out->f_bavail=1;return 0;}
    return __real_statvfs(path,out);
}
extern "C" int __wrap_rename(const char* from,const char* to) {
    if(std::getenv("FAIL_APP_RENAMES")&&std::strstr(to,DKRR_NATIVE_APP)){errno=EPERM;return -1;}
    if(std::strstr(from,"/native-install/new/")&&std::strstr(to,DKRR_NATIVE_APP)) {
        const char* fail=std::getenv("FAIL_RENAME");
        if(fail&&replacements++==unsigned(std::atoi(fail))){errno=EIO;return -1;}
    }
    return __real_rename(from,to);
}
extern "C" ssize_t __real_write(int,const void*,size_t);
extern "C" ssize_t __wrap_write(int fd,const void* data,size_t size) {
    if(std::getenv("FAIL_WRITE")||std::getenv("FAIL_APPLY_MARKER")) {
        char link[64],path[1024];std::snprintf(link,sizeof(link),"/proc/self/fd/%d",fd);
        auto n=readlink(link,path,sizeof(path)-1);if(n>=0){path[n]=0;if(std::getenv("FAIL_WRITE")&&std::strstr(path,"/native-install/new/eboot.bin")){errno=ENOSPC;return -1;}
            if(std::getenv("FAIL_APPLY_MARKER")&&std::strstr(path,"/native-install/applying")){static bool partial=false;if(partial){errno=ENOSPC;return -1;}partial=true;return __real_write(fd,data,std::min(size,size_t(2)));}}
    }
    return __real_write(fd,data,size);
}
extern "C" int __real_unlink(const char*);
extern "C" int __wrap_unlink(const char* path) {
    int result=__real_unlink(path);
    if(result==0&&std::strstr(path,"/native-install/old.manifest"))dkrr_native_test_hook("removed-journal",0);
    return result;
}
extern "C" int __real_fsync(int);
extern "C" int __wrap_fsync(int fd) {
    int result=__real_fsync(fd);const char* trace=std::getenv("FSYNC_TRACE");
    if(result==0&&trace){char link[64],path[1024];std::snprintf(link,sizeof(link),"/proc/self/fd/%d",fd);auto n=readlink(link,path,sizeof(path)-2);
        if(n>=0){path[n++]='\n';int log=open(trace,O_WRONLY|O_CREAT|O_APPEND|O_NOFOLLOW,0600);if(log>=0){__real_write(log,path,std::size_t(n));close(log);}}}
    return result;
}
extern "C" struct dirent* __real_readdir(DIR*);
extern "C" int __real_closedir(DIR*);
struct OrderedEntries {bool read=false;std::size_t at=0;std::vector<dirent> entries;};
static std::map<DIR*,OrderedEntries> ordered_entries;
extern "C" struct dirent* __wrap_readdir(DIR* dir) {
    if(!std::getenv("ORDER_JOURNAL_FIRST"))return __real_readdir(dir);
    char link[64],path[1024];std::snprintf(link,sizeof(link),"/proc/self/fd/%d",dirfd(dir));auto n=readlink(link,path,sizeof(path)-1);
    if(n<0)return __real_readdir(dir);path[n]=0;std::string full=path;
    if(!full.ends_with("/native-install"))return __real_readdir(dir);
    auto& state=ordered_entries[dir];
    if(!state.read){while(auto* entry=__real_readdir(dir))state.entries.push_back(*entry);state.read=true;
        const auto priority=[](const dirent& e){return !std::strcmp(e.d_name,"old.manifest")?0:!std::strcmp(e.d_name,"applying")?2:1;};
        std::stable_sort(state.entries.begin(),state.entries.end(),[&](const auto& a,const auto& b){return priority(a)<priority(b);});}
    if(state.at==state.entries.size()){errno=0;return nullptr;}return &state.entries[state.at++];
}
extern "C" int __wrap_closedir(DIR* dir){ordered_entries.erase(dir);return __real_closedir(dir);}
int main(int argc,char** argv) {
    assert(argc==7);char error[192]{};std::string action=argv[1];
    test_lease_path=argv[5];
    int lease=dkrr_runtime_installation_lease();assert(lease>=0);
    int result;
    if(action=="space") {
        auto text=file(argv[2]),key=file(argv[3]);assert(key.size()==32);
        dkrr::updates::Offer offer{};assert(dkrr::updates::verify_offer(text.data(),text.size(),reinterpret_cast<const unsigned char*>(key.data()),offer));
        dkrr::updates::native::Space space{};
        result=dkrr::updates::native::storage_budget(offer.bytes,space,error,sizeof(error))?0:-1;
        assert(space.required>=2*offer.bytes+16*1024*1024);
        assert(space.known==!bool(std::getenv("SPACE_UNAVAILABLE")));
        if(std::getenv("SPACE_LOW"))assert(space.available==4096&&space.required>space.available);
    }
    else if(action=="prepare") {
        auto text=file(argv[2]),key=file(argv[3]);assert(key.size()==32);
        dkrr::updates::Offer offer{};assert(dkrr::updates::verify_offer(text.data(),text.size(),reinterpret_cast<const unsigned char*>(key.data()),offer));
        int fd=open(argv[4],O_RDONLY|O_NOFOLLOW);assert(fd>=0);
        result=dkrr::updates::native::prepare(fd,offer,error,sizeof(error))?0:-1;close(fd);
    }else if(action=="apply") {
        // A running process retains its old mapped inode while its directory
        // entry is atomically replaced; the installed app directory stays put.
        std::string path=DKRR_NATIVE_APP "/eboot.bin",bytes=file(path.c_str());
        int fd=open(path.c_str(),O_RDONLY);assert(fd>=0&&!bytes.empty());
        void* mapped=mmap(nullptr,bytes.size(),PROT_READ,MAP_PRIVATE,fd,0);assert(mapped!=MAP_FAILED);
        result=dkrr::updates::native::apply(lease,error,sizeof(error))?0:-1;
        assert(!std::memcmp(mapped,bytes.data(),bytes.size()));munmap(mapped,bytes.size());close(fd);
    }else {assert(action=="recover");result=dkrr::updates::native::recover(lease,error,sizeof(error));}
    close(lease);std::printf("native %s result=%d %s\n",action.c_str(),result,error);
    return result==std::atoi(argv[6])?0:2;
}
