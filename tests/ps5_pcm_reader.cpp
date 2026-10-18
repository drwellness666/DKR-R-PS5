// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5_pcm_reader.hpp"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>
static int current=-1;static unsigned opened=0,closed=0,reads=0,stats=0;
static int mode=0;static bool replace_fifo=false;
extern "C" int __real_open(const char*,int,...);
extern "C" int __real_close(int);
extern "C" ssize_t __real_read(int,void*,size_t);
extern "C" int __real_fstat(int,struct stat*);
extern "C" int __wrap_open(const char*path,int flags,...){
 assert(flags&O_NONBLOCK);assert(flags&O_NOFOLLOW);
 if(replace_fifo){assert(unlink(path)==0&&mkfifo(path,0600)==0);replace_fifo=false;}
 int fd=__real_open(path,flags);if(fd>=0){current=fd;++opened;reads=stats=0;}return fd;
}
extern "C" int __wrap_close(int fd){assert(fd==current);++closed;current=-1;return __real_close(fd);}
extern "C" ssize_t __wrap_read(int fd,void*data,size_t n){
 assert(fd==current);++reads;
 if(mode==1&&reads<=2){errno=EINTR;return -1;}
 if(mode==2&&reads>1)return 0;
 if(mode==3){errno=EIO;return -1;}
 return __real_read(fd,data,std::min<size_t>(n,3));
}
extern "C" int __wrap_fstat(int fd,struct stat*st){
 assert(fd==current);++stats;if(mode==4){errno=EIO;return -1;}
 int rc=__real_fstat(fd,st);if(rc==0&&mode==5&&stats>1)st->st_size+=2;return rc;
}
static void bytes(const std::string&p,const std::vector<std::int16_t>&data){std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char*>(data.data()),data.size()*2);assert(f.good());}
static void refused(const std::string&p){auto before=closed;assert(dkrr::ps5_speaker::read_pcm(p.c_str()).empty());assert(opened==closed&&current==-1);assert(closed>=before);}
int main(int argc,char**argv){
 assert(argc==2);std::string root=argv[1],p=root+"/clip.pcm";std::vector<std::int16_t> data(65536);for(unsigned i=0;i<data.size();++i)data[i]=static_cast<std::int16_t>(i);
 bytes(p,data);auto got=dkrr::ps5_speaker::read_pcm(p.c_str());assert(got==data&&opened==closed);
 mode=1;assert(dkrr::ps5_speaker::read_pcm(p.c_str())==data&&opened==closed);
 for(mode=2;mode<=5;++mode)refused(p);mode=0;
 for(auto n:{0U,192001U}){bytes(p,std::vector<std::int16_t>(n));refused(p);}
 {std::ofstream f(p,std::ios::binary);f.put('x');}refused(p);
 data.resize(192000);bytes(p,data);assert(dkrr::ps5_speaker::read_pcm(p.c_str())==data);
 std::string link=root+"/link";assert(symlink(p.c_str(),link.c_str())==0);refused(link);struct stat st{};assert(lstat(link.c_str(),&st)==0&&S_ISLNK(st.st_mode));
 refused(root);refused(root+"/missing");
 assert(unlink(p.c_str())==0&&mkfifo(p.c_str(),0600)==0);refused(p);assert(lstat(p.c_str(),&st)==0&&S_ISFIFO(st.st_mode));
 assert(unlink(p.c_str())==0);bytes(p,data);replace_fifo=true;refused(p);assert(lstat(p.c_str(),&st)==0&&S_ISFIFO(st.st_mode));
 puts("PASS actual PCM reader: complete int16 parity, short reads/EINTR, EOF/read/stat failures, changed size, exact maximum, rejected odd/empty/oversize/FIFO/symlink/directory; file-to-FIFO race cannot block; descriptors closed");
}
