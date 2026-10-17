// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5_native_update.hpp"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <fstream>
#include <iterator>
#include <string>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
static std::string file(const char* p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
extern "C" void dkrr_native_test_hook(const char* event,unsigned index) {
    const char* cut=std::getenv("CUT_EVENT");
    if(cut&&!std::strcmp(cut,event)&&index==unsigned(std::atoi(std::getenv("CUT_INDEX")?std::getenv("CUT_INDEX"):"0")))_exit(77);
}
extern "C" int __real_lstat(const char*,struct stat*);
extern "C" int __real_fstat(int,struct stat*);
extern "C" int __wrap_lstat(const char* p,struct stat* st){int r=__real_lstat(p,st);if(!r&&std::getenv("ZERO_LINKS"))st->st_nlink=0;return r;}
extern "C" int __wrap_fstat(int fd,struct stat* st){int r=__real_fstat(fd,st);if(!r&&std::getenv("ZERO_LINKS"))st->st_nlink=0;return r;}
extern "C" int __real_rename(const char*,const char*);
static unsigned replacements;
extern "C" int __wrap_rename(const char* from,const char* to) {
    if(std::strstr(from,"/native-install/new/")&&std::strstr(to,DKRR_NATIVE_APP)) {
        const char* fail=std::getenv("FAIL_RENAME");
        if(fail&&replacements++==unsigned(std::atoi(fail))){errno=EIO;return -1;}
    }
    return __real_rename(from,to);
}
extern "C" ssize_t __real_write(int,const void*,size_t);
extern "C" ssize_t __wrap_write(int fd,const void* data,size_t size) {
    if(std::getenv("FAIL_WRITE")) {
        char link[64],path[1024];std::snprintf(link,sizeof(link),"/proc/self/fd/%d",fd);
        auto n=readlink(link,path,sizeof(path)-1);if(n>=0){path[n]=0;if(std::strstr(path,"/native-install/new/eboot.bin")){errno=ENOSPC;return -1;}}
    }
    return __real_write(fd,data,size);
}
int main(int argc,char** argv) {
    assert(argc==7);char error[192]{};std::string action=argv[1];
    int lease=open(argv[5],O_RDWR|O_CREAT|O_NOFOLLOW,0600);assert(lease>=0&&flock(lease,LOCK_SH|LOCK_NB)==0);
    int result;
    if(action=="prepare") {
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
