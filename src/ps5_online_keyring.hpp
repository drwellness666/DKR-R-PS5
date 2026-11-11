// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <atomic>
#include <cerrno>
#include <cstdio>
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
// Plain path calls (mkdir/open/rename) are used on purpose: the *at() family
// is not reliably available to a native title and silently failed in 1.2.3.
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
// The private directory <root>/device-session, or "" when it is not usable.
inline std::string directory(const std::string& root, bool create) {
    if (root.empty() || root.front() != '/') return {};
    const std::string path = root + "/device-session";
    if (create && mkdir(path.c_str(), 0700) != 0 && errno != EEXIST) {
        std::fprintf(stderr, "[ps5][keyring] mkdir failed errno=%d path=%s\n", errno, path.c_str());
        return {};
    }
    struct stat info{};
    if (lstat(path.c_str(), &info) != 0) {
        if (create) std::fprintf(stderr, "[ps5][keyring] lstat failed errno=%d path=%s\n", errno, path.c_str());
        return {};
    }
    // The mode requested at creation can be widened by the volume; tighten it
    // once, on our own directory, before judging it.
    if (create && S_ISDIR(info.st_mode) && (info.st_mode & 0077)) {
        chmod(path.c_str(), 0700);
        lstat(path.c_str(), &info);
    }
    if (!S_ISDIR(info.st_mode) || !dkrr::native_metadata::same_user(info, geteuid()) || (info.st_mode & 0077)) {
        std::fprintf(stderr, "[ps5][keyring] directory refused uid=%u euid=%u mode=%o nlink=%u\n",
                     unsigned(info.st_uid), unsigned(geteuid()), unsigned(info.st_mode), unsigned(info.st_nlink));
        return {};
    }
    return path;
}
} // namespace ps5_detail
inline bool available() { return true; }
inline bool store(const std::string& root, const std::string& value) {
    using namespace ps5_detail;
    if (!valid(value)) return false;
    const std::string dir = directory(root, true);
    if (dir.empty()) return false;
    static std::atomic<unsigned> sequence{0};
    const std::string temporary = dir + "/new-" + std::to_string(getpid()) + "-" + std::to_string(++sequence);
    Fd fd(open(temporary.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600));
    if (fd.value < 0) { std::fprintf(stderr, "[ps5][keyring] token create failed errno=%d\n", errno); return false; }
    bool ok = true;
    std::size_t written = 0;
    while (ok && written < value.size()) {
        const auto count = write(fd.value, value.data() + written, value.size() - written);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) { ok = false; break; }
        written += static_cast<std::size_t>(count);
    }
    // The rename is the commit; a refused flush on the console's data volume
    // must not turn a stored token into "could not be saved".
    if (ok && fsync(fd.value) != 0) std::fprintf(stderr, "[ps5][keyring] fsync ignored errno=%d\n", errno);
    ok = ok && rename(temporary.c_str(), (dir + "/device.token").c_str()) == 0;
    if (!ok) { std::fprintf(stderr, "[ps5][keyring] token write failed errno=%d\n", errno); unlink(temporary.c_str()); }
    return ok;
}
inline std::string load(const std::string& root) {
    using namespace ps5_detail;
    const std::string dir = directory(root, false);
    if (dir.empty()) return {};
    Fd fd(open((dir + "/device.token").c_str(), O_RDONLY | O_NOFOLLOW | O_CLOEXEC | O_NONBLOCK));
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
    const std::string dir = directory(root, false);
    if (!dir.empty()) unlink((dir + "/device.token").c_str());
}
} // namespace dkr::online::keyring
