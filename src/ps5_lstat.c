/* SPDX-License-Identifier: GPL-3.0-or-later
 * Native titles refuse lstat even though stat and getdents work. Inspect the
 * final directory entry before stat. Links and unknown entry types fail
 * closed; never silently turn a link into an ordinary file. This stricter
 * subset supports the game's no-link storage policy without privilege changes.
 */
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int lstat(const char *restrict path, struct stat *restrict output) {
    char parent[PATH_MAX];
    size_t length=strnlen(path,PATH_MAX);
    if (!length) { errno=ENOENT; return -1; }
    if (length==PATH_MAX) { errno=ENAMETOOLONG; return -1; }
    // Root, trailing slash, dot and dot-dot name directories rather than links.
    const char *name=strrchr(path,'/');
    name=name?name+1:path;
    if (!*name || !strcmp(name,".") || !strcmp(name,"..")) return stat(path,output);
    if (name==path) strcpy(parent,".");
    else if (name==path+1) strcpy(parent,"/");
    else { size_t size=(size_t)(name-path)-1;memcpy(parent,path,size);parent[size]=0; }
    DIR *directory=opendir(parent);
    if (!directory) return -1;
    int found=0,error=0;unsigned type=DT_UNKNOWN;
    for (;;) {
        errno=0;struct dirent *entry=readdir(directory);
        if (!entry) { error=errno;break; }
        if (!strcmp(entry->d_name,name)) { type=entry->d_type;found=1;break; }
    }
    closedir(directory);
    if (!found) { errno=error?error:ENOENT;return -1; }
    if (type!=DT_REG && type!=DT_DIR) { errno=ENOTSUP;return -1; }
    // O_NOFOLLOW also rejects a link swapped in after directory enumeration.
    int fd=open(path,O_RDONLY|O_NOFOLLOW|O_NONBLOCK);
    if (fd<0) return -1;
    int rc=fstat(fd,output),saved=errno;
    close(fd);
    if (rc<0) { errno=saved;return -1; }
    // Refuse a changed entry whose resolved type differs from the enumeration.
    if (((unsigned)output->st_mode&S_IFMT)!=(type<<12)) { errno=EAGAIN;return -1; }
    return 0;
}
