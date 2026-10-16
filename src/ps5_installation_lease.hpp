// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
// Shared by both game revisions. The installer takes the exclusive lease;
// the native game holds this shared descriptor until all game I/O has stopped.
#ifndef DKRR_GAME_LEASE_PATH
#define DKRR_GAME_LEASE_PATH "/data/dkrr/profile-us-v77/.app-run.lock"
#endif
static inline int dkrr_runtime_installation_lease() {
    const char *path=DKRR_GAME_LEASE_PATH;
    int fd=open(path,O_RDONLY|O_NOFOLLOW);
    if(fd<0)fd=open(path,O_RDWR|O_CREAT|O_EXCL|O_NOFOLLOW,0666);
    if(fd<0)return -1;
    struct stat st{};
    if(fstat(fd,&st)<0||!S_ISREG(st.st_mode)||st.st_nlink>1||flock(fd,LOCK_SH|LOCK_NB)<0){close(fd);return -1;}
    return fd;
}
