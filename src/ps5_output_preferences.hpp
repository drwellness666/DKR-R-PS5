// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "ps5_display_options.h"
#include "ps5_hdr_policy.hpp"
#include "ps5_preference_io.hpp"
#include <mutex>
#include <sys/stat.h>
#ifndef DKR_DISPLAY_DATA_ROOT
#define DKR_DISPLAY_DATA_ROOT "/data/dkrr"
#endif

namespace dkrr::output_preferences {
struct Choice {
    dkrr_ps5_display_choice display{1080,60};
    dkrr_ps5_hdr_choice hdr{0,200,800,35};
};
inline std::mutex mutex;
constexpr char directory[]=DKR_DISPLAY_DATA_ROOT "/preferences";
constexpr char path[]=DKR_DISPLAY_DATA_ROOT "/preferences/tv-output.txt";
constexpr char temporary[]=DKR_DISPLAY_DATA_ROOT "/preferences/tv-output.txt.next";
inline bool valid(dkrr_ps5_display_choice c) {
    return (c.resolution==1080||c.resolution==1440||c.resolution==2160)&&
           (c.refresh==60||c.refresh==120);
}
inline bool safe_directory() {
    struct stat st{};
    return !lstat(DKR_DISPLAY_DATA_ROOT,&st)&&S_ISDIR(st.st_mode)&&
           !lstat(directory,&st)&&S_ISDIR(st.st_mode);
}
// Nonblocking, bounded regular-file reads. Missing entries alone permit the
// legacy fallback; malformed, linked or unreadable current records never do.
inline int text(const char* file,char (&bytes)[256],std::size_t& size) {
    size=0;const int fd=open(file,O_RDONLY|O_NOFOLLOW|O_NONBLOCK);
    if(fd<0)return errno==ENOENT?0:-1;
    struct stat st{};
    bool ok=!fstat(fd,&st)&&S_ISREG(st.st_mode)&&st.st_nlink<=1&&st.st_size>0&&st.st_size<255;
    while(ok&&size<255) {
        const auto n=read(fd,bytes+size,255-size);
        if(n<0&&errno==EINTR)continue;
        if(n<0){ok=false;break;}if(!n)break;size+=std::size_t(n);
    }
    if(close(fd))ok=false;bytes[size]=0;
    if(!ok){errno=EINVAL;return -1;}return 1;
}
inline int format(char (&bytes)[256],Choice c) {
    return std::snprintf(bytes,sizeof(bytes),"DKRR-TV-1\nresolution=%up\nrefresh=%u\nhdr=%u\npaper_white=%u\npeak=%u\nexpansion=%u\n",
        c.display.resolution,c.display.refresh,c.hdr.enabled,c.hdr.paper_white,c.hdr.peak,c.hdr.expansion);
}
inline int load(Choice& out) {
    out={};if(!safe_directory()){errno=ENOTDIR;return -1;}
    char bytes[256]{},canonical[256];std::size_t size{};Choice c;
    int found=text(path,bytes,size);
    if(found<0)return -1;
    if(found) {
        if(std::sscanf(bytes,"DKRR-TV-1\nresolution=%up\nrefresh=%u\nhdr=%u\npaper_white=%u\npeak=%u\nexpansion=%u\n",
            &c.display.resolution,&c.display.refresh,&c.hdr.enabled,&c.hdr.paper_white,&c.hdr.peak,&c.hdr.expansion)!=6||
            !valid(c.display)||!hdr::valid(c.hdr)||std::size_t(format(canonical,c))!=size||std::memcmp(bytes,canonical,size)) {
            errno=EINVAL;return -1;
        }
        out=c;return 1;
    }
    const int display=text(DKR_DISPLAY_DATA_ROOT "/preferences/display-output.txt",bytes,size);
    if(display<0)return -1;
    if(display) {
        const int n=std::sscanf(bytes,"resolution=%up\nrefresh=%u\n",&c.display.resolution,&c.display.refresh);
        const int length=std::snprintf(canonical,sizeof(canonical),"resolution=%up\nrefresh=%u\n",c.display.resolution,c.display.refresh);
        if(n!=2||!valid(c.display)||std::size_t(length)!=size||std::memcmp(bytes,canonical,size)){errno=EINVAL;return -1;}
    }
    const int colour=text(DKR_DISPLAY_DATA_ROOT "/preferences/hdr-output.txt",bytes,size);
    if(colour<0)return -1;
    if(colour) {
        const int n=std::sscanf(bytes,"hdr=%u\npaper_white=%u\npeak=%u\nexpansion=%u\n",&c.hdr.enabled,&c.hdr.paper_white,&c.hdr.peak,&c.hdr.expansion);
        const int length=std::snprintf(canonical,sizeof(canonical),"hdr=%u\npaper_white=%u\npeak=%u\nexpansion=%u\n",c.hdr.enabled,c.hdr.paper_white,c.hdr.peak,c.hdr.expansion);
        if(n!=4||!hdr::valid(c.hdr)||std::size_t(length)!=size||std::memcmp(bytes,canonical,size)){errno=EINVAL;return -1;}
    }
    out=c;return display||colour?1:0;
}
inline int read(Choice& out) {std::scoped_lock lock(mutex);return load(out);}
inline int save(const dkrr_ps5_display_choice* display,const dkrr_ps5_hdr_choice* colour) {
    std::scoped_lock lock(mutex);
    if((display&&!valid(*display))||(colour&&!hdr::valid(*colour))){errno=EINVAL;return -1;}
    struct stat before{},after{};const bool existed=!lstat(path,&before);
    if((existed&&(!S_ISREG(before.st_mode)||before.st_nlink>1))||(!existed&&errno!=ENOENT)){errno=EINVAL;return -1;}
    Choice choice;if(load(choice)<0)return -1;
    if(display)choice.display=*display;if(colour)choice.hdr=*colour;
    char next[512];const int fd=preferences::reserve(temporary,next);if(fd<0)return -1;
    char bytes[256];const auto size=std::size_t(format(bytes,choice));std::size_t at=0;bool ok=true;
    while(at<size) {
        const auto n=write(fd,bytes+at,size-at);if(n<0&&errno==EINTR)continue;
        if(n<=0){ok=false;break;}at+=std::size_t(n);
    }
    if(fchmod(fd,0644)||fsync(fd))ok=false;if(close(fd))ok=false;
    const int found=lstat(path,&after);
    if((!found&&(!existed||!S_ISREG(after.st_mode)||after.st_nlink>1||after.st_ino!=before.st_ino||after.st_size!=before.st_size||after.st_mtime!=before.st_mtime))||
       (found&&(existed||errno!=ENOENT)))ok=false;
    if(!ok){unlink(next);errno=EIO;return -1;}
    if(rename(next,path)){unlink(next);return -1;}
    if(preferences::sync_parent(path))return -1;
    Choice checked;char verified[256];
    if(load(checked)!=1||std::size_t(format(verified,checked))!=size||std::memcmp(bytes,verified,size)){errno=EIO;return -1;}
    return 0;
}
}
