/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
ssize_t dkrr_ps5_readlink(const char *, char *, size_t);
int dkrr_ps5_symlink(const char *, const char *);
int dkrr_ps5_link(const char *, const char *);
int dkrr_ps5_fchown(int, uid_t, gid_t);
int dkrr_ps5_mkstemp(char *);
char *dkrr_ps5_strcasestr(const char *, const char *);
int main(int argc, char **argv) {
    assert(argc == 2);
    char buffer[64];
    errno = 0; assert(dkrr_ps5_readlink("/proc/self/exe", buffer, sizeof(buffer)) == -1 && errno == ENOSYS);
    errno = 0; assert(dkrr_ps5_symlink("a", "b") == -1 && errno == ENOSYS);
    errno = 0; assert(dkrr_ps5_link("a", "b") == -1 && errno == ENOSYS);
    const int fd = open(argv[1], O_RDWR | O_CREAT | O_TRUNC, 0600);
    assert(fd >= 0);
    assert(dkrr_ps5_fchown(fd, (uid_t)-1, (gid_t)-1) == 0 && dkrr_ps5_fchown(fd, geteuid(), getegid()) == 0);
    if (geteuid() != 0) { errno = 0; assert(dkrr_ps5_fchown(fd, geteuid() + 1, (gid_t)-1) == -1 && errno == EPERM); }
    close(fd);
    unlink(argv[1]);
    /* mkstemp: distinct, exclusive, private, only the suffix changes. */
    char names[200][512];
    for (int i = 0; i < 200; ++i) {
        snprintf(names[i], sizeof(names[i]), "%s.tmp-XXXXXX", argv[1]);
        const size_t prefix = strlen(names[i]) - 6;
        const int tmp = dkrr_ps5_mkstemp(names[i]);
        assert(tmp >= 0);
        struct stat st;
        assert(fstat(tmp, &st) == 0 && (st.st_mode & 0777) == 0600);
        assert(strncmp(names[i], argv[1], strlen(argv[1])) == 0 && strlen(names[i]) == prefix + 6);
        for (size_t k = prefix; k < prefix + 6; ++k) assert(strchr("abcdefghijklmnopqrstuvwxyz0123456789", names[i][k]));
        for (int j = 0; j < i; ++j) assert(strcmp(names[i], names[j]) != 0);
        close(tmp);
    }
    for (int i = 0; i < 200; ++i) unlink(names[i]);
    char bad[] = "no-suffix";
    errno = 0; assert(dkrr_ps5_mkstemp(bad) == -1 && errno == EINVAL);
    char directory[] = "/nonexistent-dkrr-dir/fileXXXXXX";
    errno = 0; assert(dkrr_ps5_mkstemp(directory) == -1 && errno == ENOENT);
    /* strcasestr */
    const char *text = "Controller SOUNDS and Horns";
    assert(dkrr_ps5_strcasestr(text, "sounds") == text + 11);
    assert(dkrr_ps5_strcasestr(text, "") == text);
    assert(dkrr_ps5_strcasestr(text, "horns") == text + 22);
    assert(dkrr_ps5_strcasestr(text, "hornss") == NULL);
    assert(dkrr_ps5_strcasestr("", "a") == NULL);
    assert(dkrr_ps5_strcasestr("aab", "ab") != NULL);
    puts("PASS");
    return 0;
}
