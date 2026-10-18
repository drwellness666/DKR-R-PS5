/* SPDX-License-Identifier: GPL-3.0-or-later */
#pragma once
#include <stdint.h>
#include "ps5_hdr_options.h"
#ifdef __cplusplus
extern "C" {
#endif
struct dkrr_ps5_display_choice { unsigned resolution, refresh; };
struct dkrr_ps5_display_status {
    unsigned width, height, refresh_millihz, fallback, high_refresh_build;
};
/* Resolution is buffer height, not the TV's HDMI signal resolution.
 * Defaults: 1080p / 60 Hz. A 120 Hz choice requires an opt-in title build. */
int dkrr_ps5_read_display_choice(struct dkrr_ps5_display_choice *);
int dkrr_ps5_read_output(struct dkrr_ps5_display_choice *, struct dkrr_ps5_hdr_choice *);
int dkrr_ps5_choose_display(struct dkrr_ps5_display_choice);
/* One atomic next-launch record: output size/rate and HDR never split. */
int dkrr_ps5_choose_output(struct dkrr_ps5_display_choice, struct dkrr_ps5_hdr_choice);
void dkrr_ps5_initialize_display(void);
struct dkrr_ps5_display_choice dkrr_ps5_boot_display_choice(void);
struct dkrr_ps5_display_status dkrr_ps5_display_status(void);
void dkrr_ps5_report_display(unsigned width, unsigned height, unsigned refresh_millihz);
#ifdef __cplusplus
}
/* Accepted native surface status, never the saved request or SDL dummy mode.
 * Before surface creation (or an invalid status) retain the 60-Hz baseline. */
static inline int dkrr_ps5_presentation_display_rate(struct dkrr_ps5_display_status status) {
    if (!status.width || !status.height) return 60;
    if (status.refresh_millihz >= 118000 && status.refresh_millihz <= 121000) return 120;
    return 60;
}
/* Only ranks enumerated modes. Does not invent a display mode or HDMI timing. */
static inline uint64_t dkrr_ps5_display_score(unsigned width, unsigned height, unsigned refresh,
                                            dkrr_ps5_display_choice choice) {
    if (!width || !height || width > 8192 || height > 8192) return UINT64_MAX;
    const unsigned wanted_width = choice.resolution == 2160 ? 3840 : choice.resolution == 1440 ? 2560 : 1920;
    const bool size_match = width == wanted_width && height == choice.resolution;
    const bool rate_match = choice.refresh == 120 ? refresh >= 110000 && refresh <= 121000 : refresh <= 61000;
    const bool baseline_size = width == 1920 && height == 1080;
    // Requested size first; then 1080p, then the smallest other size.
    // On a missing 120 Hz mode the same size at 60 Hz wins.
    const uint64_t size_rank = size_match ? 0 : baseline_size ? 1 : 2;
    return (size_rank << 62) | (uint64_t(!rate_match) << 61) |
           (uint64_t(width) * height * 1000000 + refresh);
}
#endif
