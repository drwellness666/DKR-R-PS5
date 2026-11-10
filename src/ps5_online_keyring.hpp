// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <atomic>
#include <cerrno>
#include <string>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "ps5_native_metadata.hpp"
// PS5 has no desktop Secret Service and never invokes payload dlopen from a
// native title. "Keep me signed in" instead stores the 43-character device
// token in a private file inside the profile, which only this title can open.
// Upstream's own local_device store rejects every file here because sandboxed
// native titles see uid=0/nlink=0 (see ps5_native_metadata.hpp); this store
// applies the same checks with that redaction accepted. The path is fixed by
// the caller, opened without following links, and never leaves the profile.
namespace dkr::online::keyring {
namespace ps5_detail {
struct Fd {
    int value = -1;
    explicit Fd(int fd) : value(fd) {}
    ~Fd() { if (value >= 0) close(value); }
    Fd(const Fd&) = delete;
    Fd& operator=(const Fd&) = delete;
};
inline bool valid(const std::string& value) {
    return value.size() == 43 &&
           value.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-") == value.npos;
}
inline int directory(const std::string& root, bool create) {
    if (root.empty() || root.front() != '/') return -1;
    if (create) mkdir(root.c_str(), 0700); // The profile root already exists in practice.
    Fd parent(open(root.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC));
    if (parent.value < 0) return -1;
    if (create && mkdirat(parent.value, "device-session", 0700) != 0 && errno != EEXIST) return -1;
    int fd = openat(parent.value, "device-session", O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    struct stat info{};
    if (fd >= 0 && (fstat(fd, &info) != 0 || !S_ISDIR(info.st_mode) ||
                    !dkrr::native_metadata::same_user(info, geteuid()) || (info.st_mode & 0077))) {
        close(fd);
        return -1;
    }
    return fd;
}
} // namespace ps5_detail
inline bool available() { return true; }
inline bool store(const std::string& root, const std::string& value) {
    using namespace ps5_detail;
    if (!valid(value)) return false;
    Fd dir(directory(root, true));
    if (dir.value < 0) return false;
    static std::atomic<unsigned> sequence{0};
    const auto temporary = "new-" + std::to_string(getpid()) + "-" + std::to_string(++sequence);
    Fd fd(openat(dir.value, temporary.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600));
    if (fd.value < 0) return false;
    bool ok = true;
    std::size_t written = 0;
    while (ok && written < value.size()) {
        const auto count = write(fd.value, value.data() + written, value.size() - written);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) { ok = false; break; }
        written += static_cast<std::size_t>(count);
    }
    ok = ok && fsync(fd.value) == 0 &&
         renameat(dir.value, temporary.c_str(), dir.value, "device.token") == 0;
    if (ok) fsync(dir.value); // Best effort: the rename itself is the commit.
    else unlinkat(dir.value, temporary.c_str(), 0);
    return ok;
}
inline std::string load(const std::string& root) {
    using namespace ps5_detail;
    Fd dir(directory(root, false));
    if (dir.value < 0) return {};
    Fd fd(openat(dir.value, "device.token", O_RDONLY | O_NOFOLLOW | O_CLOEXEC | O_NONBLOCK));
    struct stat info{};
    if (fd.value < 0 || fstat(fd.value, &info) != 0 || !S_ISREG(info.st_mode) ||
        !dkrr::native_metadata::same_user(info, geteuid()) || (info.st_mode & 0077) ||
        info.st_nlink > 1 || info.st_size != 43)
        return {};
    std::string result(43, '\0');
    std::size_t received = 0;
    while (received < result.size()) {
        const auto count = read(fd.value, result.data() + received, result.size() - received);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) return {};
        received += static_cast<std::size_t>(count);
    }
    return valid(result) ? result : std::string{};
}
inline void clear(const std::string& root) {
    using namespace ps5_detail;
    Fd dir(directory(root, false));
    if (dir.value >= 0) { unlinkat(dir.value, "device.token", 0); fsync(dir.value); }
}
} // namespace dkr::online::keyring
