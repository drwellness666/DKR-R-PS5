/* SPDX-License-Identifier: GPL-3.0-or-later
 * libc++ asks pathconf('.', _PC_PATH_MAX) before getcwd. The native import
 * resolves to NULL. Supply the path/name bounds of the pinned PS5 headers;
 * unsupported selectors fail rather than inventing filesystem capabilities.
 */
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>
long pathconf(const char *path,int name) {
    struct stat info;
    if (stat(path,&info)<0) return -1;
    if (name==_PC_PATH_MAX) return PATH_MAX;
    if (name==_PC_NAME_MAX) return NAME_MAX;
    errno=EINVAL;return -1;
}
