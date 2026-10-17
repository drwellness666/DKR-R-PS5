// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

namespace dkrr::preferences {
// Reserve only a new entry. Interrupted legacy writes and occupied entries
// (including symlinks) remain untouched, so they cannot block the next save.
template<std::size_t N>
int reserve(const char* prefix,char (&path)[N]) {
    for(unsigned slot=0;slot<128;++slot) {
        const int n=std::snprintf(path,N,"%s.%u",prefix,slot);
        if(n<0||std::size_t(n)>=N){errno=ENAMETOOLONG;return -1;}
        const int fd=open(path,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0644);
        if(fd>=0)return fd;
        if(errno!=EEXIST)return -1;
    }
    errno=EEXIST;return -1;
}
inline int sync_parent(const char* path) {
    char parent[512];const auto n=std::strlen(path);
    if(n>=sizeof(parent)){errno=ENAMETOOLONG;return -1;}
    std::memcpy(parent,path,n+1);char* slash=std::strrchr(parent,'/');
    if(!slash){errno=EINVAL;return -1;}
    if(slash==parent)slash[1]=0;else *slash=0;
    const int fd=open(parent,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);if(fd<0)return -1;
    int result=fsync(fd);if(close(fd)<0)result=-1;return result;
}
}
