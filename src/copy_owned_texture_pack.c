/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copy one exact existing own texture pack between the two own US profiles.
 * No pack/ROM bytes embedded, no application launch or system operations.
 */
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#ifndef DKR_TEXTURE_DATA_ROOT
#define DKR_TEXTURE_DATA_ROOT "/data/dkrr"
#endif
#ifndef DKR_TEXTURE_UID
#define DKR_TEXTURE_UID 1
#endif
#if !defined(DKR_PACK_NAME) || !defined(DKR_PACK_SHA1) || !defined(DKR_PACK_BYTES)
#error "Build with one explicitly chosen own pack identity"
#endif
extern int dkrr_sha1_fd(int,unsigned char[20]);
static int matches(int fd){
 struct stat st;unsigned char digest[20];char hex[41];
 if(fstat(fd,&st)||!S_ISREG(st.st_mode)||(unsigned long long)st.st_size!=DKR_PACK_BYTES||lseek(fd,0,SEEK_SET)<0||dkrr_sha1_fd(fd,digest))return 0;
 for(unsigned i=0;i<20;i++)snprintf(hex+2*i,3,"%02x",digest[i]);
 return !strcmp(hex,DKR_PACK_SHA1);
}
static int directory(const char *path){
 char prefix[256];size_t n=strlen(path);if(n>=sizeof(prefix))return -1;
 memcpy(prefix,path,n+1);struct stat st;
 for(size_t i=1;i<n;i++)if(prefix[i]=='/'){prefix[i]=0;int rc=lstat(prefix,&st);prefix[i]='/';if(rc||!S_ISDIR(st.st_mode))return -1;}
 return open(path,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);
}
int main(void){
 setvbuf(stdout,0,_IOLBF,0);setvbuf(stderr,0,_IOLBF,0);
 if(geteuid()!=DKR_TEXTURE_UID&&setuid(DKR_TEXTURE_UID)){perror("DKRR PACK STOP: own reader identity");return 1;}
 if(geteuid()!=DKR_TEXTURE_UID){puts("DKRR PACK STOP: own identity mismatch");return 1;}
 if(strchr(DKR_PACK_NAME,'/')||strstr(DKR_PACK_NAME,"..")){puts("DKRR PACK STOP: invalid basename");return 1;}
 int source_dir=directory(DKR_TEXTURE_DATA_ROOT "/profile-us-v77/texture-packs");
 if(source_dir<0){perror("DKRR PACK STOP: own source directory");return 2;}
 int source=openat(source_dir,DKR_PACK_NAME,O_RDONLY|O_NOFOLLOW|O_NONBLOCK);close(source_dir);
 if(source<0||!matches(source)){puts("DKRR PACK STOP: original pack missing or exact size/hash mismatch");if(source>=0)close(source);return 3;}
 puts("DKRR PACK: existing US 1.0 source hash verified; no pack bytes received from PC");
 int profile=directory(DKR_TEXTURE_DATA_ROOT "/profile-us-v80");
 if(profile<0){close(source);puts("DKRR PACK STOP: existing own US 1.1 profile required");return 4;}
 if(mkdirat(profile,"texture-packs",0755)&&errno!=EEXIST){close(profile);close(source);return 5;}
 int target_dir=openat(profile,"texture-packs",O_RDONLY|O_DIRECTORY|O_NOFOLLOW);close(profile);
 if(target_dir<0){close(source);puts("DKRR PACK STOP: target leaf is not an ordinary directory");return 5;}
 int prior=openat(target_dir,DKR_PACK_NAME,O_RDONLY|O_NOFOLLOW|O_NONBLOCK);
 if(prior>=0){int ok=matches(prior);close(prior);close(source);close(target_dir);puts(ok?"DKRR PACK COPY PASS: US 1.1 pack already matches":"DKRR PACK STOP: existing different target retained");return ok?0:6;}
 if(errno!=ENOENT){close(source);close(target_dir);puts("DKRR PACK STOP: unknown existing target");return 6;}
 const char *temporary=DKR_PACK_NAME ".dkrr-copying";
 int target=openat(target_dir,temporary,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0644);
 if(target<0){close(source);close(target_dir);puts("DKRR PACK STOP: existing temporary file retained; inspect before retry");return 7;}
 static unsigned char buffer[262144];unsigned long long copied=0,reported=0;int ok=lseek(source,0,SEEK_SET)>=0;
 while(ok&&copied<DKR_PACK_BYTES){
  size_t want=sizeof(buffer);if(DKR_PACK_BYTES-copied<want)want=(size_t)(DKR_PACK_BYTES-copied);
  ssize_t got=read(source,buffer,want);if(got<0&&errno==EINTR)continue;if(got<=0){ok=0;break;}
  size_t at=0;while(at<(size_t)got){ssize_t wrote=write(target,buffer+at,(size_t)got-at);if(wrote<0&&errno==EINTR)continue;if(wrote<=0){ok=0;break;}at+=(size_t)wrote;}
  copied+=at;
  if(copied-reported>=64ULL*1024*1024){reported=copied;printf("DKRR PACK: copied %llu / %llu bytes\n",copied,(unsigned long long)DKR_PACK_BYTES);}
 }
 close(source);if(ok&&(fchmod(target,0644)||fsync(target)))ok=0;if(close(target))ok=0;
 target=openat(target_dir,temporary,O_RDONLY|O_NOFOLLOW|O_NONBLOCK);
 if(target<0||!matches(target))ok=0;if(target>=0)close(target);
 struct stat appeared;if(fstatat(target_dir,DKR_PACK_NAME,&appeared,AT_SYMLINK_NOFOLLOW)==0||errno!=ENOENT)ok=0;
 if(ok&&renameat(target_dir,temporary,target_dir,DKR_PACK_NAME))ok=0;
 if(!ok){unlinkat(target_dir,temporary,0);close(target_dir);puts("DKRR PACK STOP: copy not published; existing destination retained");return 8;}
 target=openat(target_dir,DKR_PACK_NAME,O_RDONLY|O_NOFOLLOW|O_NONBLOCK);ok=target>=0&&matches(target);if(target>=0)close(target);close(target_dir);
 puts(ok?"DKRR PACK COPY PASS: verified US 1.1 pack; enable it in that profile's Textures page on next start":"DKRR PACK STOP: final read-back failed; inspect before retry");return ok?0:9;
}
