/* SPDX-License-Identifier: GPL-3.0-or-later
 * One own-title update: all destinations preflighted before any write, known
 * old/new bytes only, added assets/ROMs included, executable published last.
 * Files commit individually. Settings, saves and unknown files are not targets.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "native_deploy_files.h"
int dkrr_sha1_fd(int fd,unsigned char digest[20]);
#define COUNT(a) (sizeof(a)/sizeof((a)[0]))
static int parents(const char *path,int allow_missing) {
    char part[256];size_t len=strlen(path);
    if(path[0]!='/' || len>=sizeof(part))return 0;
    memcpy(part,path,len+1);
    for(size_t i=1;i<len;++i)if(part[i]=='/') {
        struct stat st;part[i]=0;int rc=lstat(part,&st);part[i]='/';
        if(rc<0) { if(allow_missing && errno==ENOENT)continue; return 0; }
        if(!S_ISDIR(st.st_mode))return 0;
    }
    return 1;
}
static int hex_matches(const unsigned char digest[20],const char *hex) {
    const char digits[]="0123456789abcdef";
    for(int i=0;i<20;++i)if(hex[2*i]!=digits[digest[i]>>4] || hex[2*i+1]!=digits[digest[i]&15])return 0;
    return 1;
}
/* 0 absent, 1 current, 2 known previous, -1 refused. */
static int file_state(const struct deploy_file *f,const char *path) {
    if(!parents(path,1))return -1;
    struct stat st;
    if(lstat(path,&st)<0)return errno==ENOENT?0:-1;
    if(!S_ISREG(st.st_mode))return -1;
    int fd=open(path,O_RDONLY|O_NOFOLLOW);if(fd<0)return -1;
    unsigned char digest[20];int result=-1;
    if(fstat(fd,&st)==0 && S_ISREG(st.st_mode) && st.st_nlink<=1 && dkrr_sha1_fd(fd,digest)==0) {
        if((uint64_t)st.st_size==f->new_size && hex_matches(digest,f->new_sha1))result=1;
        else if(f->old_sha1 && (uint64_t)st.st_size==f->old_size && hex_matches(digest,f->old_sha1))result=2;
    }
    close(fd);return result;
}
static int temp_path(const char *path,char *out,size_t size) {
    int n=snprintf(out,size,"%s.dkrr-deploy-tmp",path);
    return n>=0 && (size_t)n<size;
}
static int parent_sync(const char *path) {
    char part[256];size_t len=strlen(path);if(len>=sizeof(part))return -1;
    memcpy(part,path,len+1);char *slash=strrchr(part,'/');if(!slash)return -1;*slash=0;
    int fd=open(part,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);if(fd<0)return -1;
    int rc=fsync(fd);if(close(fd)<0)rc=-1;return rc;
}
static int preflight(void) {
    for(size_t i=0;i<COUNT(deploy_dirs);++i) {
        const char *path=deploy_dirs[i].path;struct stat st;
        if(!parents(path,1))return -1;
        if(lstat(path,&st)==0) { if(!S_ISDIR(st.st_mode))return -1; }
        else if(errno!=ENOENT)return -1;
    }
    for(size_t i=0;i<COUNT(deploy_files);++i) {
        const struct deploy_file *f=&deploy_files[i];int s=file_state(f,f->path);
        printf("native-deploy: preflight state=%d %s\n",s,f->path);
        if(s<0 || (s==0 && f->required))return -1;
        if(s!=1) {
            char tmp[256];struct stat st;
            if(!f->data || !temp_path(f->path,tmp,sizeof(tmp)) || lstat(tmp,&st)==0 || errno!=ENOENT)return -1;
        }
    }
    return 0;
}
int main(void) {
    setvbuf(stdout,NULL,_IOLBF,0);setvbuf(stderr,NULL,_IOLBF,0);
    puts("PRIVATE DKR-R combined update; both owned ROMs. Never share this ELF.");
    if(preflight()<0) { puts("native-deploy: unknown files or temporary collision; nothing changed");return 1; }
    for(size_t i=0;i<COUNT(deploy_dirs);++i) {
        const char *path=deploy_dirs[i].path;struct stat st;
        if(!parents(path,0))return 2;
        if(mkdir(path,(mode_t)deploy_dirs[i].mode)==0) {
            if(chmod(path,(mode_t)deploy_dirs[i].mode)<0 || parent_sync(path)<0)return 2;
        } else if(errno!=EEXIST)return 2;
        if(lstat(path,&st)<0 || !S_ISDIR(st.st_mode))return 2;
    }
    for(size_t i=0;i<COUNT(deploy_files);++i) {
        const struct deploy_file *f=&deploy_files[i];int before=file_state(f,f->path);
        if(before==1)continue;
        if(before<0 || (before==0 && f->required) || !f->data)return 3;
        char tmp[256];if(!temp_path(f->path,tmp,sizeof(tmp)) || !parents(f->path,0))return 3;
        int fd=open(tmp,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,(mode_t)f->mode);if(fd<0)return 3;
        size_t pos=0;int ok=1;
        while(pos<f->new_size) {
            ssize_t n=write(fd,f->data+pos,(size_t)f->new_size-pos);
            if(n<0 && errno==EINTR)continue;
            if(n<=0) { ok=0;break; }pos+=(size_t)n;
        }
        if(fchmod(fd,(mode_t)f->mode)<0 || fsync(fd)<0)ok=0;
        if(close(fd)<0)ok=0;
        if(!ok || file_state(f,tmp)!=1 || file_state(f,f->path)!=before) { unlink(tmp);return 4; }
        /* New files use a no-clobber link; replacement of a known predecessor
         * uses rename. Both are confined to this title's verified directories. */
        int rc=before==0?link(tmp,f->path):rename(tmp,f->path);
        if(rc<0) { unlink(tmp);return 5; }
        if(before==0 && unlink(tmp)<0)return 5;
        if(parent_sync(f->path)<0 || file_state(f,f->path)!=1)return 6;
        printf("native-deploy: committed and verified %s\n",f->path);
    }
    puts("DKR-R UPDATE PASS. App, assets and both owned revisions verified; saves and settings preserved.");
    return 0;
}
