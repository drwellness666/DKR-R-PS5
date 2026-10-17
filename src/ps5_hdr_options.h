/* SPDX-License-Identifier: GPL-3.0-or-later */
#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
struct dkrr_ps5_hdr_choice { unsigned enabled, paper_white, peak, expansion; };
/* output: 0 SDR, 1 HDR buffer awaiting HDMI check, 2 confirmed HDR, 3 SDR fallback. */
struct dkrr_ps5_hdr_status { unsigned output, reason; };
int dkrr_ps5_read_hdr_choice(struct dkrr_ps5_hdr_choice *);
int dkrr_ps5_choose_hdr(struct dkrr_ps5_hdr_choice);
void dkrr_ps5_initialize_hdr(void);
unsigned dkrr_ps5_hdr_requested(void);
struct dkrr_ps5_hdr_choice dkrr_ps5_hdr_calibration(void);
struct dkrr_ps5_hdr_status dkrr_ps5_hdr_status(void);
void dkrr_ps5_hdr_report(unsigned output, unsigned reason);
void dkrr_ps5_hdr_preview(struct dkrr_ps5_hdr_choice);
void dkrr_ps5_hdr_calibration_pattern(unsigned enabled);
unsigned dkrr_ps5_hdr_pattern(void);
/* Native VideoOut output status; -1 unavailable, 0 SDR, 1 HDR. */
int dkrr_ps5_hdr_probe_native(void);
#ifdef __cplusplus
}
#endif
