/* SPDX-License-Identifier: GPL-3.0-or-later
 * Use ShadowMountPlus' documented watched manual.lst. Preserve every existing
 * line; add exactly this app source. No HTTP endpoint or app.db mutation.
 */
#include "dkrr_install.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
/* A previous attempt may have left a partial or obsolete snapshot. Never
 * resume/overwrite that shared-directory file; reserve a fresh bounded slot. */
static int write_temporary(const char *path,char temp[DKRR_PATH],const unsigned char *bytes,size_t length,unsigned mode) {
 int fd=-1;
 for(unsigned slot=0;slot<128;++slot) {
  if(snprintf(temp,DKRR_PATH,"%s.dkrr.%u.tmp",path,slot)>=DKRR_PATH)return -1;
  fd=open(temp,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,(mode_t)mode);
  if(fd>=0)break;
  if(errno!=EEXIST)return -1;
 }
 if(fd<0)return -1;
 size_t at=0;int good=1;
 while(at<length){ssize_t n=write(fd,bytes+at,length-at);if(n<0&&errno==EINTR)continue;if(n<=0){good=0;break;}at+=(size_t)n;}
 if(fchmod(fd,(mode_t)mode)<0||fsync(fd)<0)good=0;
 if(close(fd)<0)good=0;
 return good?0:-1;
}
int dkrr_register_source(const char *root,const char *app) {
 char folder[DKRR_PATH],path[DKRR_PATH],temp[DKRR_PATH];struct stat st;
 if(snprintf(folder,sizeof(folder),"%s/shadowmount",root)>=(int)sizeof(folder)||lstat(folder,&st)<0||!S_ISDIR(st.st_mode))return -1;
 if(snprintf(path,sizeof(path),"%s/manual.lst",folder)>=(int)sizeof(path))return -1;
 int state=dkrr_regular(path,1);if(state<0)return -1;
 static unsigned char old[65536];size_t length=0;unsigned mode=0644;char before[41]={0};unsigned long long size=0;
 if(state==1){
  if(dkrr_hash_file(path,before,&size)<0||size>sizeof(old))return -1;
  int fd=open(path,O_RDONLY|O_NOFOLLOW);if(fd<0)return -1;
  if(fstat(fd,&st)<0||!S_ISREG(st.st_mode)){close(fd);return -1;}mode=(unsigned)(st.st_mode&0777);
  while(length<size){ssize_t n=read(fd,old+length,(size_t)size-length);if(n<0&&errno==EINTR)continue;if(n<=0){close(fd);return -1;}length+=(size_t)n;}close(fd);
  if(memchr(old,0,length))return -1;
  size_t start=0;
  while(start<length){size_t end=start;while(end<length&&old[end]!='\n')++end;size_t a=start,b=end;while(a<b&&(old[a]==' '||old[a]=='\t'))++a;while(b>a&&(old[b-1]==' '||old[b-1]=='\t'||old[b-1]=='\r'))--b;
   if(b-a==strlen(app)&&!memcmp(old+a,app,b-a))return 0;start=end+1;
  }
 }
 size_t n=strlen(app);if(n>=DKRR_PATH||length+1+n+1>sizeof(old))return -1;
 if(length&&old[length-1]!='\n')old[length++]='\n';memcpy(old+length,app,n);length+=n;old[length++]='\n';
 if(write_temporary(path,temp,old,length,mode)<0)return -1;
 if(state==0){if(dkrr_regular(path,1)!=0)return -1;}
 else {char current[41];unsigned long long now;if(dkrr_hash_file(path,current,&now)<0||now!=size||strcmp(current,before))return -1;}
 if(rename(temp,path)<0||dkrr_sync_parent(path)<0)return -1;
 return 0;
}
