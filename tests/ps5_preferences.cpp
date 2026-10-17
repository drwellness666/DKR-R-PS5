// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5_display_options.h"
#include "ps5_hdr_options.h"
#include "ps5_revision_selection.h"
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
namespace fs=std::filesystem;
static unsigned directory_syncs=0;
static bool deny_directory_sync=false;
extern "C" int __real_fsync(int);
extern "C" int __wrap_fsync(int fd) {
    struct stat st{};assert(!fstat(fd,&st));
    if(S_ISDIR(st.st_mode)) {
        ++directory_syncs;
        if(deny_directory_sync){errno=EIO;return -1;}
    }
    return __real_fsync(fd);
}
static std::string read(const fs::path& path) {
    std::ifstream f(path,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};
}
static int save(unsigned kind) {
    if(kind==0)return dkrr_ps5_choose_display({1080,60});
    if(kind==1)return dkrr_ps5_choose_hdr({1,180,700,25});
    return dkrr_ps5_choose_revision(77);
}
int main() {
    const fs::path root=DKR_DISPLAY_DATA_ROOT,folder=root/"preferences";
    fs::create_directories(folder);fs::create_directory(root/"roms");
    std::ofstream(root/"roms/dkr.us.v77.z64").close();fs::resize_file(root/"roms/dkr.us.v77.z64",12*1024*1024);
    const auto unrelated=root/"unrelated";std::ofstream(unrelated)<<"keep unrelated data";
    const char* names[]={"display-output.txt","hdr-output.txt","boot-revision.txt"};
    for(unsigned kind=0;kind<3;++kind) {
        const auto file=folder/names[kind],legacy=fs::path(file.string()+".next");
        std::ofstream(legacy)<<"partial legacy write";
        fs::create_symlink(unrelated,file.string()+".next.0");
        std::ofstream(file.string()+".next.1")<<"partial numbered write";
        directory_syncs=0;assert(save(kind)==0&&directory_syncs==1);
        assert(read(legacy)=="partial legacy write"&&read(unrelated)=="keep unrelated data");
        assert(fs::is_symlink(file.string()+".next.0")&&read(file.string()+".next.1")=="partial numbered write");
        const auto complete=read(file);assert(!complete.empty());
        for(unsigned i=2;i<128;++i)std::ofstream(file.string()+".next."+std::to_string(i))<<"occupied";
        assert(save(kind)==-1&&read(file)==complete);
        for(unsigned i=0;i<128;++i)fs::remove(file.string()+".next."+std::to_string(i));
        // File data alone is insufficient: a failed directory sync must not
        // be reported as a durable success, even after the atomic rename.
        deny_directory_sync=true;directory_syncs=0;
        assert(save(kind)==-1&&directory_syncs==1&&read(file)==complete);
        deny_directory_sync=false;assert(save(kind)==0);
        fs::remove(file);assert(mkfifo(file.c_str(),0600)==0);
        if(kind==0){dkrr_ps5_display_choice c{};assert(dkrr_ps5_read_display_choice(&c)==-1);}
        else if(kind==1){dkrr_ps5_hdr_choice c{};assert(dkrr_ps5_read_hdr_choice(&c)==-1);}
        else assert(dkrr_ps5_initial_revision()==77);
        assert(save(kind)==-1);fs::remove(file);
    }
    std::puts("PASS actual display/HDR/revision preferences: legacy and numbered interrupted writes, symlink preservation, bounded slot exhaustion, directory-sync failure and nonblocking FIFO refusal");
}
