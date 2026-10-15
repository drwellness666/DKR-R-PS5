// SPDX-License-Identifier: GPL-3.0-or-later
// Own, next-launch display preference. No system settings or game saves.
#include "ps5_display_options.h"
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <initializer_list>
#include <sys/stat.h>
#include <unistd.h>
#ifndef DKR_DISPLAY_DATA_ROOT
#define DKR_DISPLAY_DATA_ROOT "/data/dkrr"
#endif
#ifndef DKR_PS5_HIGH_REFRESH_BUILD
#define DKR_PS5_HIGH_REFRESH_BUILD 0
#endif
namespace {
constexpr const char *directory = DKR_DISPLAY_DATA_ROOT "/preferences";
constexpr const char *preference = DKR_DISPLAY_DATA_ROOT "/preferences/display-output.txt";
constexpr const char *temporary = DKR_DISPLAY_DATA_ROOT "/preferences/display-output.txt.next";
std::atomic<unsigned> boot_resolution{1080}, boot_refresh{60};
std::atomic<unsigned> active_width{0}, active_height{0}, active_refresh{0}, active_fallback{0};
bool valid(dkrr_ps5_display_choice c) {
    return (c.resolution == 1080 || c.resolution == 1440 || c.resolution == 2160) &&
           (c.refresh == 60 || c.refresh == 120);
}
bool own_directory() {
    struct stat st{};
    return lstat(DKR_DISPLAY_DATA_ROOT, &st) == 0 && S_ISDIR(st.st_mode) &&
           lstat(directory, &st) == 0 && S_ISDIR(st.st_mode);
}
int read_choice(dkrr_ps5_display_choice *choice) {
    if (!choice) { errno = EINVAL; return -1; }
    *choice = {1080, 60};
    if (!own_directory()) { errno = ENOTDIR; return -1; }
    const int fd = open(preference, O_RDONLY | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) return errno == ENOENT ? 0 : -1;
    struct stat st{}; char text[65]{}; size_t bytes = 0;
    bool ok = fstat(fd, &st) == 0 && S_ISREG(st.st_mode) && st.st_size > 0 && st.st_size <= 64;
    while (ok && bytes < sizeof(text)) {
        const ssize_t n = read(fd, text + bytes, sizeof(text) - bytes);
        if (n < 0 && errno == EINTR) continue;
        if (n < 0) { ok = false; break; }
        if (!n) break;
        bytes += size_t(n);
    }
    close(fd);
    if (ok) for (const unsigned res : {1080U, 1440U, 2160U}) for (const unsigned hz : {60U, 120U}) {
        char expected[65];
        const int length = std::snprintf(expected, sizeof(expected), "resolution=%up\nrefresh=%u\n", res, hz);
        if (bytes == size_t(length) && !std::memcmp(text, expected, bytes)) {
            *choice = {res, hz}; return 1;
        }
    }
    errno = EINVAL; return -1;
}
}
extern "C" int dkrr_ps5_read_display_choice(dkrr_ps5_display_choice *choice) { return read_choice(choice); }
extern "C" void dkrr_ps5_initialize_display() {
    dkrr_ps5_display_choice c{1080, 60}; read_choice(&c);
    boot_resolution.store(c.resolution, std::memory_order_relaxed);
    boot_refresh.store(DKR_PS5_HIGH_REFRESH_BUILD ? c.refresh : 60, std::memory_order_relaxed);
}
extern "C" dkrr_ps5_display_choice dkrr_ps5_boot_display_choice() {
    return {boot_resolution.load(std::memory_order_relaxed), boot_refresh.load(std::memory_order_relaxed)};
}
extern "C" void dkrr_ps5_report_display(unsigned width, unsigned height, unsigned refresh) {
    const auto c = dkrr_ps5_boot_display_choice();
    active_width.store(width, std::memory_order_relaxed); active_height.store(height, std::memory_order_relaxed);
    active_refresh.store(refresh, std::memory_order_relaxed);
    active_fallback.store(height != c.resolution || (refresh + 500) / 1000 != c.refresh, std::memory_order_relaxed);
}
extern "C" struct dkrr_ps5_display_status dkrr_ps5_display_status() {
    return {active_width.load(std::memory_order_relaxed), active_height.load(std::memory_order_relaxed),
            active_refresh.load(std::memory_order_relaxed), active_fallback.load(std::memory_order_relaxed),
            DKR_PS5_HIGH_REFRESH_BUILD};
}
extern "C" int dkrr_ps5_choose_display(dkrr_ps5_display_choice choice) {
    if (!valid(choice) || (choice.refresh == 120 && !DKR_PS5_HIGH_REFRESH_BUILD)) { errno = EINVAL; return -1; }
    if (!own_directory()) { errno = ENOTDIR; return -1; }
    struct stat old{}; const bool existed = lstat(preference, &old) == 0;
    dkrr_ps5_display_choice prior{};
    if (existed) { if (!S_ISREG(old.st_mode) || read_choice(&prior) != 1) { errno = EINVAL; return -1; } }
    else if (errno != ENOENT) return -1;
    const int fd = open(temporary, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0644);
    if (fd < 0) return -1;
    char text[65];
    const size_t bytes = size_t(std::snprintf(text, sizeof(text), "resolution=%up\nrefresh=%u\n", choice.resolution, choice.refresh));
    size_t position = 0; bool ok = true;
    while (position < bytes) {
        const ssize_t n = write(fd, text + position, bytes - position);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) { ok = false; break; } position += size_t(n);
    }
    if (fchmod(fd, 0644) || fsync(fd)) ok = false;
    if (close(fd)) ok = false;
    struct stat now{}; const int checked = lstat(preference, &now);
    if (checked == 0) {
        if (!existed || !S_ISREG(now.st_mode) || now.st_ino != old.st_ino || now.st_size != old.st_size ||
            now.st_mtime != old.st_mtime || read_choice(&prior) != 1) ok = false;
    } else if (errno != ENOENT || existed) ok = false;
    if (!ok) { unlink(temporary); errno = EIO; return -1; }
    if (rename(temporary, preference)) { unlink(temporary); return -1; }
    if (read_choice(&prior) != 1 || prior.resolution != choice.resolution || prior.refresh != choice.refresh) {
        errno = EIO; return -1;
    }
    return 0;
}
