/* SPDX-License-Identifier: GPL-3.0-or-later
 * Adapt only N64ModernRuntime's exact 4 GiB arena / 512 MiB RDRAM request.
 * Reserve virtual address space, commit direct memory and retain inaccessible
 * guard space. Every other mapping is forwarded unchanged. No guest addresses
 * or allocator limits are reduced, and newly committed RAM is explicitly zero.
 */
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#ifdef DKR_RDRAM_HOST_TEST
#include "rdram_host_shm.h"
#else
#include <ps5platform/shm.h>
#endif
#ifndef DKRR_RDRAM_RESERVE_BYTES
#define DKRR_RDRAM_RESERVE_BYTES (4096ULL*1024*1024)
#endif
#ifndef DKRR_RDRAM_COMMIT_BYTES
#define DKRR_RDRAM_COMMIT_BYTES (512ULL*1024*1024)
#endif
void *__real_mmap(void *,size_t,int,int,int,off_t);
int __real_mprotect(void *,size_t,int);
int __real_munmap(void *,size_t);
static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
static void *arena;
static struct ps5_shm memory;
static int committed;
static int failure(const char *stage,int result) {
    fprintf(stderr,"[ps5-rdram] %s rc=0x%08x\n",stage,(unsigned)result);
    errno=((unsigned)result&0xffff0000U)==0x80020000U?result&0xffff:ENOMEM;
    return -1;
}
void *__wrap_mmap(void *address,size_t bytes,int protection,int flags,int fd,off_t offset) {
    if(address || bytes!=DKRR_RDRAM_RESERVE_BYTES || protection!=PROT_NONE ||
       flags!=(MAP_ANON|MAP_PRIVATE) || fd!=-1 || offset!=0)
        return __real_mmap(address,bytes,protection,flags,fd,offset);
    pthread_mutex_lock(&lock);
    if(arena) { pthread_mutex_unlock(&lock);errno=EBUSY;return MAP_FAILED; }
    int rc=ps5_vrange_reserve(bytes,NULL,0x10000,&arena);
    if(rc) { arena=NULL;pthread_mutex_unlock(&lock);failure("reserve",rc);return MAP_FAILED; }
    void *result=arena;
    fprintf(stderr,"[ps5-rdram] reserved bytes=%zu\n",bytes);
    pthread_mutex_unlock(&lock);return result;
}
int __wrap_mprotect(void *address,size_t bytes,int protection) {
    pthread_mutex_lock(&lock);
    if(!arena || address!=arena) {
        pthread_mutex_unlock(&lock);return __real_mprotect(address,bytes,protection);
    }
    if(bytes!=DKRR_RDRAM_COMMIT_BYTES || protection!=(PROT_READ|PROT_WRITE)) {
        pthread_mutex_unlock(&lock);errno=EINVAL;return -1;
    }
    if(committed) { pthread_mutex_unlock(&lock);return 0; }
    int rc=ps5_shm_create(bytes,&memory);
    if(rc) { pthread_mutex_unlock(&lock);return failure("allocate-direct",rc); }
    void *view=NULL;
    rc=ps5_shm_map(&memory,0,bytes,arena,PS5_SHM_READ|PS5_SHM_WRITE,PS5_SHM_FIXED,&view);
    if(rc || view!=arena) {
        if(!rc)ps5_shm_unmap(view,bytes,0);
        ps5_shm_destroy(&memory);
        pthread_mutex_unlock(&lock);return failure("map-direct",rc?rc:PS5_SHM_NO_PLACE);
    }
    memset(arena,0,bytes);
    committed=1;
    fprintf(stderr,"[ps5-rdram] committed and zeroed bytes=%zu guard-bytes=%llu\n",bytes,
            (unsigned long long)(DKRR_RDRAM_RESERVE_BYTES-DKRR_RDRAM_COMMIT_BYTES));
    pthread_mutex_unlock(&lock);return 0;
}
int __wrap_munmap(void *address,size_t bytes) {
    pthread_mutex_lock(&lock);
    if(!arena || address!=arena) {
        pthread_mutex_unlock(&lock);return __real_munmap(address,bytes);
    }
    if(bytes!=DKRR_RDRAM_RESERVE_BYTES) {
        pthread_mutex_unlock(&lock);errno=EINVAL;return -1;
    }
    int rc;
    if(committed) {
        rc=ps5_shm_unmap(arena,DKRR_RDRAM_COMMIT_BYTES,PS5_SHM_KEEP_RESERVED);
        if(rc) { pthread_mutex_unlock(&lock);return failure("unmap-direct",rc); }
        committed=0;ps5_shm_destroy(&memory);
    }
    rc=ps5_vrange_release(arena,bytes);
    if(!rc) { arena=NULL;fprintf(stderr,"[ps5-rdram] released arena and direct memory\n"); }
    pthread_mutex_unlock(&lock);
    return rc?failure("release-arena",rc):0;
}
