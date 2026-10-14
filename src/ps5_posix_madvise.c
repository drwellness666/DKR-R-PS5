/* SPDX-License-Identifier: GPL-3.0-or-later
 * Optional advisory API, absent from public native imports. SLJIT ignores its
 * return value while shrinking a stack; normal unmap remains implemented.
 */
#include <errno.h>
#include <stddef.h>
#include <sys/mman.h>
__attribute__((visibility("hidden"))) int posix_madvise(void *address, size_t bytes, int advice) {
    (void)address;(void)bytes;
    // Return an error number directly, as POSIX requires; never change errno.
    switch(advice) {
        case POSIX_MADV_NORMAL: return 0;
        case POSIX_MADV_RANDOM: case POSIX_MADV_SEQUENTIAL:
        case POSIX_MADV_WILLNEED: case POSIX_MADV_DONTNEED: return ENOTSUP;
        default: return EINVAL;
    }
}
