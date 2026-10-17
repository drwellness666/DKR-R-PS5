// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5_hdr_policy.hpp"
#include <atomic>
#include <chrono>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <fcntl.h>
#include <mutex>
#include <sys/stat.h>
#include <unistd.h>
#ifndef DKR_DISPLAY_DATA_ROOT
#define DKR_DISPLAY_DATA_ROOT "/data/dkrr"
#endif
namespace {
constexpr char directory[]=DKR_DISPLAY_DATA_ROOT "/preferences";
constexpr char path[]=DKR_DISPLAY_DATA_ROOT "/preferences/hdr-output.txt";
constexpr char next[]=DKR_DISPLAY_DATA_ROOT "/preferences/hdr-output.txt.next";
std::mutex mutex;
// Render snapshots never wait on preference reads, writes or fsync. One word
// publishes a complete validated choice, so readers cannot see mixed fields.
static_assert(std::atomic<std::uint32_t>::is_always_lock_free);
constexpr std::uint32_t pack(dkrr_ps5_hdr_choice c){return c.enabled|(c.paper_white<<1)|(c.peak<<10)|(c.expansion<<21);}
constexpr dkrr_ps5_hdr_choice unpack(std::uint32_t c){return {c&1,(c>>1)&511,(c>>10)&2047,(c>>21)&127};}
std::atomic<std::uint32_t> calibration{pack({0,200,800,35})};
std::atomic<unsigned> requested{0},output{0},reason{0},pattern{0};
std::atomic<std::int64_t> pattern_until{0};
std::int64_t now_ms(){return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();}
bool safe_directory(){struct stat s{};return !lstat(DKR_DISPLAY_DATA_ROOT,&s)&&S_ISDIR(s.st_mode)&&!lstat(directory,&s)&&S_ISDIR(s.st_mode);}
int format(char *p,dkrr_ps5_hdr_choice c){return std::snprintf(p,128,"hdr=%u\npaper_white=%u\npeak=%u\nexpansion=%u\n",c.enabled,c.paper_white,c.peak,c.expansion);}
int read_choice(dkrr_ps5_hdr_choice *c){
    if(!c){errno=EINVAL;return -1;}*c={0,200,800,35};
    if(!safe_directory()){errno=ENOTDIR;return -1;}
    int fd=open(path,O_RDONLY|O_NOFOLLOW|O_NONBLOCK);if(fd<0)return errno==ENOENT?0:-1;
    struct stat s{};char text[128]{};size_t n=0;
    bool ok=!fstat(fd,&s)&&S_ISREG(s.st_mode)&&s.st_nlink<=1&&s.st_size>0&&s.st_size<127;
    while(ok&&n<127){ssize_t b=read(fd,text+n,127-n);if(b<0&&errno==EINTR)continue;if(b<0){ok=false;break;}if(!b)break;n+=size_t(b);}
    close(fd);dkrr_ps5_hdr_choice candidate{};char canonical[128];int end=0;
    if(ok&&std::sscanf(text,"hdr=%u\npaper_white=%u\npeak=%u\nexpansion=%u\n%n",&candidate.enabled,&candidate.paper_white,&candidate.peak,&candidate.expansion,&end)==4&&
       dkrr::hdr::valid(candidate)&&size_t(format(canonical,candidate))==n&&!std::memcmp(canonical,text,n)){
        *c=candidate;return 1;
    }errno=EINVAL;return -1;
}
}
extern "C" int dkrr_ps5_read_hdr_choice(dkrr_ps5_hdr_choice *c){std::scoped_lock lock(mutex);return read_choice(c);}
extern "C" void dkrr_ps5_initialize_hdr(){std::scoped_lock lock(mutex);dkrr_ps5_hdr_choice c{};read_choice(&c);calibration.store(pack(c),std::memory_order_release);requested=c.enabled;output=reason=pattern=0;}
extern "C" unsigned dkrr_ps5_hdr_requested(){return requested.load();}
extern "C" dkrr_ps5_hdr_choice dkrr_ps5_hdr_calibration(){return unpack(calibration.load(std::memory_order_acquire));}
extern "C" struct dkrr_ps5_hdr_status dkrr_ps5_hdr_status(){return {output.load(),reason.load()};}
extern "C" void dkrr_ps5_hdr_report(unsigned o,unsigned r){reason=r;output=o;}
extern "C" void dkrr_ps5_hdr_preview(dkrr_ps5_hdr_choice c){if(dkrr::hdr::valid(c))calibration.store(pack(c),std::memory_order_release);}
extern "C" void dkrr_ps5_hdr_calibration_pattern(unsigned p){pattern_until=p?now_ms()+300:0;pattern=p!=0;}
extern "C" unsigned dkrr_ps5_hdr_pattern(){return pattern.load() && now_ms()<pattern_until.load();}
extern "C" int dkrr_ps5_choose_hdr(dkrr_ps5_hdr_choice c){
    std::scoped_lock lock(mutex);if(!dkrr::hdr::valid(c)){errno=EINVAL;return -1;}
    if(!safe_directory()){errno=ENOTDIR;return -1;}
    struct stat old{},now{};const bool existed=!lstat(path,&old);
    if((existed&&!S_ISREG(old.st_mode))||(!existed&&errno!=ENOENT)){errno=EINVAL;return -1;}
    dkrr_ps5_hdr_choice prior{};if(existed&&read_choice(&prior)!=1){errno=EINVAL;return -1;}
    int fd=open(next,O_CREAT|O_EXCL|O_WRONLY|O_NOFOLLOW,0644);if(fd<0)return -1;
    char text[128];const int length=format(text,c);size_t at=0;bool ok=true;
    while(at<size_t(length)){const ssize_t n=write(fd,text+at,size_t(length)-at);if(n<0&&errno==EINTR)continue;if(n<=0){ok=false;break;}at+=size_t(n);}
    if(fchmod(fd,0644)||fsync(fd))ok=false;if(close(fd))ok=false;
    const int found=lstat(path,&now);
    if((!found&&(!existed||!S_ISREG(now.st_mode)||now.st_ino!=old.st_ino||now.st_mtime!=old.st_mtime||now.st_size!=old.st_size))||
       (found&&(existed||errno!=ENOENT)))ok=false;
    if(!ok||rename(next,path)){unlink(next);errno=EIO;return -1;}
    fd=open(directory,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);if(fd<0)return -1;
    const int synced=fsync(fd);close(fd);if(synced)return -1;
    calibration.store(pack(c),std::memory_order_release);return 0;
}
