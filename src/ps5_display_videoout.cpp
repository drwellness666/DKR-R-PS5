// SPDX-License-Identifier: GPL-3.0-or-later
// Keep the pinned WSI's high-refresh negotiation opt-in in this title only.
#include "ps5_display_options.h"
#include <ps5platform/videoout.h>
#include <cstdio>
extern "C" { int dkrr_ps5_hdr_videoout_handle=-1; }
extern "C" int __real_sceVideoOutIsOutputSupported(int32_t, uint32_t, const void *, const void *, const void *);
extern "C" int __real_sceVideoOutOpen(int32_t, int32_t, int32_t, const void *);
extern "C" int __wrap_sceVideoOutIsOutputSupported(int32_t handle, uint32_t mode,
                                                 const void *a, const void *b, const void *c) {
    if (mode == PS5_VIDEO_OUT_MODE_HIGH_FRAME_RATE && dkrr_ps5_boot_display_choice().refresh != 120) return 0;
    return __real_sceVideoOutIsOutputSupported(handle, mode, a, b, c);
}
extern "C" int __wrap_sceVideoOutOpen(int32_t user, int32_t bus, int32_t index, const void *parameter) {
    const int handle = __real_sceVideoOutOpen(user, bus, index, parameter);
    dkrr_ps5_hdr_videoout_handle=handle;
    // A metadata-capable test title can enter 120 Hz at launch. Explicitly
    // restore the qualified 60 Hz mode if the user has not opted in.
    if (handle >= 0 && dkrr_ps5_display_status().high_refresh_build && dkrr_ps5_boot_display_choice().refresh == 60) {
        const int result = sceVideoOutConfigureOutput(handle, PS5_VIDEO_OUT_MODE_RESTORE, nullptr, nullptr, nullptr);
        std::fprintf(stderr, "ps5-display: 60 Hz requested; restore rc=0x%08x\n", unsigned(result));
    }
    return handle;
}
