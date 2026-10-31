// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <filesystem>
#include <sys/stat.h>
#include <unistd.h>
namespace dkrr::ps5_store {
inline bool own_lease_path(const std::filesystem::path& path) {
    if(path != "/data/dkrr/profile-us-v77/mod-store/install.lock" &&
       path != "/data/dkrr/profile-us-v80/mod-store/install.lock") return false;
    for(auto parent=path.parent_path();!parent.empty();parent=parent.parent_path()) {
        struct stat info{};
        if(lstat(parent.c_str(),&info)!=0 || !S_ISDIR(info.st_mode))return false;
        if(parent==parent.root_path())break;
    }
    return true;
}
// Native title metadata may redact nlink and uid. This exception is only for our empty lease
// inode, which is never truncated or used as content. Keep aliases, replaced
// entries, links, foreign owners and all other paths refused.
inline bool own_lease_metadata(const std::filesystem::path& path, const struct stat& file,
                               const struct stat& entry, uid_t owner) {
    const bool own_path = path == "/data/dkrr/profile-us-v77/mod-store/install.lock" ||
                          path == "/data/dkrr/profile-us-v80/mod-store/install.lock";
    return own_path && S_ISREG(file.st_mode) && S_ISREG(entry.st_mode) &&
        file.st_nlink == 0 && entry.st_nlink == 0 && file.st_size == 0 && entry.st_size == 0 &&
        (file.st_mode&0777)==0600 && (entry.st_mode&0777)==0600 &&
        file.st_uid==entry.st_uid &&
        (file.st_uid==owner || (owner!=0 && file.st_uid==0)) &&
        file.st_dev != 0 && file.st_ino != 0 &&
        file.st_dev == entry.st_dev && file.st_ino == entry.st_ino;
}
}
