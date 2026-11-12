/* SPDX-License-Identifier: GPL-3.0-or-later
 * libc functions the PS5 platform does not export.
 *
 * Observed on the console (journal of 1.2.6, "unresolved function import:" and
 * DKRR_CRASH rip=0x0 in the readlink self-test step): these names stay bound to
 * address 0, so the first call jumps to 0 and the process dies with SIGSEGV.
 * Each function here fails the way the system would for an unsupported
 * operation, so callers (std::filesystem, third-party code) take their normal
 * error path instead of crashing. Symbols are kept local to the executable
 * (game-symbols.map) so they never shadow a platform library.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/* Symbolic links cannot be read, created or resolved through these calls. */
ssize_t readlink(const char *path, char *buffer, size_t size) {
    (void)path; (void)buffer; (void)size;
    errno = ENOSYS;
    return -1;
}
int symlink(const char *target, const char *path) {
    (void)target; (void)path;
    errno = ENOSYS;
    return -1;
}
int link(const char *existing, const char *path) {
    (void)existing; (void)path;
    errno = ENOSYS;
    return -1;
}
/* A title cannot give files to someone else. Leaving ownership as it is
 * ((uid_t)-1 / own ids) succeeds; anything else is refused like an unprivileged call. */
int fchown(int fd, uid_t owner, gid_t group) {
    (void)fd;
    const int keeps_owner = owner == (uid_t)-1 || owner == geteuid();
    const int keeps_group = group == (gid_t)-1 || group == getegid();
    if (keeps_owner && keeps_group) return 0;
    errno = EPERM;
    return -1;
}
/* Exclusive create with an unpredictable suffix; never overwrites an existing file. */
int mkstemp(char *pattern) {
    static const char alphabet[] = "abcdefghijklmnopqrstuvwxyz0123456789";
    static uint64_t counter;
    if (!pattern) { errno = EINVAL; return -1; }
    const size_t length = strlen(pattern);
    if (length < 6 || strcmp(pattern + length - 6, "XXXXXX") != 0) { errno = EINVAL; return -1; }
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    for (unsigned attempt = 0; attempt < 128; ++attempt) {
        uint64_t state = (uint64_t)now.tv_nsec * 0x9E3779B97F4A7C15ULL ^ ((uint64_t)getpid() << 32) ^
                         (uint64_t)__atomic_add_fetch(&counter, 0x632BE59BD9B4E019ULL, __ATOMIC_RELAXED);
        for (unsigned i = 0; i < 6; ++i) {
            state ^= state >> 12; state ^= state << 25; state ^= state >> 27;
            pattern[length - 6 + i] = alphabet[(state * 0x2545F4914F6CDD1DULL >> 33) % (sizeof(alphabet) - 1)];
        }
        const int fd = open(pattern, O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
        if (fd >= 0) return fd;
        if (errno != EEXIST) return -1;
    }
    errno = EEXIST;
    return -1;
}
/* Case-insensitive substring search (ASCII), as in glibc. */
char *strcasestr(const char *haystack, const char *needle) {
    if (!*needle) return (char *)haystack;
    for (; *haystack; ++haystack) {
        const char *h = haystack, *n = needle;
        while (*h && *n) {
            const int a = (unsigned char)*h, b = (unsigned char)*n;
            if ((a >= 'A' && a <= 'Z' ? a + 32 : a) != (b >= 'A' && b <= 'Z' ? b + 32 : b)) break;
            ++h; ++n;
        }
        if (!*n) return (char *)haystack;
    }
    return NULL;
}
