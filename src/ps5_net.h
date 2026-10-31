/* SPDX-License-Identifier: GPL-3.0-or-later */
#pragma once
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <stdint.h>
#include <ifaddrs.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Native socket IDs are not kernel file descriptors. Keep this API confined
 * to DNS, HTTPS and RTC; file, audio and GPU descriptors keep their normal ABI. */
#define DKRR_NET_DECL(name) extern __typeof__(name) dkrr_net_##name
DKRR_NET_DECL(socket); DKRR_NET_DECL(bind); DKRR_NET_DECL(connect);
DKRR_NET_DECL(listen); DKRR_NET_DECL(accept); DKRR_NET_DECL(shutdown);
DKRR_NET_DECL(getsockopt); DKRR_NET_DECL(setsockopt);
DKRR_NET_DECL(getsockname); DKRR_NET_DECL(getpeername);
DKRR_NET_DECL(send); DKRR_NET_DECL(sendto);
DKRR_NET_DECL(recv); DKRR_NET_DECL(recvfrom);
DKRR_NET_DECL(recvmsg); DKRR_NET_DECL(sendmsg);
DKRR_NET_DECL(getifaddrs); DKRR_NET_DECL(freeifaddrs);
DKRR_NET_DECL(close); DKRR_NET_DECL(fcntl); DKRR_NET_DECL(ioctl);
DKRR_NET_DECL(poll); DKRR_NET_DECL(select);
#undef DKRR_NET_DECL
int dkrr_net_wakeup_pair(int pair[2]);
int dkrr_net_init_result(void);
struct dkrr_net_counters { uint64_t opened, closed, close_errors, polls, epolls_opened, epolls_closed; };
struct dkrr_net_counters dkrr_net_counters_read(void);
int dkrr_net_diagnostic(char *out,size_t bytes);
#ifdef __cplusplus
}
#endif
