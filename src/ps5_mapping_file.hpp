// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <array>
#include <cerrno>
#include <cstdint>
#include <fcntl.h>
#include <filesystem>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
namespace dkrr::ps5_mappings {
inline constexpr std::uint64_t maximum_bytes=16ULL*1024*1024;
inline constexpr std::size_t maximum_line=64*1024, maximum_lines=65536;
// Bound external input, avoid blocking FIFO opens and read only regular files.
// A read error discards the whole import instead of applying a readable prefix.
inline std::vector<std::string> read(const std::filesystem::path& path,
                                     std::string* error=nullptr) {
    if(error)error->clear();
    const auto fail=[&](const char* message) {
        if(error)*error=message;
        return std::vector<std::string>{};
    };
    struct File {int fd;~File(){if(fd>=0)::close(fd);}} file{
        ::open(path.c_str(),O_RDONLY|O_NOFOLLOW|O_NONBLOCK)};
    if(file.fd<0)return fail("Could not open that controller mapping file.");
    struct stat info{};
    if(::fstat(file.fd,&info)<0||!S_ISREG(info.st_mode)||info.st_size<0)
        return fail("Choose an ordinary controller mapping file.");
    if(static_cast<std::uint64_t>(info.st_size)>maximum_bytes)
        return fail("That controller mapping file is too large (limit: 16 MiB).");
    std::vector<std::string> lines;std::string line;
    std::uint64_t bytes=0;std::size_t count=0;std::array<char,8192> buffer{};
    const auto finish_line=[&] {
        if(++count>maximum_lines)return false;
        if(!line.empty()&&line.back()=='\r')line.pop_back();
        if(!line.empty()&&line.front()!='#')lines.push_back(line);
        line.clear();return true;
    };
    for(;;) {
        const auto size=::read(file.fd,buffer.data(),buffer.size());
        if(size<0){if(errno==EINTR)continue;return fail("The mapping file could not be fully read. Try again.");}
        if(!size)break;
        if(static_cast<std::uint64_t>(size)>maximum_bytes-bytes)
            return fail("That controller mapping file is too large (limit: 16 MiB).");
        bytes+=static_cast<std::uint64_t>(size);
        for(ssize_t i=0;i<size;++i) {
            const char value=buffer[static_cast<std::size_t>(i)];
            if(!value)return fail("Use a plain-text controller mapping file.");
            if(value=='\n') {
                if(!finish_line())return fail("That controller mapping file has too many lines.");
            }else {
                if(line.size()==maximum_line)return fail("A line in that mapping file is too long.");
                line.push_back(value);
            }
        }
    }
    if(!line.empty()&&!finish_line())return fail("That controller mapping file has too many lines.");
    const int fd=file.fd;file.fd=-1;
    if(::close(fd)<0)return fail("The mapping file could not be closed cleanly. Try again.");
    return lines;
}
}
