// SPDX-License-Identifier: GPL-3.0-or-later
// Own ROM-version preference only. No system services, saves or migrations.
#include "ps5_revision_selection.h"
#include "ps5_preference_io.hpp"
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <atomic>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#ifndef DKR_PAYLOAD_V77_ENABLED
#define DKR_PAYLOAD_V77_ENABLED 1
#endif
#ifndef DKR_PAYLOAD_V80_ENABLED
#define DKR_PAYLOAD_V80_ENABLED 1
#endif
#ifndef DKR_REVISION_DATA_ROOT
#define DKR_REVISION_DATA_ROOT "/data/dkrr"
#endif
namespace {
constexpr const char* choice_directory=DKR_REVISION_DATA_ROOT "/preferences";
constexpr const char* preference=DKR_REVISION_DATA_ROOT "/preferences/boot-revision.txt";
constexpr const char* legacy_preference=DKR_REVISION_DATA_ROOT "/boot-revision.txt";
constexpr const char* temporary_prefix=DKR_REVISION_DATA_ROOT "/preferences/boot-revision.txt.next";
constexpr const char* rom77=DKR_REVISION_DATA_ROOT "/roms/dkr.us.v77.z64";
constexpr const char* rom80=DKR_REVISION_DATA_ROOT "/roms/dkr.us.v80.z64";
constexpr unsigned compiled=(DKR_PAYLOAD_V77_ENABLED?1U:0U)|(DKR_PAYLOAD_V80_ENABLED?2U:0U);
std::atomic<unsigned> active{0};
unsigned bit(unsigned revision){return revision==77?1U:revision==80?2U:0U;}
bool ordinary_rom(const char* path){struct stat st={};return lstat(path,&st)==0&&S_ISREG(st.st_mode)&&st.st_size==12*1024*1024;}
unsigned available(){return (ordinary_rom(rom77)?1U:0U)|(ordinary_rom(rom80)?2U:0U);}
// Exact legacy revision IDs remain accepted; new choices are human-readable.
unsigned read_preference(const char* path=preference){
 int fd=open(path,O_RDONLY|O_NOFOLLOW|O_NONBLOCK);if(fd<0)return 0;
 struct stat st={};char value[8]={};size_t size=0;
 bool ok=fstat(fd,&st)==0&&S_ISREG(st.st_mode)&&st.st_size>0&&st.st_size<8;
 while(ok&&size<sizeof(value)){ssize_t n=read(fd,value+size,sizeof(value)-size);if(n<0&&errno==EINTR)continue;if(n<0){ok=false;break;}if(!n)break;size+=static_cast<size_t>(n);}
 close(fd);if(!ok||size>=sizeof(value))return 0;
 if(size&&value[size-1]=='\n')--size;
 if(size&&value[size-1]=='\r')--size;
 if((size==2&&!std::memcmp(value,"77",2))||(size==3&&!std::memcmp(value,"1.0",3)))return 77;
 if((size==2&&!std::memcmp(value,"80",2))||(size==3&&!std::memcmp(value,"1.1",3)))return 80;
 return 0;
}
}
extern "C" unsigned dkrr_ps5_initial_revision(){
 // A single-ROM build is unaffected by a choice left by a dual build.
 if(compiled==1)return 77;
 if(compiled==2)return 80;
 unsigned selected=read_preference();
 if(!selected){struct stat st={};if(lstat(preference,&st)!=0&&errno==ENOENT)selected=read_preference(legacy_preference);}
 if(selected&&compiled&bit(selected))return selected;
 const unsigned ready=available()&compiled;return ready==2?80:77;
}
extern "C" void dkrr_ps5_set_active_revision(unsigned revision){active.store(revision,std::memory_order_relaxed);}
extern "C" unsigned dkrr_ps5_active_revision(){return active.load(std::memory_order_relaxed);}
extern "C" struct dkrr_ps5_revision_state dkrr_ps5_revision_state(){
 const unsigned current=active.load(std::memory_order_relaxed);
 unsigned preferred=read_preference();if(!preferred||!(compiled&bit(preferred)))preferred=dkrr_ps5_initial_revision();
 return {current,preferred,compiled,available()};
}
extern "C" int dkrr_ps5_choose_revision(unsigned revision){
 const unsigned requested=bit(revision);
 if(!requested||!(compiled&requested)){errno=EINVAL;return -1;}
 if(!(available()&requested)){errno=ENOENT;return -1;}
 struct stat directory={};if(lstat(choice_directory,&directory)||!S_ISDIR(directory.st_mode)){errno=ENOTDIR;return -1;}
 struct stat old={};const bool existed=lstat(preference,&old)==0;
 if(existed){if(!S_ISREG(old.st_mode)||!read_preference()){errno=EINVAL;return -1;}}
 else if(errno!=ENOENT)return -1;
 char temporary[512];int fd=dkrr::preferences::reserve(temporary_prefix,temporary);if(fd<0)return -1;
 const char* value=revision==77?"1.0\n":"1.1\n";size_t position=0;bool ok=true;
 while(position<4){ssize_t n=write(fd,value+position,4-position);if(n<0&&errno==EINTR)continue;if(n<=0){ok=false;break;}position+=static_cast<size_t>(n);}
 if(fchmod(fd,0644)||fsync(fd))ok=false;
 if(close(fd))ok=false;
 // Refuse a substituted or unrecognised target rather than clobber it.
 struct stat current={};int checked=lstat(preference,&current);
 if(checked==0){if(!existed||!S_ISREG(current.st_mode)||current.st_ino!=old.st_ino||current.st_size!=old.st_size||current.st_mtime!=old.st_mtime||!read_preference())ok=false;}
 else if(errno!=ENOENT||existed)ok=false;
 if(!ok){unlink(temporary);errno=EIO;return -1;}
 if(rename(temporary,preference)){unlink(temporary);return -1;}
 if(dkrr::preferences::sync_parent(preference))return -1;
 if(read_preference()!=revision){errno=EIO;return -1;}
 return 0;
}
