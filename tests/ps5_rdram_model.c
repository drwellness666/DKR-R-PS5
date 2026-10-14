/* SPDX-License-Identifier: GPL-3.0-or-later
 * Exercise zeroing, guards, forwarding and cleanup with actual host mappings.
 * Smaller byte constants reduce resource cost; the production branch and
 * lifecycle are identical. Hardware allocation/pool limits remain separate.
 */
#include "rdram_host_shm.h"
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
static int objects,views,ranges,fail,forwarded;
static unsigned char *backing;
void *__wrap_mmap(void *,size_t,int,int,int,off_t);
int __wrap_mprotect(void *,size_t,int);
int __wrap_munmap(void *,size_t);
void *__real_mmap(void *a,size_t b,int p,int f,int d,off_t o) {++forwarded;return mmap(a,b,p,f,d,o);}
int __real_mprotect(void *a,size_t b,int p) {++forwarded;return mprotect(a,b,p);}
int __real_munmap(void *a,size_t b) {++forwarded;return munmap(a,b);}
int ps5_vrange_reserve(size_t b,void *hint,size_t align,void **result) {
    assert(!hint && align==65536);
    *result=mmap(NULL,b,PROT_NONE,MAP_ANON|MAP_PRIVATE,-1,0);
    if(*result==MAP_FAILED)return PS5_SHM_NO_PLACE;
    ++ranges;return 0;
}
int ps5_vrange_release(void *a,size_t b) {
    if(fail==4)return PS5_SHM_NO_PLACE;
    assert(!munmap(a,b));--ranges;return 0;
}
int ps5_shm_create(size_t b,struct ps5_shm *s) {
    if(fail==1)return (int)0x8002000c;
    s->bytes=b;s->direct_start=1;
    backing=malloc(b);assert(backing);memset(backing,0xa5,b);++objects;return 0;
}
void ps5_shm_destroy(struct ps5_shm *s) {free(backing);backing=NULL;memset(s,0,sizeof(*s));--objects;}
int ps5_shm_map(const struct ps5_shm *s,size_t offset,size_t b,void *a,int p,unsigned f,void **out) {
    assert(offset==0 && b==s->bytes && p==(PS5_SHM_READ|PS5_SHM_WRITE) && f==PS5_SHM_FIXED);
    if(fail==2)return PS5_SHM_NO_PLACE;
    assert(!mprotect(a,b,PROT_READ|PROT_WRITE));memcpy(a,backing,b);*out=a;++views;return 0;
}
int ps5_shm_unmap(void *a,size_t b,unsigned f) {
    assert(f==PS5_SHM_KEEP_RESERVED);
    if(fail==3)return PS5_SHM_NO_PLACE;
    assert(!mprotect(a,b,PROT_NONE));--views;return 0;
}
static void *reserve(void) {
    void *a=__wrap_mmap(NULL,DKRR_RDRAM_RESERVE_BYTES,PROT_NONE,MAP_ANON|MAP_PRIVATE,-1,0);
    assert(a!=MAP_FAILED);return a;
}
static void clean(void) {assert(objects==0 && views==0 && ranges==0 && backing==NULL);}
int main(void) {
    struct rlimit core={0,0};assert(!setrlimit(RLIMIT_CORE,&core));
    void *a=reserve();
    errno=0;assert(__wrap_mmap(NULL,DKRR_RDRAM_RESERVE_BYTES,PROT_NONE,MAP_ANON|MAP_PRIVATE,-1,0)==MAP_FAILED && errno==EBUSY);
    assert(__wrap_mprotect(a,DKRR_RDRAM_COMMIT_BYTES/2,PROT_READ|PROT_WRITE)==-1 && errno==EINVAL);
    assert(!__wrap_mprotect(a,DKRR_RDRAM_COMMIT_BYTES,PROT_READ|PROT_WRITE));
    for(size_t i=0;i<DKRR_RDRAM_COMMIT_BYTES;++i)assert(((unsigned char *)a)[i]==0);
    pid_t child=fork();assert(child>=0);
    if(!child) {_exit(*(volatile unsigned char *)((unsigned char *)a+DKRR_RDRAM_COMMIT_BYTES));}
    int status;assert(waitpid(child,&status,0)==child && WIFSIGNALED(status) && WTERMSIG(status)==SIGSEGV);
    assert(!__wrap_mprotect(a,DKRR_RDRAM_COMMIT_BYTES,PROT_READ|PROT_WRITE));
    assert(objects==1 && views==1 && ranges==1);
    assert(!__wrap_munmap(a,DKRR_RDRAM_RESERVE_BYTES));clean();
    for(int injected=1;injected<=2;++injected) {
        a=reserve();fail=injected;
        assert(__wrap_mprotect(a,DKRR_RDRAM_COMMIT_BYTES,PROT_READ|PROT_WRITE)==-1);
        fail=0;assert(!__wrap_munmap(a,DKRR_RDRAM_RESERVE_BYTES));clean();
    }
    for(int injected=3;injected<=4;++injected) {
        a=reserve();assert(!__wrap_mprotect(a,DKRR_RDRAM_COMMIT_BYTES,PROT_READ|PROT_WRITE));fail=injected;
        assert(__wrap_munmap(a,DKRR_RDRAM_RESERVE_BYTES)==-1);
        fail=0;assert(!__wrap_munmap(a,DKRR_RDRAM_RESERVE_BYTES));clean();
    }
    a=__wrap_mmap(NULL,16384,PROT_READ|PROT_WRITE,MAP_ANON|MAP_PRIVATE,-1,0);assert(a!=MAP_FAILED);
    assert(!__wrap_mprotect(a,16384,PROT_READ));assert(!__wrap_munmap(a,16384));assert(forwarded==3);clean();
    puts("PS5 RDRAM host model PASS: zeroed commit, inaccessible guards, single arena, failure cleanup/retry, unrelated mapping forwarding");
}
