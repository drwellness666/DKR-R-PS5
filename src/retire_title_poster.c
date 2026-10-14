/* SPDX-License-Identifier: GPL-3.0-or-later
 * File-only retirement of exact recorded launch artwork. No IPC, application
 * database, mount, game executable, profile, ROM or save operations.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "native_poster_retirement.h"
#ifndef DKR_RESTORE_POSTER
#define DKR_RESTORE_POSTER 0
#endif
int dkrr_sha1_fd(int fd,unsigned char digest[20]);
static int ordinary_parents(const char *path){
 char prefix[256];size_t n=strlen(path);if(path[0]!='/'||n>=sizeof(prefix))return 0;
 memcpy(prefix,path,n+1);
 for(size_t i=1;i<n;++i){
  if(prefix[i]=='/'){
   struct stat st;prefix[i]=0;int good=lstat(prefix,&st)==0&&S_ISDIR(st.st_mode);
   prefix[i]='/';if(!good)return 0;
  }
 }
 return 1;
}
static int matches(const struct poster_file *file,const char *path){
 if(!ordinary_parents(path))return -1;
 int fd=open(path,O_RDONLY|O_NOFOLLOW);if(fd<0)return errno==ENOENT?0:-1;
 struct stat st;unsigned char digest[20];int good=fstat(fd,&st)==0&&S_ISREG(st.st_mode)
  &&(uint64_t)st.st_size==file->size&&dkrr_sha1_fd(fd,digest)==0;close(fd);
 const char hex[]="0123456789abcdef";
 if(good)for(unsigned i=0;i<20;++i)
  if(file->sha1[i*2]!=hex[digest[i]>>4]||file->sha1[i*2+1]!=hex[digest[i]&15])good=0;
 return good?1:-1;
}
static int poster_retirement_preflight(void){
 for(size_t i=0;i<sizeof(poster_guards)/sizeof(poster_guards[0]);++i)
  if(matches(&poster_guards[i],poster_guards[i].path)!=1){puts("REFUSED: own identity/art guard differs; nothing changed");return 1;}
 for(size_t i=0;i<sizeof(poster_files)/sizeof(poster_files[0]);++i){
  const struct poster_file*f=&poster_files[i];int old=matches(f,f->path),backup=matches(f,f->backup);
  if(old<0||backup<0||(old==1&&backup!=0)||(DKR_RESTORE_POSTER&&old==0&&backup!=1)){
   puts("REFUSED: unknown poster, missing restore backup or backup collision; nothing changed");return 2;
  }
 }
 return 0;
}
int main(void){
 setvbuf(stdout,NULL,_IOLBF,0);
 puts(DKR_RESTORE_POSTER?"DKR-R: restore exact previous launch-art backups":"DKR-R: retire exact recorded own launch posters; retain icon/home-menu art");
 if(poster_retirement_preflight()!=0)return 1;
 for(size_t i=0;i<sizeof(poster_files)/sizeof(poster_files[0]);++i){
  const struct poster_file*f=&poster_files[i];
  const char*source=DKR_RESTORE_POSTER?f->backup:f->path;
  const char*destination=DKR_RESTORE_POSTER?f->path:f->backup;
  if(matches(f,f->path)==(DKR_RESTORE_POSTER?1:0))continue;
  if(matches(f,source)!=1||matches(f,destination)!=0){puts("REFUSED: changed before artwork move");return 3;}
  if(rename(source,destination)!=0){perror("move known poster");return 4;}
  if(matches(f,source)!=0||matches(f,destination)!=1){puts("FAIL: final artwork verification");return 5;}
  printf("Verified known artwork move: %s\n",destination);
 }
 puts(DKR_RESTORE_POSTER?"PASS: recorded launch-art backups restored":"PASS: launch-art files absent; exact backups retained. Shell refresh still required.");return 0;
}
