/* SPDX-License-Identifier: GPL-3.0-or-later */
#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
enum dkrr_diag_stage { DKR_DIAG_ENV=1, DKR_DIAG_CTORS=2, DKR_DIAG_MAIN=3,
    DKR_DIAG_GAME=4, DKR_DIAG_PRESENT=5, DKR_DIAG_EXIT=6 };
struct dkrr_diag_crash {
    uint64_t signal, code, address, rip, rsp, trap, error, entry, stage;
};
struct dkrr_diag_capture {
    uint64_t intervals, elapsed_ns, max_gap_ns, over_33ms, over_50ms, over_100ms, dropped;
    unsigned active, complete;
};
size_t dkrr_diag_format_crash(char *out, size_t capacity, const char *build,
                             const struct dkrr_diag_crash *);
int dkrr_diag_initialize(const char *build, uintptr_t entry);
void dkrr_diag_stage(unsigned);
void dkrr_diag_note(const char *);
void dkrr_diag_shutdown(void);
const char *dkrr_diag_build(void);
int dkrr_diag_journal_ready(void);
enum dkrr_ui_phase { DKR_UI_IDLE=0, DKR_UI_PRESENCE=1, DKR_UI_ATTACH=2,
    DKR_UI_INPUT=3, DKR_UI_NEW_FRAME=4, DKR_UI_CONTENT=5,
    DKR_UI_DIALOGS=6, DKR_UI_END_FRAME=7, DKR_UI_FILE_PICKER=8,
    DKR_UI_TEXTURE_DIALOG=9, DKR_UI_MOD_DIALOG=10, DKR_UI_KEYBOARD=11,
    DKR_UI_NATIVE_RENDER=12, DKR_UI_LOG=13 };
/* Marks are memory/clock operations only. An internal observer writes a
 * bounded journal entry if a section remains blocked for 1.5 seconds. */
void dkrr_diag_ui_phase(unsigned phase);
/* Existing present-worker wait: 1 cursor, 2 workload, 3 interpolation,
 * 4 GPU fence, 5 frame pacer, 6 swap wait, 7 acquire, 8 present,
 * 9 frontend fence, 10 frontend images, 11 frontend submission; 0 complete. */
void dkrr_diag_render_wait(unsigned phase);
/* Renderer callback: memory only except the optional monotonic clock read;
 * never disk/log/network/GPU-wait operations or allocations. */
void dkrr_diag_present(void);
int dkrr_diag_capture_start(void);
struct dkrr_diag_capture dkrr_diag_capture_state(void);
int dkrr_diag_capture_finish(void);
/* Same recorder with an explicit monotonic timestamp for deterministic tests. */
void dkrr_diag_present_at(uint64_t now);
int dkrr_diag_capture_start_at(uint64_t now);
#ifdef __cplusplus
}
#endif
