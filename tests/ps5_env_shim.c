/* SPDX-License-Identifier: GPL-3.0-or-later
 * Host-side emulation of observed PS5 title-sandbox behaviour.
 *
 * Link this file into a host test executable; its definitions interpose the
 * libc calls of the code under test (and of libstdc++/libc++ it calls). Each
 * rule models ONE observed fact listed with its evidence in docs/PS5_FACTS.md.
 * Nothing here guesses: a behaviour the console has not shown is not modelled.
 *
 * DKRR_PS5_ENV is a comma list of rule names, or "all":
 *   proc    /proc is absent                      -> readlink("/proc/...") ENOENT
 *   nlink0  regular files report zero links      -> st_nlink = 0
 *   uid0    owner is redacted to root            -> st_uid = 0
 *   flock   advisory locks unsupported           -> flock() EOPNOTSUPP
 *   chmod   chmod/fchmodat unavailable           -> ENOSYS (fchmod still works)
 *   fsync   fsync/fdatasync rejected             -> EINVAL
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

static int rule(const char *name) {
    const char *list = getenv("DKRR_PS5_ENV");
    if (!list || !*list) return 0;
    if (!strcmp(list, "all")) return 1;
    const size_t length = strlen(name);
    for (const char *at = list; *at;) {
        const char *end = strchr(at, ',');
        const size_t size = end ? (size_t)(end - at) : strlen(at);
        if (size == length && !strncmp(at, name, length)) return 1;
        if (!end) break;
        at = end + 1;
    }
    return 0;
}
#define REAL(ret, args, name) static ret(*real) args = 0; if (!real) real = (ret(*) args)dlsym(RTLD_NEXT, name)

static void redact(struct stat *st) {
    if (rule("uid0")) st->st_uid = 0;
    if (rule("nlink0") && S_ISREG(st->st_mode)) st->st_nlink = 0;
}
int stat(const char *path, struct stat *st) {
    REAL(int, (const char *, struct stat *), "stat");
    const int result = real(path, st);
    if (!result) redact(st);
    return result;
}
int lstat(const char *path, struct stat *st) {
    REAL(int, (const char *, struct stat *), "lstat");
    const int result = real(path, st);
    if (!result) redact(st);
    return result;
}
int fstat(int fd, struct stat *st) {
    REAL(int, (int, struct stat *), "fstat");
    const int result = real(fd, st);
    if (!result) redact(st);
    return result;
}
int fstatat(int dir, const char *path, struct stat *st, int flags) {
    REAL(int, (int, const char *, struct stat *, int), "fstatat");
    const int result = real(dir, path, st, flags);
    if (!result) redact(st);
    return result;
}
int statx(int dir, const char *path, int flags, unsigned mask, struct statx *out) {
    REAL(int, (int, const char *, int, unsigned, struct statx *), "statx");
    const int result = real(dir, path, flags, mask, out);
    if (!result) {
        if (rule("uid0")) out->stx_uid = 0;
        if (rule("nlink0") && S_ISREG(out->stx_mode)) out->stx_nlink = 0;
    }
    return result;
}
ssize_t readlink(const char *path, char *buffer, size_t size) {
    REAL(ssize_t, (const char *, char *, size_t), "readlink");
    if (rule("proc") && !strncmp(path, "/proc/", 6)) { errno = ENOENT; return -1; }
    return real(path, buffer, size);
}
int flock(int fd, int operation) {
    REAL(int, (int, int), "flock");
    if (rule("flock")) { errno = EOPNOTSUPP; return -1; }
    return real(fd, operation);
}
int chmod(const char *path, mode_t mode) {
    REAL(int, (const char *, mode_t), "chmod");
    if (rule("chmod")) { errno = ENOSYS; return -1; }
    return real(path, mode);
}
int fchmodat(int dir, const char *path, mode_t mode, int flags) {
    REAL(int, (int, const char *, mode_t, int), "fchmodat");
    if (rule("chmod")) { errno = ENOSYS; return -1; }
    return real(dir, path, mode, flags);
}
int fsync(int fd) {
    REAL(int, (int), "fsync");
    if (rule("fsync")) { errno = EINVAL; return -1; }
    return real(fd);
}
int fdatasync(int fd) {
    REAL(int, (int), "fdatasync");
    if (rule("fsync")) { errno = EINVAL; return -1; }
    return real(fd);
}
