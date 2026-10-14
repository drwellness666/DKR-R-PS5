/* SPDX-License-Identifier: GPL-3.0-or-later
 * No video/audio initialization, mapping, kernel patches or file writes.
 * Allocate and immediately release each temporary direct-memory block.
 * app_info layout follows the public SDK's samples/ps/main.c.
 */
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct {
    uint32_t app_id;
    uint64_t unknown1;
    uint32_t app_type;
    char title_id[10];
    char unknown2[0x3c];
} app_info_t;

int sceKernelGetAppInfo(pid_t, app_info_t *);
int sceKernelAllocateMainDirectMemory(size_t, size_t, int, intptr_t *);
int sceKernelReleaseDirectMemory(intptr_t, size_t);

int main(void) {
    const size_t sizes[] = {0x20000, 4*1024*1024, 20*1024*1024, 64*1024*1024};
    app_info_t info = {0};
    int all_ok = 1;
    int rc = sceKernelGetAppInfo(getpid(), &info);
    printf("DKR-R PS5 MEMORY PROBE v1 - TERMINAL ONLY\n");
    printf("pid=%ld uid=%ld appinfo_rc=0x%08x", (long)getpid(),
           (long)getuid(), (unsigned)rc);
    if (rc == 0) printf(" app_id=0x%08x app_type=0x%08x title=%.10s",
                        info.app_id, info.app_type, info.title_id);
    printf("\nSCE return codes are authoritative; errno is supplemental.\n");
    fflush(stdout);
    for (size_t i=0; i<sizeof(sizes)/sizeof(sizes[0]); ++i) {
        intptr_t paddr = 0;
        errno = 0;
        rc = sceKernelAllocateMainDirectMemory(sizes[i], 0x20000, 3, &paddr);
        int saved_errno = errno;
        printf("alloc bytes=%zu type=3 align=131072 rc=0x%08x errno=%d (%s)\n",
               sizes[i], (unsigned)rc, saved_errno, strerror(saved_errno));
        fflush(stdout);
        if (rc != 0) {all_ok = 0; continue;}
        /* Physical address zero is valid: release every successful allocation. */
        errno = 0;
        rc = sceKernelReleaseDirectMemory(paddr, sizes[i]);
        saved_errno = errno;
        printf("release bytes=%zu rc=0x%08x errno=%d (%s)\n",
               sizes[i], (unsigned)rc, saved_errno, strerror(saved_errno));
        fflush(stdout);
        if (rc != 0) {
            printf("STOP: release failed; no further allocations attempted.\n");
            return 2;
        }
    }
    printf("MEMORY PROBE DONE: allocations=%s; video/audio/GPU untested\n",
           all_ok ? "PASS" : "PARTIAL_OR_FAIL");
    fflush(stdout);
    return all_ok ? 0 : 1;
}
