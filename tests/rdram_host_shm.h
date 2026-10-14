/* SPDX-License-Identifier: GPL-3.0-or-later
 * Host model declarations matching the public PS5 platform/shm.h interface.
 */
#pragma once
#include <stddef.h>
#include <stdint.h>
struct ps5_shm {int64_t direct_start;size_t bytes;};
#define PS5_SHM_READ 1
#define PS5_SHM_WRITE 2
#define PS5_SHM_FIXED 1U
#define PS5_SHM_KEEP_RESERVED 2U
#define PS5_SHM_NO_PLACE ((int)0x80ff0101)
int ps5_vrange_reserve(size_t,void *,size_t,void **);
int ps5_vrange_release(void *,size_t);
int ps5_shm_create(size_t,struct ps5_shm *);
void ps5_shm_destroy(struct ps5_shm *);
int ps5_shm_map(const struct ps5_shm *,size_t,size_t,void *,int,unsigned,void **);
int ps5_shm_unmap(void *,size_t,unsigned);
