/* SPDX-License-Identifier: GPL-3.0-or-later */
#pragma once
#include <stdint.h>
#include <sys/statvfs.h>
/* Some native libc variants do not expose statvfs. Missing/invalid telemetry
 * is unknown, never a fabricated zero-space result or an installation promise. */
#pragma weak statvfs
static inline int dkrr_space_check(const char* path,uint64_t required,uint64_t* available) {
    struct statvfs fs;
    *available=0;
    if(!statvfs||statvfs(path,&fs))return 0;
    const uint64_t unit=fs.f_frsize?fs.f_frsize:fs.f_bsize;
    if(unit<512||unit>16U*1024U*1024U)return 0;
    *available=fs.f_bavail>UINT64_MAX/unit?UINT64_MAX:(uint64_t)fs.f_bavail*unit;
    return *available>=required?1:-1;
}
