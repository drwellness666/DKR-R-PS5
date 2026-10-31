/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <ares.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <poll.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdatomic.h>
#include <string.h>
#include <time.h>
#if defined(__PROSPERO__) || defined(DKRR_TEST_NATIVE_NET)
#include "ps5_net.h"
/* Use c-ares' supported native-I/O interface. SceNet IDs have no kernel
 * FD_CLOEXEC flag; invoking its default POSIX socket setup would reject them. */
static ares_socket_t dns_socket(int family,int type,int protocol,void *p){(void)p;return dkrr_net_socket(family,type|SOCK_NONBLOCK,protocol);}
static int dns_close(ares_socket_t fd,void *p){(void)p;return dkrr_net_close(fd);}
static int dns_connect(ares_socket_t fd,const struct sockaddr*a,ares_socklen_t n,unsigned flags,void *p){(void)p;if(flags){errno=ENOSYS;return -1;}return dkrr_net_connect(fd,a,n);}
static int dns_option(ares_socket_t fd,ares_socket_opt_t opt,const void*v,ares_socklen_t n,void *p){
    (void)p;int option;
    switch(opt){case ARES_SOCKET_OPT_SENDBUF_SIZE:option=SO_SNDBUF;break;
    case ARES_SOCKET_OPT_RECVBUF_SIZE:option=SO_RCVBUF;break;default:errno=ENOSYS;return -1;}
    return dkrr_net_setsockopt(fd,SOL_SOCKET,option,v,n);
}
static ares_ssize_t dns_recv(ares_socket_t fd,void*b,size_t n,int flags,struct sockaddr*a,ares_socklen_t*len,void*p){(void)p;return dkrr_net_recvfrom(fd,b,n,flags,a,len);}
static ares_ssize_t dns_send(ares_socket_t fd,const void*b,size_t n,int flags,const struct sockaddr*a,ares_socklen_t len,void*p){(void)p;return a?dkrr_net_sendto(fd,b,n,flags,a,len):dkrr_net_send(fd,b,n,flags);}
static int dns_name(ares_socket_t fd,struct sockaddr*a,ares_socklen_t*n,void*p){(void)p;return dkrr_net_getsockname(fd,a,n);}
static int dns_bind(ares_socket_t fd,unsigned flags,const struct sockaddr*a,socklen_t n,void*p){(void)p;(void)flags;return dkrr_net_bind(fd,a,n);}
static int native_channel(ares_channel_t *channel){
    const struct ares_socket_functions_ex io={.version=1,.flags=ARES_SOCKFUNC_FLAG_NONBLOCKING,
        .asocket=dns_socket,.aclose=dns_close,.asetsockopt=dns_option,.aconnect=dns_connect,
        .arecvfrom=dns_recv,.asendto=dns_send,.agetsockname=dns_name,.abind=dns_bind};
    return ares_set_socket_functions_ex(channel,&io,NULL);
}
#endif

extern int sceNetCtlInit(void) __attribute__((weak));
extern int sceNetCtlGetInfo(int,void *) __attribute__((weak));
extern int dkrr_secure_random(void *,size_t);
extern int dkrr_rtc_entropy_available(void);
void dkrr_dns_random(void *out,size_t bytes){if(dkrr_secure_random(out,bytes))abort();}
/* This adapter accepts numeric services only, and always passes
 * ARES_AI_NUMERICSERV. Keep c-ares' optional service-database path confined
 * here: a request outside that contract fails rather than calling a missing
 * WebKit export. This does not replace other game/system service lookups. */
struct servent *dkrr_dns_service_by_name(const char *name,const char *protocol){(void)name;(void)protocol;errno=ENOENT;return NULL;}
struct servent *dkrr_dns_service_by_port(int port,const char *protocol){(void)port;(void)protocol;errno=ENOENT;return NULL;}
enum { DNS_SOCKETS=16, DNS_DEADLINE_MS=4000 };
struct lookup { int done,status,overflow; struct ares_addrinfo *answer; struct pollfd sockets[DNS_SOCKETS]; };
static pthread_once_t once=PTHREAD_ONCE_INIT;
static int library_status=ARES_ENOTINITIALIZED;
static void initialize(void){if(dkrr_rtc_entropy_available())library_status=ares_library_init(ARES_LIB_INIT_ALL);}
static uint64_t milliseconds(void){struct timespec t;if(clock_gettime(CLOCK_MONOTONIC,&t))return 0;return (uint64_t)t.tv_sec*1000+(unsigned)t.tv_nsec/1000000;}
static int failed(const char *phase,int error,int family,int flags){
    static _Atomic uint64_t next_report;
    const uint64_t now=milliseconds();uint64_t next=atomic_load_explicit(&next_report,memory_order_relaxed);
    if(now>=next&&atomic_compare_exchange_strong_explicit(&next_report,&next,now+10000,memory_order_relaxed,memory_order_relaxed))
        fprintf(stderr,"[ps5][dns] failure phase=%s status=%d family=%d flags=%x errno=%d\n",phase,error,family,flags,errno);
    return error;
}
static void socket_state(void *opaque,ares_socket_t fd,int reading,int writing){
    struct lookup *l=opaque;int empty=-1;
    for(int i=0;i<DNS_SOCKETS;++i){
        if(l->sockets[i].fd==fd){l->sockets[i].fd=(reading||writing)?fd:-1;l->sockets[i].events=(reading?POLLIN:0)|(writing?POLLOUT:0);return;}
        if(l->sockets[i].fd<0)empty=i;
    }
    if(!reading&&!writing)return;
    if(empty<0){l->overflow=1;return;}
    l->sockets[empty]=(struct pollfd){fd,(short)((reading?POLLIN:0)|(writing?POLLOUT:0)),0};
}
static void completed(void *opaque,int status,int timeouts,struct ares_addrinfo *answer){
    (void)timeouts;struct lookup *l=opaque;l->status=status;l->answer=answer;l->done=1;
}
/* NetCtl's documented info union is 256 bytes; strings are copied only after
 * a successful call and a bounded terminator check. Never alter console DNS.
 * A title without NetCtl imports can still use the console's local resolver;
 * no external/public DNS address is substituted. */
static int servers(struct in_addr out[2]){
    int count=0;
    if(sceNetCtlGetInfo){
        if(sceNetCtlInit)(void)sceNetCtlInit();
        for(int id=17;id<=18;++id){unsigned char info[256]={0};struct in_addr ip;
            if(sceNetCtlGetInfo(id,info)==0&&memchr(info,0,sizeof(info))&&
               inet_pton(AF_INET,(const char*)info,&ip)==1&&ip.s_addr!=INADDR_ANY&&
               (!count||ip.s_addr!=out[0].s_addr))out[count++]=ip;
        }
    }
    if(!count){out[0].s_addr=htonl(INADDR_LOOPBACK);count=1;}
    return count;
}
static int configured_family(int family){
    struct ifaddrs *all=NULL;
    if(getifaddrs(&all))return family; /* Keep explicit DNS failure handling. */
    int v4=0,v6=0;
    for(const struct ifaddrs *p=all;p;p=p->ifa_next){
        if(!p->ifa_addr||!(p->ifa_flags&IFF_UP)||(p->ifa_flags&IFF_LOOPBACK))continue;
        v4|=p->ifa_addr->sa_family==AF_INET;v6|=p->ifa_addr->sa_family==AF_INET6;
    }
    freeifaddrs(all);
    /* A native title may receive no visible interfaces despite having a
     * working network stack. Empty enumeration is not proof that DNS has no
     * answer. Like a failed enumeration above, leave the family unspecified
     * and let the bounded resolver/connection attempt provide the result. */
    if(!v4&&!v6)return family;
    if((family==AF_INET&&!v4)||(family==AF_INET6&&!v6))return -1;
    return family==AF_UNSPEC?(v4?(v6?AF_UNSPEC:AF_INET):AF_INET6):family;
}
void __wrap_freeaddrinfo(struct addrinfo *p){while(p){struct addrinfo *next=p->ai_next;free(p->ai_addr);free(p->ai_canonname);free(p);p=next;}}
static int error_code(int status){
    switch(status){case ARES_SUCCESS:return 0;case ARES_ENOMEM:return EAI_MEMORY;
    case ARES_ETIMEOUT:case ARES_ESERVFAIL:case ARES_ECONNREFUSED:return EAI_AGAIN;
    case ARES_ENOTFOUND:case ARES_ENODATA:case ARES_EBADNAME:case ARES_ENONAME:return EAI_NONAME;
    case ARES_ESERVICE:return EAI_SERVICE;case ARES_EBADFAMILY:return EAI_FAMILY;
    default:return EAI_FAIL;}
}
int __wrap_getaddrinfo(const char *name,const char *service,const struct addrinfo *hints,struct addrinfo **result){
    if(!result)return EAI_FAIL;*result=NULL;
    const int flags=hints?hints->ai_flags:0;
    if(flags&~(AI_PASSIVE|AI_CANONNAME|AI_NUMERICHOST|AI_NUMERICSERV|AI_ADDRCONFIG))return EAI_BADFLAGS;
    int family=hints?hints->ai_family:AF_UNSPEC;
    if(family!=AF_UNSPEC&&family!=AF_INET&&family!=AF_INET6)return EAI_FAMILY;
    const int type=hints?hints->ai_socktype:0,protocol=hints?hints->ai_protocol:0;
    if(type&&type!=SOCK_STREAM&&type!=SOCK_DGRAM)return EAI_SOCKTYPE;
    if(protocol&&protocol!=IPPROTO_TCP&&protocol!=IPPROTO_UDP)return EAI_SERVICE;
    if((type==SOCK_STREAM&&protocol==IPPROTO_UDP)||(type==SOCK_DGRAM&&protocol==IPPROTO_TCP))return EAI_SOCKTYPE;
    if((!name||!*name)&&!service)return EAI_NONAME;
    if(name&&(!*name||strnlen(name,254)>253))return EAI_NONAME;
    char numeric_name[254];uint32_t scope=0;int numeric_family=0;
    if(name){
        memcpy(numeric_name,name,strlen(name)+1);char *zone=strchr(numeric_name,'%');
        if(zone){*zone++=0;if(!*zone||strlen(zone)>10)return EAI_NONAME;uint64_t value=0;
            for(const char *p=zone;*p;++p){if(*p<'0'||*p>'9')return EAI_NONAME;value=value*10+(unsigned)(*p-'0');if(value>UINT32_MAX)return EAI_NONAME;}scope=(uint32_t)value;
        }
        struct in_addr v4;struct in6_addr v6;
        if(!zone&&inet_pton(AF_INET,numeric_name,&v4)==1)numeric_family=AF_INET;
        else if(inet_pton(AF_INET6,numeric_name,&v6)==1)numeric_family=AF_INET6;
        if(zone&&!numeric_family)return EAI_NONAME;
        if(!numeric_family&&(flags&AI_NUMERICHOST))return EAI_NONAME;
        if(numeric_family){if(family!=AF_UNSPEC&&family!=numeric_family)return EAI_NONAME;family=numeric_family;name=numeric_name;}
    }
    if(flags&AI_ADDRCONFIG){family=configured_family(family);if(family<0)return failed("address-config",EAI_NONAME,hints?hints->ai_family:AF_UNSPEC,flags);}
    if(service){unsigned port=0;if(!*service||strnlen(service,16)>15)return EAI_SERVICE;for(const char*p=service;*p;++p){if(*p<'0'||*p>'9')return EAI_SERVICE;port=port*10+(unsigned)(*p-'0');if(port>65535)return EAI_SERVICE;}}
    if(pthread_once(&once,initialize)||library_status!=ARES_SUCCESS)return failed("library-init",EAI_FAIL,family,flags);
    struct lookup l={0};for(int i=0;i<DNS_SOCKETS;++i)l.sockets[i].fd=-1;
    struct in_addr dns[2];struct ares_options options={0};
    options.flags=ARES_FLAG_NOSEARCH|ARES_FLAG_NOALIASES|ARES_FLAG_NO_DFLT_SVR;
    options.timeout=800;options.tries=2;options.servers=dns;options.nservers=servers(dns);
#ifdef DKRR_DNS_TEST_PORT
    options.udp_port=DKRR_DNS_TEST_PORT;options.tcp_port=DKRR_DNS_TEST_PORT;
#endif
    options.lookups="b";options.sock_state_cb=socket_state;options.sock_state_cb_data=&l;
    const int mask=ARES_OPT_FLAGS|ARES_OPT_TIMEOUTMS|ARES_OPT_TRIES|ARES_OPT_SERVERS|ARES_OPT_LOOKUPS|ARES_OPT_SOCK_STATE_CB
#ifdef DKRR_DNS_TEST_PORT
        |ARES_OPT_UDP_PORT|ARES_OPT_TCP_PORT
#endif
        ;
    ares_channel_t *channel=NULL;int status=ares_init_options(&channel,&options,mask);
    if(status!=ARES_SUCCESS)return failed("channel-init",error_code(status),family,flags);
#if defined(__PROSPERO__) || defined(DKRR_TEST_NATIVE_NET)
    status=native_channel(channel);
    if(status!=ARES_SUCCESS){ares_destroy(channel);return failed("native-sockets",error_code(status),family,flags);}
#endif
    struct ares_addrinfo_hints query={0};query.ai_family=family;query.ai_socktype=type;query.ai_protocol=protocol;
    query.ai_flags=ARES_AI_NOSORT|ARES_AI_NUMERICSERV;
    if(flags&AI_PASSIVE)query.ai_flags|=ARES_AI_PASSIVE;
    if(flags&AI_CANONNAME)query.ai_flags|=ARES_AI_CANONNAME;
    if(flags&AI_NUMERICHOST)query.ai_flags|=ARES_AI_NUMERICHOST;
    ares_getaddrinfo(channel,name,service,&query,completed,&l);
    const uint64_t start=milliseconds();
    while(!l.done&&!l.overflow){
        const uint64_t now=milliseconds();if(!start||!now||now-start>=DNS_DEADLINE_MS){l.status=ARES_ETIMEOUT;break;}
        struct pollfd sockets[DNS_SOCKETS];memcpy(sockets,l.sockets,sizeof(sockets));
        struct timeval max={0,100000},tv;const struct timeval *t=ares_timeout(channel,&max,&tv);
        const int timeout=(int)(t->tv_sec*1000+(t->tv_usec+999)/1000);
        const int ready=
#if defined(__PROSPERO__) || defined(DKRR_TEST_NATIVE_NET)
            dkrr_net_poll(sockets,DNS_SOCKETS,timeout);
#else
            poll(sockets,DNS_SOCKETS,timeout);
#endif
        if(ready<0){if(errno==EINTR)continue;l.status=ARES_ECONNREFUSED;break;}
        ares_fd_events_t events[DNS_SOCKETS];size_t count=0;
        for(int i=0;i<DNS_SOCKETS;++i)if(sockets[i].fd>=0&&sockets[i].revents){
            if(sockets[i].revents&POLLNVAL){l.overflow=1;break;}
            unsigned bits=0;if(sockets[i].revents&(POLLIN|POLLERR|POLLHUP))bits|=ARES_FD_EVENT_READ;
            if(sockets[i].revents&POLLOUT)bits|=ARES_FD_EVENT_WRITE;
            events[count++]=(ares_fd_events_t){sockets[i].fd,bits};
        }
        status=ares_process_fds(channel,events,count,ARES_PROCESS_FLAG_NONE);
        if(status!=ARES_SUCCESS){l.status=status;break;}
    }
    /* Destroy may synchronously invoke the callback: preserve timeout/error
     * before destruction, and keep callback storage alive until it returns. */
    status=l.done?l.status:(l.overflow?ARES_EFORMERR:l.status);
    ares_destroy(channel);
    int error=error_code(status);struct addrinfo **tail=result;size_t count=0;
    if(!error&&l.answer)for(const struct ares_addrinfo_node *n=l.answer->nodes;n;n=n->ai_next){
        if(++count>64||!n->ai_addr||
           (n->ai_family!=AF_INET&&n->ai_family!=AF_INET6)||
           n->ai_addrlen!=(n->ai_family==AF_INET?sizeof(struct sockaddr_in):sizeof(struct sockaddr_in6))){error=EAI_FAIL;break;}
        struct addrinfo *p=calloc(1,sizeof(*p));if(!p){error=EAI_MEMORY;break;}
        p->ai_family=n->ai_family;p->ai_socktype=n->ai_socktype;p->ai_protocol=n->ai_protocol;p->ai_flags=flags;p->ai_addrlen=n->ai_addrlen;
        p->ai_addr=malloc(p->ai_addrlen);if(!p->ai_addr){free(p);error=EAI_MEMORY;break;}memcpy(p->ai_addr,n->ai_addr,p->ai_addrlen);
        if(scope&&p->ai_family==AF_INET6)((struct sockaddr_in6*)p->ai_addr)->sin6_scope_id=scope;
        const char *canonical=l.answer->name?l.answer->name:(l.answer->cnames?l.answer->cnames->name:name);
        if(!*result&&(flags&AI_CANONNAME)&&canonical){const size_t bytes=strnlen(canonical,254);if(bytes>253){free(p->ai_addr);free(p);error=EAI_FAIL;break;}
            p->ai_canonname=malloc(bytes+1);if(!p->ai_canonname){free(p->ai_addr);free(p);error=EAI_MEMORY;break;}memcpy(p->ai_canonname,canonical,bytes+1);}
        *tail=p;tail=&p->ai_next;
    }
    if(l.answer)ares_freeaddrinfo(l.answer);
    if(error||!*result){__wrap_freeaddrinfo(*result);*result=NULL;return failed("query-result",error?error:EAI_NONAME,family,flags);}
    return 0;
}
