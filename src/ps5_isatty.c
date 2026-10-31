/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <sys/types.h>
#include <sys/ioctl.h>
#include <termios.h>

/* PLog checks whether its output supports terminal colours. Native title
 * stdout/stderr are files or pipes; query the descriptor through the existing
 * native ioctl export instead of calling the absent libc isatty import. */
int __wrap_isatty(int descriptor) {
    struct termios attributes;
#if defined(TIOCGETA)
    return ioctl(descriptor,TIOCGETA,&attributes)==0;
#elif defined(TCGETS)
    return ioctl(descriptor,TCGETS,&attributes)==0;
#else
#error "No supported terminal-attribute request in the platform headers"
#endif
}
