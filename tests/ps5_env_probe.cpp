// SPDX-License-Identifier: GPL-3.0-or-later
// Prints what the standard library sees; tools/test_ps5_env_shim.py compares runs with and without each rule.
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
namespace fs = std::filesystem;
int main(int, char **argv) {
    const fs::path file = fs::path(argv[1]) / "probe.txt";
    int fd = open(file.c_str(), O_RDWR | O_CREAT, 0600);
    if (fd < 0) return 2;
    (void)!write(fd, "x", 1);
    std::error_code ec;
    (void)fs::read_symlink("/proc/self/exe", ec);
    std::printf("proc=%d\n", ec ? 0 : 1);
    struct stat st{};
    stat(file.c_str(), &st);
    std::printf("nlink=%u\n", unsigned(st.st_nlink));
    std::printf("hard_link_count=%ju\n", std::uintmax_t(fs::hard_link_count(file, ec)));
    struct stat fst{};
    fstat(fd, &fst);
    std::printf("uid_is_owner=%d\n", fst.st_uid == getuid() ? 1 : 0);
    std::printf("flock=%d\n", flock(fd, LOCK_EX | LOCK_NB) == 0 ? 1 : 0);
    fs::permissions(file, fs::perms::owner_all, fs::perm_options::replace, ec);
    std::printf("chmod=%d\n", ec ? 0 : 1);
    std::printf("fchmod=%d\n", fchmod(fd, 0600) == 0 ? 1 : 0);
    std::printf("fsync=%d\n", fsync(fd) == 0 ? 1 : 0);
    close(fd);
    return 0;
}
