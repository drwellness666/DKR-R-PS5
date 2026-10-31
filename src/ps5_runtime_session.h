/* SPDX-License-Identifier: GPL-3.0-or-later */
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
/* Worker lifetime, not the guest's sticky None/Running/Quit launch gate. */
int dkrr_native_runtime_active(void);
#ifdef __cplusplus
}
#endif
