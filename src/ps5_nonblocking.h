/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef DKRR_PS5_NONBLOCKING_H
#define DKRR_PS5_NONBLOCKING_H
#include <errno.h>
#include <sys/ioctl.h>
#include <unistd.h>

/* Native titles can reject F_SETFL. Never publish a blocking wakeup pipe:
 * the consumers drain until EAGAIN, and their producers must not block. */
static inline int dkrr_nonblocking_pipe(int fds[2]) {
    fds[0] = fds[1] = -1;
    if (pipe(fds) != 0) return -1;
    int enabled = 1;
    if (ioctl(fds[0], FIONBIO, &enabled) != 0 ||
        ioctl(fds[1], FIONBIO, &enabled) != 0) {
        const int error = errno;
        close(fds[0]);
        close(fds[1]);
        fds[0] = fds[1] = -1;
        errno = error;
        return -1;
    }
    return 0;
}
#endif
