/* SPDX-License-Identifier: GPL-3.0-or-later */
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
struct dkrr_ps5_revision_state {
 unsigned active, preferred, compiled_mask, available_mask;
};
unsigned dkrr_ps5_initial_revision(void);
void dkrr_ps5_set_active_revision(unsigned revision);
unsigned dkrr_ps5_active_revision(void);
struct dkrr_ps5_revision_state dkrr_ps5_revision_state(void);
/* Returns zero after a verified preference write. Never changes a save or
 * current process/game revision; the choice takes effect on the next launch. */
int dkrr_ps5_choose_revision(unsigned revision);
#ifdef __cplusplus
}
#endif
