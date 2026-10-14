/* SPDX-License-Identifier: GPL-3.0-or-later
 * Stages this diagnostic's four files. Never launches or mounts a title,
 * changes a whitelist, or replaces an existing different file.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "native_probe_files.h"

static int directory(const char *path) {
    struct stat st;
    if (mkdir(path,0755)<0 && errno!=EEXIST) return -1;
    if (lstat(path,&st)<0 || !S_ISDIR(st.st_mode)) { errno=ENOTDIR;return -1; }
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
    puts("DKR-R native renderer title installer (diagnostic only, no ROM)");
    const char *folders[]={"/data/homebrew","/data/homebrew/PPSA94677","/data/homebrew/PPSA94677/sce_sys","/data/homebrew/PPSA94677/sce_module"};
    for(size_t i=0;i<sizeof(folders)/sizeof(folders[0]);++i) {
        if(directory(folders[i])<0) { fprintf(stderr,"native-install: directory FAIL %s: %s\n",folders[i],strerror(errno));return 1; }
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
