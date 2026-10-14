/* SPDX-License-Identifier: GPL-3.0-or-later
 * Installs only the embedded diagnostic into /data/homebrew/DKR-R-Vulkan.
 * Existing differing files and symlinks are refused. Does not launch graphics.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

extern const unsigned char dkrr_gpu_elf_start[], dkrr_gpu_elf_end[];
extern const unsigned char dkrr_gpu_icon_start[], dkrr_gpu_icon_end[];
#ifndef DKR_INSTALL_HOME
#define DKR_INSTALL_HOME "/data/homebrew/DKR-R-Vulkan"
#endif

static int directory(const char *path) {
    struct stat st;
    if (mkdir(path, 0755) < 0 && errno != EEXIST) return -1;
    if (lstat(path, &st) < 0 || !S_ISDIR(st.st_mode)) { errno = ENOTDIR; return -1; }
    return 0;
}
static int same(const char *path, const unsigned char *bytes, size_t count) {
    int fd = open(path, O_RDONLY | O_NOFOLLOW);
    if (fd < 0) return 0;
    unsigned char buffer[4096]; size_t position = 0; int matches = 1;
    struct stat st;
    if (fstat(fd, &st) < 0 || !S_ISREG(st.st_mode) || (uint64_t)st.st_size != count) matches = 0;
    while (matches && position < count) {
        size_t wanted = count - position; if (wanted > sizeof(buffer)) wanted = sizeof(buffer);
        ssize_t read_count = read(fd, buffer, wanted);
        if (read_count < 0 && errno == EINTR) continue;
        if (read_count <= 0 || memcmp(buffer, bytes + position, (size_t)read_count)) { matches = 0; break; }
        position += (size_t)read_count;
    }
    close(fd); return matches;
}
static int install(const char *path, const unsigned char *bytes, size_t count) {
    if (same(path, bytes, count)) { printf("install: already matches %s\n", path); return 0; }
    int fd = open(path, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0644);
    if (fd < 0) { fprintf(stderr, "install: refused %s: %s\n", path, strerror(errno)); return -1; }
    size_t position = 0; int success = 1;
    while (position < count) {
        ssize_t written = write(fd, bytes + position, count - position);
        if (written < 0 && errno == EINTR) continue;
        if (written <= 0) { success = 0; break; }
        position += (size_t)written;
    }
    if (fsync(fd) < 0) success = 0;
    if (close(fd) < 0) success = 0;
    if (!success || !same(path, bytes, count)) {
        // This process created this file with O_EXCL; no pre-existing file is removed.
        unlink(path); fprintf(stderr, "install: FAIL write/verification: %s\n", path); return -1;
    }
    printf("install: verified %zu bytes: %s\n", count, path); return 0;
}
int main(void) {
    setvbuf(stdout, NULL, _IOLBF, 0); setvbuf(stderr, NULL, _IOLBF, 0);
    puts("DKR-R Vulkan diagnostic installer v1 (no game, no ROM)");
    const char *folders[] = {"/data/homebrew", DKR_INSTALL_HOME, DKR_INSTALL_HOME "/sce_sys"};
    for (size_t i = 0; i < sizeof(folders) / sizeof(folders[0]); ++i) {
        if (directory(folders[i]) < 0) { fprintf(stderr, "install: directory failed %s: %s\n", folders[i], strerror(errno)); return 1; }
    }
    if (install(DKR_INSTALL_HOME "/eboot.elf", dkrr_gpu_elf_start, (size_t)(dkrr_gpu_elf_end - dkrr_gpu_elf_start)) < 0) return 2;
    if (install(DKR_INSTALL_HOME "/sce_sys/icon0.png", dkrr_gpu_icon_start, (size_t)(dkrr_gpu_icon_end - dkrr_gpu_icon_start)) < 0) return 3;
    puts("INSTALL PASS. Probe installed, not started. Existing different files were not overwritten.");
    return 0;
}
