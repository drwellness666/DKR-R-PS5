/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Keep SLJIT's original pooled allocator and locking. Only its system chunks
 * use the SDK's PS5 executable direct-memory owner: map RW, then enable
 * execute, outside the GPU range. Never request executable mmap at map time. */
#include <ps5platform/exec.h>
static SLJIT_INLINE void* alloc_chunk(sljit_uw bytes)
{
    return ps5_exec_allocate((size_t)bytes,(uintptr_t)&sljit_generate_code);
}
static SLJIT_INLINE void free_chunk(void *chunk,sljit_uw bytes)
{
    SLJIT_UNUSED_ARG(bytes);
    (void)ps5_exec_release(chunk);
}
#include "allocator_src/sljitExecAllocatorCore.c"
