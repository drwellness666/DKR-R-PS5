/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* The native title has no getnameinfo import. All RTC callers request numeric
 * addresses and ports. The matching getaddrinfo/freeaddrinfo adapter is in
 * ps5_dns.c; the renderer SDK's always-failing DNS placeholder is excluded. */
int __wrap_getnameinfo(const struct sockaddr *address, socklen_t length,
                      char *host, socklen_t host_bytes,
                      char *service, socklen_t service_bytes, int flags) {
    const int host_requested=host&&host_bytes;
    const int service_requested=service&&service_bytes;
    if(flags&~(NI_NUMERICHOST|NI_NUMERICSERV|NI_DGRAM))return EAI_BADFLAGS;
    if(!host_requested&&!service_requested)return EAI_NONAME;
    if((host_requested&&!(flags&NI_NUMERICHOST))||
       (service_requested&&!(flags&NI_NUMERICSERV)))return EAI_NONAME;
    if(!address||length<offsetof(struct sockaddr,sa_family)+sizeof(address->sa_family))return EAI_FAMILY;
    char numeric_host[INET6_ADDRSTRLEN+12]={0};
    char numeric_service[6]={0};
    const void *source=NULL;unsigned port=0;uint32_t scope=0;
    if(address->sa_family==AF_INET&&length>=sizeof(struct sockaddr_in)) {
        const struct sockaddr_in *ipv4=(const struct sockaddr_in *)address;
        source=&ipv4->sin_addr;port=ntohs(ipv4->sin_port);
    } else if(address->sa_family==AF_INET6&&length>=sizeof(struct sockaddr_in6)) {
        const struct sockaddr_in6 *ipv6=(const struct sockaddr_in6 *)address;
        source=&ipv6->sin6_addr;port=ntohs(ipv6->sin6_port);scope=ipv6->sin6_scope_id;
    } else return EAI_FAMILY;
    if(host_requested) {
        if(!inet_ntop(address->sa_family,source,numeric_host,sizeof(numeric_host)))return EAI_SYSTEM;
        if(scope) {
            const size_t used=strlen(numeric_host);
            const int added=snprintf(numeric_host+used,sizeof(numeric_host)-used,"%%%u",(unsigned)scope);
            if(added<0||(size_t)added>=sizeof(numeric_host)-used)return EAI_OVERFLOW;
        }
        if(strlen(numeric_host)+1>host_bytes)return EAI_OVERFLOW;
    }
    if(service_requested) {
        const int count=snprintf(numeric_service,sizeof(numeric_service),"%u",port);
        if(count<0||(size_t)count>=sizeof(numeric_service)||(size_t)count+1>service_bytes)return EAI_OVERFLOW;
    }
    /* Validate both outputs before modifying either caller buffer. */
    if(host_requested)memcpy(host,numeric_host,strlen(numeric_host)+1);
    if(service_requested)memcpy(service,numeric_service,strlen(numeric_service)+1);
    return 0;
}
