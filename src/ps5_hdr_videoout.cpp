// SPDX-License-Identifier: GPL-3.0-or-later
// Public 48-byte VideoOut status ABI documented by Kodi PS5 / ProsperoLight.
#include "ps5_hdr_options.h"
#include <atomic>
#include <cstddef>
#include <cstdint>
struct HdrOutputStatus {uint32_t resolution,range;uint64_t refresh,flags,reserved[3];};
static_assert(sizeof(HdrOutputStatus)==48&&offsetof(HdrOutputStatus,flags)==16);
extern "C" int sceVideoOutGetOutputStatus(int32_t,HdrOutputStatus*);
extern "C" int dkrr_ps5_hdr_videoout_handle;
extern "C" int dkrr_ps5_hdr_probe_native(){
    HdrOutputStatus s{};auto volatile query=sceVideoOutGetOutputStatus;
    if(!query||dkrr_ps5_hdr_videoout_handle<0||query(dkrr_ps5_hdr_videoout_handle,&s))return -1;
    return s.range==2||(s.flags&1)?1:0;
}
