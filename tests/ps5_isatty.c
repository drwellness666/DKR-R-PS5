/* SPDX-License-Identifier: GPL-3.0-or-later */
#define _XOPEN_SOURCE 600
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pty.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int __wrap_isatty(int);
static void compare(int descriptor,int expected) {
    errno=0;const int actual=__wrap_isatty(descriptor),actual_errno=errno;
    errno=0;const int system=isatty(descriptor),system_errno=errno;
    assert(actual==expected&&actual==system);
    if(!actual)assert(actual_errno==system_errno);
}
int main(void) {
    compare(-1,0);
    int pipes[2];assert(!pipe(pipes));compare(pipes[0],0);compare(pipes[1],0);close(pipes[0]);close(pipes[1]);
    char name[]="/tmp/dkrr-isatty-XXXXXX";int file=mkstemp(name);assert(file>=0);assert(!unlink(name));compare(file,0);assert(!close(file));compare(file,0);
    int master,slave;assert(!openpty(&master,&slave,NULL,NULL,NULL));compare(master,1);compare(slave,1);close(master);close(slave);
    puts("PASS terminal detection versus libc: real PTY master/slave, regular file, both pipe ends, invalid/closed descriptors and preserved error semantics");
}
