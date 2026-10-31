/* SPDX-License-Identifier: GPL-3.0-or-later */
#pragma once
#include "ps5_net.h"
#if defined(__PROSPERO__) || defined(DKRR_TEST_NATIVE_NET)
#ifdef __cplusplus
extern "C" {
#endif
/* Symbol aliases, not token macros: std::bind and stream.close() stay intact.
 * This header is force-included only in networking dependency targets. */
#define DKRR_NET_ALIAS(name) extern __typeof__(name) name __asm__("dkrr_net_" #name)
DKRR_NET_ALIAS(socket); DKRR_NET_ALIAS(bind); DKRR_NET_ALIAS(connect);
DKRR_NET_ALIAS(listen); DKRR_NET_ALIAS(accept); DKRR_NET_ALIAS(shutdown);
DKRR_NET_ALIAS(getsockopt); DKRR_NET_ALIAS(setsockopt);
DKRR_NET_ALIAS(getsockname); DKRR_NET_ALIAS(getpeername);
DKRR_NET_ALIAS(send); DKRR_NET_ALIAS(sendto);
DKRR_NET_ALIAS(recv); DKRR_NET_ALIAS(recvfrom);
DKRR_NET_ALIAS(recvmsg); DKRR_NET_ALIAS(sendmsg);
DKRR_NET_ALIAS(getifaddrs); DKRR_NET_ALIAS(freeifaddrs);
DKRR_NET_ALIAS(close); DKRR_NET_ALIAS(fcntl); DKRR_NET_ALIAS(ioctl);
DKRR_NET_ALIAS(poll); DKRR_NET_ALIAS(select);
#undef DKRR_NET_ALIAS
#ifdef __cplusplus
}
#endif
#endif
