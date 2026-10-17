/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "dkrr_install.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#ifndef DKRR_TEST_ROM_HASH77
#define DKRR_TEST_ROM_HASH77 "0cb115d8716dbbc2922fda38e533b9fe63bb9670"
#define DKRR_TEST_ROM_HASH80 "6d96743d46f8c0cd0edb0ec5600b003c89b93755"
#endif
static const unsigned revisions[2]={77,80};
int dkrr_rom_identify(const char *path,unsigned char **bytes,unsigned *revision) {
 *bytes=NULL;*revision=0;if(dkrr_regular(path,0)!=1)return -1;
 int fd=open(path,O_RDONLY|O_NOFOLLOW);if(fd<0)return -1;struct stat st;
 if(fstat(fd,&st)<0||st.st_size!=DKRR_ROM_SIZE){close(fd);return -1;}
 // Reject unrelated same-size USB files before allocating/reading 12 MiB.
 unsigned char header[4];size_t got=0;
 while(got<sizeof(header)){ssize_t n=read(fd,header+got,sizeof(header)-got);if(n<0&&errno==EINTR)continue;if(n<=0){close(fd);return -1;}got+=(size_t)n;}
 if(memcmp(header,"\x80\x37\x12\x40",4)&&memcmp(header,"\x37\x80\x40\x12",4)&&memcmp(header,"\x40\x12\x37\x80",4)){close(fd);return -1;}
 unsigned char *rom=malloc(DKRR_ROM_SIZE);if(!rom){close(fd);return -1;}
 memcpy(rom,header,sizeof(header));size_t at=sizeof(header);int ok=1;while(at<DKRR_ROM_SIZE){ssize_t n=read(fd,rom+at,DKRR_ROM_SIZE-at);if(n<0&&errno==EINTR)continue;if(n<=0){ok=0;break;}at+=(size_t)n;}
 unsigned char extra;if(read(fd,&extra,1)!=0)ok=0;close(fd);if(!ok){free(rom);return -1;}
 if(!memcmp(rom,"\x37\x80\x40\x12",4))for(size_t i=0;i<DKRR_ROM_SIZE;i+=2){unsigned char b=rom[i];rom[i]=rom[i+1];rom[i+1]=b;}
 else if(!memcmp(rom,"\x40\x12\x37\x80",4))for(size_t i=0;i<DKRR_ROM_SIZE;i+=4){unsigned char a=rom[i],b=rom[i+1];rom[i]=rom[i+3];rom[i+1]=rom[i+2];rom[i+2]=b;rom[i+3]=a;}
 else if(memcmp(rom,"\x80\x37\x12\x40",4)){free(rom);return -1;}
 unsigned char digest[20];char hash[41];dkrr_sha1_bytes(rom,DKRR_ROM_SIZE,digest);dkrr_hex(digest,hash);
 if(!strcmp(hash,DKRR_TEST_ROM_HASH77))*revision=77;else if(!strcmp(hash,DKRR_TEST_ROM_HASH80))*revision=80;
 if(!*revision){free(rom);return -1;}*bytes=rom;return 0;
}
static int consider(const char *p,struct dkrr_rom out[2]) {
 struct stat st;if(lstat(p,&st)<0||!S_ISREG(st.st_mode)||st.st_size!=DKRR_ROM_SIZE)return 0;
 unsigned char *rom;unsigned rev;if(dkrr_rom_identify(p,&rom,&rev)<0)return 0;free(rom);
 unsigned i=rev==80?1:0;if(!out[i].revision){if(strlen(p)>=sizeof(out[i].path))return -1;strcpy(out[i].path,p);out[i].revision=rev;}
 return 0;
}
static int scan(const char *folder,struct dkrr_rom out[2]) {
 struct stat st;if(lstat(folder,&st)<0)return errno==ENOENT?0:-1;if(!S_ISDIR(st.st_mode))return -1;
 DIR *dir=opendir(folder);if(!dir)return -1;struct dirent *e;unsigned count=0;int rc=0;
 // A bounded search miss on one crowded drive must not prevent the DKRR
 // folder on a later drive from being checked.
 while((e=readdir(dir))){if(!strcmp(e->d_name,".")||!strcmp(e->d_name,".."))continue;if(++count>256)break;
  char path[DKRR_PATH];if(snprintf(path,sizeof(path),"%s/%s",folder,e->d_name)>=(int)sizeof(path)){rc=-1;break;}
  if(consider(path,out)<0){rc=-1;break;}if(out[0].revision&&out[1].revision)break;
 }closedir(dir);return rc;
}
int dkrr_rom_find(const char *usb,const char *owned,struct dkrr_rom out[2]) {
 memset(out,0,2*sizeof(*out));
 for(unsigned i=0;i<2;++i){char path[DKRR_PATH];if(snprintf(path,sizeof(path),"%s/dkr.us.v%u.z64",owned,revisions[i])>=(int)sizeof(path))return -1;
  struct stat st;if(lstat(path,&st)==0){if(dkrr_regular(path,0)!=1||consider(path,out)<0||!out[i].revision)return -1;}else if(errno!=ENOENT)return -1;
 }
 for(unsigned i=0;i<8&&(!out[0].revision||!out[1].revision);++i){char path[DKRR_PATH];
  if(snprintf(path,sizeof(path),"%s/usb%u/DKRR",usb,i)>=(int)sizeof(path)||scan(path,out)<0)return -1;
  if(out[0].revision&&out[1].revision)break;
  if(snprintf(path,sizeof(path),"%s/usb%u",usb,i)>=(int)sizeof(path)||scan(path,out)<0)return -1;
 }return out[0].revision==77&&out[1].revision==80?0:-1;
}
int dkrr_rom_install(const char *owned,const struct dkrr_rom inputs[2]) {
 for(unsigned i=0;i<2;++i){char path[DKRR_PATH];if(snprintf(path,sizeof(path),"%s/dkr.us.v%u.z64",owned,revisions[i])>=(int)sizeof(path))return -1;
  int state=dkrr_regular(path,1);if(state<0)return -1;
  if(state==1){unsigned char *existing;unsigned r;int rc=dkrr_rom_identify(path,&existing,&r);free(existing);if(rc<0||r!=revisions[i])return -1;continue;}
  unsigned char *rom;unsigned rev;if(dkrr_rom_identify(inputs[i].path,&rom,&rev)<0)return -1;
  if(rev!=revisions[i]){free(rom);return -1;}
  char temp[DKRR_PATH];if(snprintf(temp,sizeof(temp),"%s.import",path)>=(int)sizeof(temp)){free(rom);return -1;}
  int rc=dkrr_write_new(temp,rom,DKRR_ROM_SIZE,0644);free(rom);if(rc<0)return -1;
  unsigned char *check;unsigned r;rc=dkrr_rom_identify(temp,&check,&r);free(check);if(rc<0||r!=rev)return -1;
  if(dkrr_regular(path,1)!=0||rename(temp,path)<0||dkrr_sync_parent(path)<0)return -1;
 }return 0;
}
