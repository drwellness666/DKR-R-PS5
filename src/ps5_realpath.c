/* SPDX-License-Identifier: GPL-3.0-or-later
 * Resolve existing paths using PS5-supported POSIX filesystem calls. The
 * native runtime's realpath refuses title paths with EPERM. Preserve physical
 * symlink resolution on the host, existence checks and directory traversal;
 * lexical normalization is insufficient for the game's profile/mod policies.
 * PS5 titles cannot read links: the native lstat adapter rejects them instead.
 */
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

char *realpath(const char *restrict path, char *restrict output) {
    char pending[PATH_MAX], resolved[PATH_MAX];
    struct stat info;
#if !defined(__PROSPERO__)
    char link[PATH_MAX];
    unsigned followed=0;
#endif
    if (!path) { errno=EINVAL; return NULL; }
    size_t length=strnlen(path,PATH_MAX);
    if (!length) { errno=ENOENT; return NULL; }
    if (length==PATH_MAX) { errno=ENAMETOOLONG; return NULL; }
    memcpy(pending,path,length+1);
    if (path[0]=='/') { resolved[0]='/'; resolved[1]=0; }
    else if (!getcwd(resolved,sizeof(resolved))) return NULL;
    if (lstat(resolved,&info)<0) return NULL;
    size_t used=strlen(resolved);
    char *cursor=pending;
    while (*cursor) {
        while (*cursor=='/') ++cursor;
        if (!*cursor) break;
        char *component=cursor;
        while (*cursor && *cursor!='/') ++cursor;
        size_t count=(size_t)(cursor-component);
        if (count==1 && component[0]=='.') continue;
        if (count==2 && component[0]=='.' && component[1]=='.') {
            while (used>1 && resolved[used-1]!='/') --used;
            if (used>1) --used;
            resolved[used]=0;
            continue;
        }
#if !defined(__PROSPERO__)
        size_t parent=used;
#endif
        size_t separator=used>1;
        if (used+separator+count>=sizeof(resolved)) { errno=ENAMETOOLONG; return NULL; }
        if (separator) resolved[used++]='/';
        memcpy(resolved+used,component,count); used+=count; resolved[used]=0;
        if (lstat(resolved,&info)<0) return NULL;
        if (S_ISLNK(info.st_mode)) {
#if defined(__PROSPERO__)
            errno=ENOTSUP; return NULL;
#else
            if (++followed>40) { errno=ELOOP; return NULL; }
            ssize_t size=readlink(resolved,link,sizeof(link));
            if (size<0) return NULL;
            if (size==0) { errno=ENOENT; return NULL; }
            size_t rest=strlen(cursor);
            if ((size_t)size+rest>=sizeof(pending)) { errno=ENAMETOOLONG; return NULL; }
            memmove(pending+(size_t)size,cursor,rest+1);
            memcpy(pending,link,(size_t)size);
            if (link[0]=='/') { used=1; resolved[1]=0; }
            else { used=parent; resolved[used]=0; }
            cursor=pending;
#endif
        } else if (*cursor && !S_ISDIR(info.st_mode)) {
            errno=ENOTDIR; return NULL;
        }
    }
    if (!output) {
        output=malloc(used+1);
        if (!output) { errno=ENOMEM; return NULL; }
    }
    memcpy(output,resolved,used+1);
    return output;
}
