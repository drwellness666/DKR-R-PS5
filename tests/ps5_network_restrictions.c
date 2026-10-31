/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <sys/ioctl.h>
#include <unistd.h>
static atomic_int setters, ioctls, failure, pipe_failure;
static int last_pipe[2] = {-1, -1};
int __real_fcntl(int, int, ...);
int __real_ioctl(int, unsigned long, ...);
int __real_pipe(int [2]);
int __wrap_fcntl(int fd, int command, ...) {
    if (command == F_SETFL) {
        atomic_fetch_add(&setters, 1);
        errno = EACCES;
        return -1;
    }
    if (command == F_GETFL || command == F_GETFD) return __real_fcntl(fd, command);
    assert(command == F_SETFD || command == F_DUPFD || command == F_DUPFD_CLOEXEC);
    va_list args;
    va_start(args, command);
    const int value = va_arg(args, int);
    va_end(args);
    return __real_fcntl(fd, command, value);
}
int __wrap_ioctl(int fd, unsigned long command, ...) {
    va_list args;
    va_start(args, command);
    void *value = va_arg(args, void *);
    va_end(args);
    if (command == FIONBIO) {
        int n = atomic_fetch_add(&ioctls, 1) + 1;
        if (n == atomic_load(&failure)) { errno = EACCES; return -1; }
    }
    return __real_ioctl(fd, command, value);
}
int __wrap_pipe(int fds[2]) {
    if (atomic_load(&pipe_failure)) { errno = EMFILE; return -1; }
    int rc = __real_pipe(fds);
    if (!rc) { last_pipe[0] = fds[0]; last_pipe[1] = fds[1]; }
    return rc;
}
void dkrr_test_reset_network(int fail_at, int fail_pipe) {
    atomic_store(&setters, 0); atomic_store(&ioctls, 0);
    atomic_store(&failure, fail_at); atomic_store(&pipe_failure, fail_pipe);
}
void dkrr_test_pipe_closed(void) {
    for (int i = 0; i < 2; ++i) {
        assert(__real_fcntl(last_pipe[i], F_GETFD) == -1 && errno == EBADF);
    }
}
void dkrr_test_nonblocking_checked(void) {
    assert(!atomic_load(&setters));
    assert(atomic_load(&ioctls) > 0);
}
