/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef DKRR_PS5_ASYNC_LOG_H
#define DKRR_PS5_ASYNC_LOG_H
#ifdef __cplusplus
extern "C" {
#endif
/* Borrow the already opened regular diagnostic FILE's descriptor. The
 * caller keeps it open until stop; the writer never closes or reopens it. */
int dkrr_async_log_start(int descriptor);
void dkrr_async_log_stop(void);
int dkrr_async_log_diagnostic(char *out,unsigned bytes);
#ifdef __cplusplus
}
#endif
#endif
