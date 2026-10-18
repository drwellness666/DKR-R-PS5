// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5_display_options.h"
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
namespace fs=std::filesystem;
static unsigned fault=0;
extern "C" int __real_fsync(int);
extern "C" int __real_rename(const char*,const char*);
extern "C" ssize_t __real_write(int,const void*,size_t);
extern "C" ssize_t __wrap_write(int fd,const void* p,size_t n) {
    if(fault==1){errno=ENOSPC;return -1;}
    if(fault==2){const auto rc=__real_write(fd,p,n/2);(void)rc;_exit(77);}
    return __real_write(fd,p,n);
}
extern "C" int __wrap_fsync(int fd) {
    struct stat s{};assert(!fstat(fd,&s));const bool dir=S_ISDIR(s.st_mode);
    if((fault==3&&!dir)||(fault==6&&dir)){errno=EIO;return -1;}
    const int rc=__real_fsync(fd);
    if((fault==4&&!dir)||(fault==7&&dir))_exit(77);
    return rc;
}
extern "C" int __wrap_rename(const char* from,const char* to) {
    if(fault==5){errno=EIO;return -1;}
    const int rc=__real_rename(from,to);if(fault==8)_exit(77);return rc;
}
static std::string bytes(const fs::path& p){std::ifstream f(p);return {std::istreambuf_iterator<char>(f),{}};}
static void choice(unsigned res,unsigned hz,unsigned enabled,unsigned white) {
    dkrr_ps5_display_choice display{};dkrr_ps5_hdr_choice hdr{};
    assert(dkrr_ps5_read_output(&display,&hdr)==1);
    assert(display.resolution==res&&display.refresh==hz&&hdr.enabled==enabled&&hdr.paper_white==white);
}
int main() {
    const fs::path root=DKR_DISPLAY_DATA_ROOT,folder=root/"preferences",file=folder/"tv-output.txt";
    const auto legacy_display=folder/"display-output.txt",legacy_hdr=folder/"hdr-output.txt";
    fs::create_directories(folder);
    const std::string old_display="resolution=1440p\nrefresh=60\n",old_hdr="hdr=1\npaper_white=180\npeak=700\nexpansion=25\n";
    std::ofstream(legacy_display)<<old_display;std::ofstream(legacy_hdr)<<old_hdr;
    choice(1440,60,1,180);assert(!fs::exists(file)); // Reading never migrates or writes.
    assert(!dkrr_ps5_choose_output({2160,120},{0,220,900,15}));choice(2160,120,0,220);
    assert(bytes(legacy_display)==old_display&&bytes(legacy_hdr)==old_hdr);
    // The new complete record is authoritative, including for individual API calls.
    assert(!dkrr_ps5_choose_display({1080,60}));choice(1080,60,0,220);
    assert(!dkrr_ps5_choose_hdr({1,180,700,25}));choice(1080,60,1,180);
    for(unsigned injection=1;injection<=8;++injection) {
        assert(!dkrr_ps5_choose_output({1080,60},{0,200,800,35}));
        const auto child=fork();assert(child>=0);
        if(!child){fault=injection;const int rc=dkrr_ps5_choose_output({2160,120},{1,180,700,25});assert(rc==-1);_exit(0);}
        int status{};assert(waitpid(child,&status,0)==child&&WIFEXITED(status));
        assert(WEXITSTATUS(status)==((injection==2||injection==4||injection==7||injection==8)?77:0));
        if(injection<6)choice(1080,60,0,200);else choice(2160,120,1,180);
        assert(bytes(legacy_display)==old_display&&bytes(legacy_hdr)==old_hdr);
        assert(!dkrr_ps5_choose_output({1440,60},{0,210,800,20}));choice(1440,60,0,210);
    }
    // A corrupt current record must never resurrect stale legacy choices.
    std::ofstream(file)<<"unrecognized current format";
    dkrr_ps5_display_choice display{};dkrr_ps5_hdr_choice hdr{};
    assert(dkrr_ps5_read_display_choice(&display)==-1&&display.resolution==1080);
    assert(dkrr_ps5_read_hdr_choice(&hdr)==-1&&!hdr.enabled);
    assert(dkrr_ps5_choose_output({1080,60},{0,200,800,35})==-1);
    assert(bytes(file)=="unrecognized current format");fs::remove(file);
    std::ofstream(legacy_hdr)<<"invalid legacy HDR";
    assert(dkrr_ps5_choose_output({1080,60},{0,200,800,35})==-1&&!fs::exists(file));
    assert(bytes(legacy_hdr)=="invalid legacy HDR"&&bytes(legacy_display)==old_display);
    std::puts("PASS one output/HDR transaction: legacy import without mutation, complete-record authority, eight I/O/power-cut points, retry after interruption and refusal without stale fallback");
}
