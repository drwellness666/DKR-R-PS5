// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cerrno>
#include <cstdint>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace dkrr::ps5_speaker {
// Generated clips are signed 16-bit mono, at most four seconds at 48 kHz.
// Inspect the opened inode: a path check before ifstream::open can race a
// replacement with a FIFO and prevent the joined worker from shutting down.
inline std::vector<std::int16_t> read_pcm(const char* path) {
    const int fd = open(path, O_RDONLY | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) return {};
    struct Close { int fd; ~Close() { close(fd); } } close_file{fd};
    struct stat info{};
    if (fstat(fd, &info) != 0 || !S_ISREG(info.st_mode) || info.st_size <= 0 ||
        info.st_size % 2 || info.st_size > 48000 * 2 * 4) return {};
    const auto bytes = static_cast<std::size_t>(info.st_size);
    std::vector<std::int16_t> samples(bytes / sizeof(std::int16_t));
    auto* data = reinterpret_cast<unsigned char*>(samples.data());
    for (std::size_t at = 0; at < bytes;) {
        const auto got = read(fd, data + at, bytes - at);
        if (got < 0 && errno == EINTR) continue;
        if (got <= 0 || static_cast<std::size_t>(got) > bytes - at) return {};
        at += static_cast<std::size_t>(got);
    }
    // A shrinking/growing file cannot be accepted as a complete generated clip.
    if (fstat(fd, &info) != 0 || info.st_size != static_cast<off_t>(bytes)) return {};
    return samples;
}
}
