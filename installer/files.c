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
void dkrr_hex(const unsigned char d[20],char out[41]) {static const char h[]="0123456789abcdef";for(int i=0;i<20;++i){out[i*2]=h[d[i]>>4];out[i*2+1]=h[d[i]&15];}out[40]=0;}
static int parents(const char *path) {
 char p[DKRR_PATH];size_t n=strlen(path);if(path[0]!='/'||n>=sizeof(p)||strstr(path,"/../")||strstr(path,"/./"))return -1;
 memcpy(p,path,n+1);for(size_t i=1;i<n;++i)if(p[i]=='/'){struct stat st;p[i]=0;int r=lstat(p,&st);p[i]='/';if(r<0||!S_ISDIR(st.st_mode))return -1;}return 0;
}
int dkrr_regular(const char *p,int missing) {
 if(parents(p)<0)return -1;struct stat st;if(lstat(p,&st)<0)return missing&&errno==ENOENT?0:-1;
 return S_ISREG(st.st_mode)&&st.st_nlink<=1?1:-1;
}
int dkrr_mkdir(const char *p,unsigned mode) {
 if(parents(p)<0)return -1;
 if(mkdir(p,(mode_t)mode)==0){if(chmod(p,(mode_t)mode)<0)return -1;return dkrr_sync_parent(p);}
 struct stat st;return errno==EEXIST&&lstat(p,&st)==0&&S_ISDIR(st.st_mode)?0:-1;
}
int dkrr_sync_parent(const char *path) {
 char p[DKRR_PATH];if(strlen(path)>=sizeof(p))return -1;strcpy(p,path);char *s=strrchr(p,'/');if(!s)return -1;if(s==p)s[1]=0;else *s=0;
 int fd=open(p,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);if(fd<0)return -1;int r=fsync(fd);if(close(fd)<0)r=-1;return r;
}
/* The PS5 checks write access to the moved directory when updating its '..'
 * entry. A payload with euid 0 cannot move an app-owned 0755 directory across
 * parents merely because it can write both parents. Keep the verified tree's
 * modes private: claim only its top directory through an open descriptor for
 * the rename, then restore its owner on that same descriptor on either result.
 * Callers have already verified the complete app manifest and keep the journal
 * until both parent directory entries are durable. No content is copied here.
 */
int dkrr_move_app_directory(const char *from,const char *to) {
 if(parents(from)<0||parents(to)<0)return -1;
 struct stat st;
 if(lstat(to,&st)==0){errno=EEXIST;return -1;}if(errno!=ENOENT)return -1;
 int fd=open(from,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);if(fd<0)return -1;
 if(fstat(fd,&st)<0||!S_ISDIR(st.st_mode)||(st.st_mode&07000)){close(fd);errno=EINVAL;return -1;}
 const uid_t installer_uid=geteuid();const int claimed=st.st_uid!=installer_uid;
 if(claimed&&fchown(fd,installer_uid,(gid_t)-1)<0){int e=errno;close(fd);errno=e;return -1;}
 int rc=rename(from,to);int saved=errno;
 if(claimed&&fchown(fd,st.st_uid,(gid_t)-1)<0){
  int e=errno;printf("DKRR INSTALL: directory owner restore failed after rename=%d: %s -> %s: errno=%d (%s)\n",rc,from,to,e,strerror(e));close(fd);errno=e;return -1;
 }
 if(fsync(fd)<0&&rc==0){rc=-1;saved=errno;}
 if(close(fd)<0&&rc==0){rc=-1;saved=errno;}
 if(rc<0)printf("DKRR INSTALL: directory move failed: %s -> %s: errno=%d (%s)\n",from,to,saved,strerror(saved));
 errno=saved;return rc;
}
int dkrr_write_new(const char *p,const void *bytes,size_t size,unsigned mode) {
 if(parents(p)<0)return -1;
 const unsigned char *data=bytes;size_t at=0;int fd=-1;
 int state=dkrr_regular(p,1);if(state<0)return -1;
 if(state==1){
  fd=open(p,O_RDONLY|O_NOFOLLOW);struct stat st;unsigned char buf[8192];
  if(fd<0)return -1;
  if(fstat(fd,&st)<0||!S_ISREG(st.st_mode)||st.st_nlink>1||st.st_size<0||(uint64_t)st.st_size>size){close(fd);return -1;}
  while(at<(size_t)st.st_size){size_t n=(size_t)st.st_size-at;if(n>sizeof(buf))n=sizeof(buf);ssize_t got=read(fd,buf,n);if(got<0&&errno==EINTR)continue;if(got<=0||memcmp(buf,data+at,(size_t)got)){close(fd);return -1;}at+=(size_t)got;}
  if(at==size){int rc=fchmod(fd,(mode_t)mode);if(fsync(fd)<0)rc=-1;if(close(fd)<0)rc=-1;return rc;}close(fd);
  // Resume only an exact prefix in an installer-owned staging/import path.
  fd=open(p,O_WRONLY|O_APPEND|O_NOFOLLOW);if(fd<0)return -1;
  if(fstat(fd,&st)<0||!S_ISREG(st.st_mode)||st.st_nlink>1||(uint64_t)st.st_size!=at){close(fd);return -1;}
 }else {fd=open(p,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,(mode_t)mode);if(fd<0)return -1;}
 int ok=1;
 while(at<size){ssize_t n=write(fd,data+at,size-at);if(n<0&&errno==EINTR)continue;if(n<=0){ok=0;break;}at+=(size_t)n;}
 if(fchmod(fd,(mode_t)mode)<0||fsync(fd)<0)ok=0;if(close(fd)<0)ok=0;
 // Preserve an interrupted exact prefix; the identical package can resume it.
 if(!ok)return -1;return dkrr_sync_parent(p);
}
int dkrr_hash_file(const char *p,char out[41],unsigned long long *size) {
 if(dkrr_regular(p,0)!=1)return -1;int fd=open(p,O_RDONLY|O_NOFOLLOW);if(fd<0)return -1;struct stat st;unsigned char digest[20];
 int ok=fstat(fd,&st)==0&&S_ISREG(st.st_mode)&&st.st_nlink<=1&&dkrr_sha1_fd(fd,digest)==0;
 if(close(fd)<0)ok=0;if(!ok)return -1;dkrr_hex(digest,out);*size=(unsigned long long)st.st_size;return 0;
}
static int relative(const char *p) {
 if(!p[0]||p[0]=='/'||strlen(p)>=160||strstr(p,"..")||strstr(p,"//"))return 0;
 for(const unsigned char *s=(const unsigned char*)p;*s;++s)if(*s<=32||*s>126||*s=='\\'||*s==':')return 0;return 1;
}
static int digest_ok(const char *p) {if(strlen(p)!=40)return 0;for(int i=0;i<40;++i)if(!((p[i]>='0'&&p[i]<='9')||(p[i]>='a'&&p[i]<='f')))return 0;return 1;}
int dkrr_manifest_read(const char *p,struct dkrr_manifest *out) {
 if(dkrr_regular(p,0)!=1)return -1;FILE *f=fopen(p,"rb");if(!f)return -1;
 char line[320];memset(out,0,sizeof(*out));int ok=fgets(line,sizeof(line),f)&&(!strcmp(line,"DKRR-INSTALL-1\n")||!strcmp(line,"DKRR-INSTALL-2\n"));
 int version2=ok&&!strcmp(line,"DKRR-INSTALL-2\n");
 if(ok){ok=fgets(line,sizeof(line),f)!=NULL;if(ok){size_t n=strlen(line);ok=n>1&&n<sizeof(out->build)&&line[n-1]=='\n';if(ok){line[n-1]=0;for(size_t i=0;i<n-1;++i)if(!((line[i]>='a'&&line[i]<='z')||(line[i]>='0'&&line[i]<='9')||line[i]=='-'))ok=0;if(ok)strcpy(out->build,line);}}}
 if(ok&&version2){
  ok=fgets(line,sizeof(line),f)!=NULL;
  if(ok){char *end;unsigned long n=strtoul(line,&end,10);ok=end!=line&&end[0]=='\n'&&!end[1]&&n>0&&n<=2147483647UL;if(ok)out->sequence=(unsigned)n;}
 }
 int end=0;while(ok&&fgets(line,sizeof(line),f)){
  if(!strcmp(line,"END\n")){end=1;break;}if(out->count==DKRR_MAX_FILES){ok=0;break;}
  struct dkrr_file *r=&out->files[out->count];char extra;
  if(sscanf(line,"%40s %llu %o %159s %c",r->sha1,&r->size,&r->mode,r->path,&extra)!=4||!digest_ok(r->sha1)||!relative(r->path)||r->size>128ULL*1024*1024||(r->mode!=0644&&r->mode!=0755)){ok=0;break;}
  for(size_t i=0;i<out->count;++i)if(!strcmp(r->path,out->files[i].path))ok=0;
  ++out->count;
 }
 if(!end||!out->count||fgetc(f)!=EOF||ferror(f))ok=0;fclose(f);return ok?0:-1;
}
int dkrr_manifest_write(const char *p,const struct dkrr_manifest *m) {
 if(!m->count||m->count>DKRR_MAX_FILES)return -1;size_t cap=80+DKRR_MAX_FILES*240;char *data=malloc(cap);if(!data)return -1;
 int n=m->sequence?snprintf(data,cap,"DKRR-INSTALL-2\n%s\n%u\n",m->build,m->sequence):snprintf(data,cap,"DKRR-INSTALL-1\n%s\n",m->build);size_t at=n>0?(size_t)n:cap;
 for(size_t i=0;i<m->count&&at<cap;++i){const struct dkrr_file *r=&m->files[i];if(!relative(r->path)||!digest_ok(r->sha1)){free(data);return -1;}n=snprintf(data+at,cap-at,"%s %llu %o %s\n",r->sha1,r->size,r->mode,r->path);if(n<0||(size_t)n>=cap-at){at=cap;break;}at+=(size_t)n;}
 if(at+4>=cap){free(data);return -1;}memcpy(data+at,"END\n",4);at+=4;char temp[DKRR_PATH];if(snprintf(temp,sizeof(temp),"%s.tmp",p)>=(int)sizeof(temp)){free(data);return -1;}
 int rc=dkrr_write_new(temp,data,at,0644);free(data);if(rc<0)return -1;
 int existing=dkrr_regular(p,1);if(existing!=0)return -1;
 if(rename(temp,p)<0)return -1;return dkrr_sync_parent(p);
}
int dkrr_manifest_add(struct dkrr_manifest *m,const char *root,const char *name,unsigned mode) {
 if(m->count==DKRR_MAX_FILES||!relative(name))return -1;char path[DKRR_PATH];if(snprintf(path,sizeof(path),"%s/%s",root,name)>=(int)sizeof(path))return -1;
 struct dkrr_file *r=&m->files[m->count];if(dkrr_hash_file(path,r->sha1,&r->size)<0)return -1;strcpy(r->path,name);r->mode=mode;++m->count;return 0;
}
static int walk(const char *root,const char *rel,const struct dkrr_manifest *m,unsigned depth) {
 if(depth>8)return -1;char path[DKRR_PATH];if(snprintf(path,sizeof(path),"%s%s%s",root,rel[0]?"/":"",rel)>=(int)sizeof(path))return -1;
 struct stat info;if(lstat(path,&info)<0||!S_ISDIR(info.st_mode))return -1;DIR *dir=opendir(path);if(!dir)return -1;
 struct dirent *e;int ok=1;unsigned count=0;
 while((e=readdir(dir))){if(!strcmp(e->d_name,".")||!strcmp(e->d_name,".."))continue;if(++count>256){ok=0;break;}
  char name[160],full[DKRR_PATH];if(snprintf(name,sizeof(name),"%s%s%s",rel,rel[0]?"/":"",e->d_name)>=(int)sizeof(name)||snprintf(full,sizeof(full),"%s/%s",root,name)>=(int)sizeof(full)){ok=0;break;}
  if(lstat(full,&info)<0){ok=0;break;}if(S_ISDIR(info.st_mode)){if(walk(root,name,m,depth+1)<0){ok=0;break;}continue;}
  if(!S_ISREG(info.st_mode)){ok=0;break;}if(!strcmp(name,"dkrr-install.manifest")||!strcmp(name,".dkrr-stage-id"))continue;
  size_t i=0;for(;i<m->count;++i)if(!strcmp(name,m->files[i].path))break;if(i==m->count){ok=0;break;}
 }closedir(dir);return ok?0:-1;
}
int dkrr_manifest_check(const char *root,const struct dkrr_manifest *m,int reject_unknown) {
 if(!m->count||m->count>DKRR_MAX_FILES)return -1;
 for(size_t i=0;i<m->count;++i){char p[DKRR_PATH],hash[41];unsigned long long n;
  if(!relative(m->files[i].path)||snprintf(p,sizeof(p),"%s/%s",root,m->files[i].path)>=(int)sizeof(p)||dkrr_hash_file(p,hash,&n)<0||n!=m->files[i].size||strcmp(hash,m->files[i].sha1))return -1;
 }return reject_unknown?walk(root,"",m,0):0;
}
