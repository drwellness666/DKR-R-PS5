/* SPDX-License-Identifier: GPL-3.0-or-later
 * Stages only this private preview title and its UI/controller assets. Never launches or mounts a title,
 * changes a whitelist, or replaces an existing different file.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "native_game_files.h"
#ifndef DKR_DIAGNOSTICS_DIRECTORY
#define DKR_DIAGNOSTICS_DIRECTORY "/data/dkrr/diagnostics"
#endif

static int directory(const char *path) {
    struct stat st;
    // The payload creates directories as a different identity from the native
    // game (uid 1). Provision its journal leaf too; sticky mode protects files
    // owned by another identity, and individual journals are still mode 0600.
    unsigned mode=!strcmp(path,DKR_DIAGNOSTICS_DIRECTORY)?01777:
        (!strcmp(path,"/data/dkrr/profile-us-v77") || !strcmp(path,"/data/dkrr/profile-us-v80") || !strcmp(path,"/data/dkrr/renderer") || !strcmp(path,"/data/dkrr/preferences")?0777:0755);
    int created=mkdir(path,(mode_t)mode)==0;
    if (!created && errno!=EEXIST) return -1;
    // Existing directories retain their permissions. Fresh native profiles
    // need write access despite the payload's different filesystem identity.
    if (created && chmod(path,(mode_t)mode)<0) return -1;
    if (lstat(path,&st)<0 || !S_ISDIR(st.st_mode)) { errno=ENOTDIR;return -1; }
    if (!strcmp(path,DKR_DIAGNOSTICS_DIRECTORY))
        printf("native-install: own journal directory mode=%o uid=%u gid=%u %s\n",(unsigned)(st.st_mode&07777),(unsigned)st.st_uid,(unsigned)st.st_gid,path);
    return 0;
}
static int same(const char *path,const unsigned char *bytes,size_t count) {
    int fd=open(path,O_RDONLY|O_NOFOLLOW);
    if(fd<0)return 0;
    struct stat st;unsigned char buffer[4096];size_t position=0;int matches=1;
    if(fstat(fd,&st)<0 || !S_ISREG(st.st_mode) || (uint64_t)st.st_size!=count)matches=0;
    while(matches && position<count) {
        size_t wanted=count-position;if(wanted>sizeof(buffer))wanted=sizeof(buffer);
        ssize_t got=read(fd,buffer,wanted);
        if(got<0 && errno==EINTR)continue;
        if(got<=0 || memcmp(buffer,bytes+position,(size_t)got)) { matches=0;break; }
        position+=(size_t)got;
    }
    close(fd);return matches;
}
static int install(const char *path,const unsigned char *bytes,size_t count,unsigned mode) {
    if(same(path,bytes,count)) {
        int fd=open(path,O_RDONLY|O_NOFOLLOW);
        if(fd<0)return -1;
        int rc=fchmod(fd,(mode_t)mode);close(fd);
        printf("native-install: existing bytes match; mode=%o rc=%d %s\n",mode,rc,path);
        return rc;
    }
    int fd=open(path,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,(mode_t)mode);
    if(fd<0) { fprintf(stderr,"native-install: refused %s: %s\n",path,strerror(errno));return -1; }
    size_t position=0;int ok=1;
    while(position<count) {
        ssize_t wrote=write(fd,bytes+position,count-position);
        if(wrote<0 && errno==EINTR)continue;
        if(wrote<=0) { ok=0;break; }
        position+=(size_t)wrote;
    }
    if(fchmod(fd,(mode_t)mode)<0)ok=0;
    if(fsync(fd)<0)ok=0;
    if(close(fd)<0)ok=0;
    if(!ok || !same(path,bytes,count)) {
        unlink(path);fprintf(stderr,"native-install: write/verification FAIL %s\n",path);return -1;
    }
    printf("native-install: verified %zu bytes: %s\n",count,path);return 0;
}
int main(void) {
    setvbuf(stdout,NULL,_IOLBF,0);setvbuf(stderr,NULL,_IOLBF,0);
    puts("DKR-R 1.0.7 native preview installer (private build; no ROM included)");
    for(size_t i=0;i<sizeof(native_dirs)/sizeof(native_dirs[0]);++i) {
        if(directory(native_dirs[i])<0) { fprintf(stderr,"native-install: directory FAIL %s: %s\n",native_dirs[i],strerror(errno));return 1; }
    }
    // Publish executable and metadata last. Verify every existing file before
    // changing the directory, so a collision leaves its files untouched.
    for(size_t i=0;i<sizeof(native_files)/sizeof(native_files[0]);++i) {
        const struct native_file *f=&native_files[i];struct stat st;
        if(lstat(f->path,&st)==0) {
            if(!S_ISREG(st.st_mode) || !same(f->path,f->start,(size_t)(f->end-f->start))) {
                fprintf(stderr,"native-install: collision, refused %s\n",f->path);return 2;
            }
        } else if(errno!=ENOENT) { perror("native-install: preflight");return 2; }
    }
    for(size_t i=0;i<sizeof(native_files)/sizeof(native_files[0]);++i) {
        const struct native_file *f=&native_files[i];
        if(install(f->path,f->start,(size_t)(f->end-f->start),f->mode)<0)return 3;
    }
    puts("NATIVE INSTALL PASS. Files staged only; mounting and gameplay untested.");return 0;
}
