/* SPDX-License-Identifier: GPL-3.0-or-later
 * Replaces only files whose full bytes match a recorded preceding build.
 * New files are fsynced, byte/hash-verified and atomically renamed in place.
 * The old build is preserved on the PC. No settings or foreign files change.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "native_update_files.h"
int dkrr_sha1_fd(int fd,unsigned char digest[20]);
#ifndef DKR_UPDATE_VERSION
#define DKR_UPDATE_VERSION 4
#endif
static int matches_hex(const unsigned char digest[20],const char *hex) {
    const char digits[]="0123456789abcdef";
    for(int i=0;i<20;++i)if(hex[2*i]!=digits[digest[i]>>4] || hex[2*i+1]!=digits[digest[i]&15])return 0;
    return 1;
}
static int ordinary_parents(const char *path) {
    char parent[256];size_t length=strlen(path);
    if(path[0]!='/' || length>=sizeof(parent))return 0;
    memcpy(parent,path,length+1);
    for(size_t i=1;i<length;++i)if(parent[i]=='/') {
        struct stat info;parent[i]=0;
        int ok=lstat(parent,&info)==0 && S_ISDIR(info.st_mode);
        parent[i]='/';if(!ok)return 0;
    }
    return 1;
}
static int state(const struct update_file *f,const char *path,int new_only) {
    if(!ordinary_parents(path))return -1;
    int fd=open(path,O_RDONLY|O_NOFOLLOW);if(fd<0)return -1;
    struct stat st;unsigned char digest[20];int result=-1;
    if(fstat(fd,&st)==0 && S_ISREG(st.st_mode) && dkrr_sha1_fd(fd,digest)==0) {
        if((uint64_t)st.st_size==f->new_size && matches_hex(digest,f->new_sha1))result=1;
        else if(!new_only && (uint64_t)st.st_size==f->old_size && matches_hex(digest,f->old_sha1))result=2;
    }
    close(fd);return result;
}
static int native_update_preflight(void) {
    for(size_t i=0;i<sizeof(update_files)/sizeof(update_files[0]);++i) {
        int s=state(&update_files[i],update_files[i].path,0);
        printf("native-update: preflight %s state=%d (1=new, 2=recorded old)\n",update_files[i].path,s);
        if(s<0) { puts("native-update: refused unknown/different file; nothing changed");return 1; }
        if(s==2){
            char temporary[256];struct stat info;
            int length=snprintf(temporary,sizeof(temporary),"%s.dkrr-v%d-update",update_files[i].path,DKR_UPDATE_VERSION);
            if(length<0||(size_t)length>=sizeof(temporary)||lstat(temporary,&info)==0||errno!=ENOENT){
                puts("native-update: refused temporary-file collision; nothing changed");return 1;
            }
        }
    }
    return 0;
}
int main(void) {
    setvbuf(stdout,NULL,_IOLBF,0);
    printf("DKR-R native probe updater -> v%d: exact previous-byte hashes required\n",DKR_UPDATE_VERSION);
    if(native_update_preflight()!=0)return 1;
    for(size_t i=0;i<sizeof(update_files)/sizeof(update_files[0]);++i) {
        const struct update_file *f=&update_files[i];
        if(state(f,f->path,0)==1)continue;
        char tmp[256];int len=snprintf(tmp,sizeof(tmp),"%s.dkrr-v%d-update",f->path,DKR_UPDATE_VERSION);
        if(len<0 || (size_t)len>=sizeof(tmp))return 2;
        int fd=open(tmp,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,(mode_t)f->mode);
        if(fd<0) { perror("native-update: create temp refused");return 2; }
        size_t pos=0;int ok=1;
        while(pos<f->new_size) {
            ssize_t n=write(fd,f->start+pos,(size_t)f->new_size-pos);
            if(n<0 && errno==EINTR)continue;
            if(n<=0) { ok=0;break; }
            pos+=(size_t)n;
        }
        if(fchmod(fd,(mode_t)f->mode)<0 || fsync(fd)<0)ok=0;
        if(close(fd)<0)ok=0;
        if(!ok || state(f,tmp,1)!=1 || state(f,f->path,0)!=2) {
            unlink(tmp);puts("native-update: refused before commit; target left untouched");return 3;
        }
        if(rename(tmp,f->path)<0) { unlink(tmp);perror("native-update: rename");return 4; }
        if(state(f,f->path,1)!=1) { puts("native-update: final verification FAIL");return 5; }
        printf("native-update: committed and verified %s\n",f->path);
    }
    puts("NATIVE UPDATE PASS. Own files staged only; GPU run not implied.");return 0;
}
