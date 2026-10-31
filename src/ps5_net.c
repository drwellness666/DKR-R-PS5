/*
 * Native SceNet socket and epoll ABI adapted from ps5-native-app-boilerplate /
 * ProsperoLight, Copyright (C) 2026 BlackBearReloaded.
 * Reference: Portablelle/OpenNOW-PS5 ee86867651d1aeb2a95d7a706c4b0b84f05d3542
 * src/platform/ps5_sockets.c (SHA256 in HTTPS_SOURCES.json).
 * SPDX-License-Identifier: GPL-3.0-or-later
 * DKR-R changes: scoped symbols, checked UDP wakeups, poll/select semantics,
 * resource accounting; retain the game's IPv4/IPv6 c-ares DNS and TLS stack.
 */
#include "ps5_net.h"
#include <errno.h>
#include <limits.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <net/if.h>

struct dkrr_sce_epoll_event { uint32_t events, pad; uint64_t ident, data; };
_Static_assert(sizeof(struct dkrr_sce_epoll_event)==24,"SceNet epoll ABI");
enum { NET_IN=1, NET_OUT=2, NET_ERR=8, NET_HUP=16, NET_ADD=1, NET_NBIO=0x1200, NET_MAX_POLL=1024 };
extern int sceNetInit(void);
extern int *sceNetErrnoLoc(void);
extern int sceNetSocket(const char*,int,int,int);
extern int sceNetSocketClose(int);
extern int sceNetBind(int,const struct sockaddr*,socklen_t);
extern int sceNetConnect(int,const struct sockaddr*,socklen_t);
extern int sceNetListen(int,int);
extern int sceNetAccept(int,struct sockaddr*,socklen_t*);
extern int sceNetShutdown(int,int);
extern int sceNetGetsockopt(int,int,int,void*,socklen_t*);
extern int sceNetSetsockopt(int,int,int,const void*,socklen_t);
extern int sceNetGetsockname(int,struct sockaddr*,socklen_t*);
extern int sceNetGetpeername(int,struct sockaddr*,socklen_t*);
extern int sceNetSend(int,const void*,size_t,int);
extern int sceNetSendto(int,const void*,size_t,int,const struct sockaddr*,socklen_t);
extern int sceNetRecv(int,void*,size_t,int);
extern int sceNetRecvfrom(int,void*,size_t,int,struct sockaddr*,socklen_t*);
extern int sceNetRecvmsg(int,struct msghdr*,int);
extern int sceNetSendmsg(int,const struct msghdr*,int);
extern int sceNetCtlInit(void) __attribute__((weak));
extern int sceNetCtlGetInfo(int,void*) __attribute__((weak));
extern int sceNetEpollCreate(const char*,int);
extern int sceNetEpollControl(int,int,int,struct dkrr_sce_epoll_event*);
extern int sceNetEpollWait(int,struct dkrr_sce_epoll_event*,int,int);
extern int sceNetEpollDestroy(int);
static pthread_once_t initialized=PTHREAD_ONCE_INIT;
static int init_result;
static _Atomic uint64_t opened,closed,close_errors,polls,epolls_opened,epolls_closed;
static void initialize(void){init_result=sceNetInit();}
int dkrr_net_init_result(void){pthread_once(&initialized,initialize);return init_result;}
static int net_result(int result){
    if(result<0){int *p=sceNetErrnoLoc();errno=p&&*p?*p:EIO;return -1;}return result;
}
struct dkrr_net_counters dkrr_net_counters_read(void){
    return (struct dkrr_net_counters){atomic_load(&opened),atomic_load(&closed),atomic_load(&close_errors),
        atomic_load(&polls),atomic_load(&epolls_opened),atomic_load(&epolls_closed)};
}
int dkrr_net_diagnostic(char *out,size_t bytes){
    const struct dkrr_net_counters n=dkrr_net_counters_read();
    return snprintf(out,bytes,"DKRR_NETWORK v=1 opened=%llu closed=%llu close-errors=%llu polls=%llu epoll-open=%llu epoll-close=%llu\n",
        (unsigned long long)n.opened,(unsigned long long)n.closed,(unsigned long long)n.close_errors,
        (unsigned long long)n.polls,(unsigned long long)n.epolls_opened,(unsigned long long)n.epolls_closed);
}
int dkrr_net_close(int fd){
    if(fd<0){errno=EBADF;return -1;} /* RTC cleanup also visits unused -1 slots. */
    int result=net_result(sceNetSocketClose(fd));
    if(!result)atomic_fetch_add(&closed,1);else atomic_fetch_add(&close_errors,1);
    return result;
}
int dkrr_net_socket(int family,int type,int protocol){
    (void)dkrr_net_init_result();
    const int nb=(type&SOCK_NONBLOCK)!=0;
    /* SceNet creates process-owned IDs, not inheritable kernel FDs. */
    type&=~(SOCK_NONBLOCK|SOCK_CLOEXEC);
    int fd=net_result(sceNetSocket("dkrr",family,type,protocol));
    if(fd<0&&errno==EPROTONOSUPPORT&&(family==AF_INET||family==AF_INET6)&&
       ((type==SOCK_STREAM&&protocol==IPPROTO_TCP)||(type==SOCK_DGRAM&&protocol==IPPROTO_UDP)))
        fd=net_result(sceNetSocket("dkrr",family,type,0));
    if(fd<0)return -1;
    atomic_fetch_add(&opened,1);
    if(nb){int on=1;if(net_result(sceNetSetsockopt(fd,SOL_SOCKET,NET_NBIO,&on,sizeof(on)))<0){
        int saved=errno;dkrr_net_close(fd);errno=saved;return -1;}}
    return fd;
}
int dkrr_net_bind(int fd,const struct sockaddr*a,socklen_t n){return net_result(sceNetBind(fd,a,n));}
int dkrr_net_connect(int fd,const struct sockaddr*a,socklen_t n){return net_result(sceNetConnect(fd,a,n));}
int dkrr_net_listen(int fd,int n){return net_result(sceNetListen(fd,n));}
int dkrr_net_accept(int fd,struct sockaddr*a,socklen_t*n){int s=net_result(sceNetAccept(fd,a,n));if(s>=0)atomic_fetch_add(&opened,1);return s;}
int dkrr_net_shutdown(int fd,int how){return net_result(sceNetShutdown(fd,how));}
int dkrr_net_getsockopt(int fd,int level,int opt,void*v,socklen_t*n){return net_result(sceNetGetsockopt(fd,level,opt,v,n));}
int dkrr_net_setsockopt(int fd,int level,int opt,const void*v,socklen_t n){return net_result(sceNetSetsockopt(fd,level,opt,v,n));}
int dkrr_net_getsockname(int fd,struct sockaddr*a,socklen_t*n){return net_result(sceNetGetsockname(fd,a,n));}
int dkrr_net_getpeername(int fd,struct sockaddr*a,socklen_t*n){return net_result(sceNetGetpeername(fd,a,n));}
ssize_t dkrr_net_send(int fd,const void*b,size_t n,int flags){if(n>INT_MAX){errno=EMSGSIZE;return -1;}return net_result(sceNetSend(fd,b,n,flags&~MSG_NOSIGNAL));}
ssize_t dkrr_net_sendto(int fd,const void*b,size_t n,int flags,const struct sockaddr*a,socklen_t len){if(n>INT_MAX){errno=EMSGSIZE;return -1;}return net_result(sceNetSendto(fd,b,n,flags&~MSG_NOSIGNAL,a,len));}
ssize_t dkrr_net_recv(int fd,void*b,size_t n,int flags){if(n>INT_MAX)n=INT_MAX;return net_result(sceNetRecv(fd,b,n,flags&~MSG_NOSIGNAL));}
ssize_t dkrr_net_recvfrom(int fd,void*b,size_t n,int flags,struct sockaddr*a,socklen_t*len){if(n>INT_MAX)n=INT_MAX;return net_result(sceNetRecvfrom(fd,b,n,flags&~MSG_NOSIGNAL,a,len));}
ssize_t dkrr_net_recvmsg(int fd,struct msghdr *message,int flags){return net_result(sceNetRecvmsg(fd,message,flags&~MSG_NOSIGNAL));}
ssize_t dkrr_net_sendmsg(int fd,const struct msghdr *message,int flags){return net_result(sceNetSendmsg(fd,message,flags&~MSG_NOSIGNAL));}
static char native_interface_name[]="PS5 Network";
struct native_interface {struct ifaddrs info;struct sockaddr_in address,mask,broadcast;};
static int netctl_ip(int key,struct in_addr *out){
    unsigned char value[256]={0};
    return sceNetCtlGetInfo&&sceNetCtlGetInfo(key,value)==0&&memchr(value,0,sizeof(value))&&
        inet_pton(AF_INET,(const char*)value,out)==1;
}
int dkrr_net_getifaddrs(struct ifaddrs **out){
    if(!out){errno=EINVAL;return -1;}*out=NULL;
    /* Keep real IPv4/IPv6 interface metadata when the title can enumerate it.
     * Some native app contexts expose no interface list. NetCtl then supplies
     * the actual configured IPv4 address, never a guessed/public address. */
    struct ifaddrs *all=NULL;
    if(getifaddrs(&all)==0){
        for(struct ifaddrs *p=all;p;p=p->ifa_next)
            if(p->ifa_addr&&(p->ifa_flags&IFF_UP)&&!(p->ifa_flags&IFF_LOOPBACK)&&
               (p->ifa_addr->sa_family==AF_INET||p->ifa_addr->sa_family==AF_INET6)){*out=all;return 0;}
        freeifaddrs(all);
    }
    if(sceNetCtlInit)(void)sceNetCtlInit();
    struct in_addr address,mask;
    if(!netctl_ip(14,&address)||address.s_addr==INADDR_ANY||
       (ntohl(address.s_addr)>>24)==127){errno=ENETDOWN;return -1;}
    struct native_interface *entry=calloc(1,sizeof(*entry));if(!entry){errno=ENOMEM;return -1;}
    entry->info.ifa_name=native_interface_name;entry->info.ifa_flags=IFF_UP|IFF_RUNNING;
    entry->address.sin_family=AF_INET;entry->address.sin_addr=address;
#ifdef __PROSPERO__
    entry->address.sin_len=sizeof(entry->address);
#endif
    entry->info.ifa_addr=(struct sockaddr*)&entry->address;
    if(netctl_ip(15,&mask)){
        entry->mask=entry->address;entry->mask.sin_addr=mask;
        entry->broadcast=entry->address;entry->broadcast.sin_addr.s_addr=address.s_addr|~mask.s_addr;
        entry->info.ifa_netmask=(struct sockaddr*)&entry->mask;
        entry->info.ifa_broadaddr=(struct sockaddr*)&entry->broadcast;entry->info.ifa_flags|=IFF_BROADCAST;
    }
    *out=&entry->info;return 0;
}
void dkrr_net_freeifaddrs(struct ifaddrs *p){
    if(p&&p->ifa_name==native_interface_name)free(p);else freeifaddrs(p);
}
int dkrr_net_fcntl(int fd,int cmd,...){
    if(cmd==F_GETFL){int on=0;socklen_t len=sizeof(on);if(dkrr_net_getsockopt(fd,SOL_SOCKET,NET_NBIO,&on,&len)<0)return -1;return on?O_NONBLOCK:0;}
    if(cmd==F_SETFL){va_list args;va_start(args,cmd);int flags=va_arg(args,int);va_end(args);
        if(flags&~(O_NONBLOCK|O_RDWR)){errno=EINVAL;return -1;}
        int on=(flags&O_NONBLOCK)!=0;return dkrr_net_setsockopt(fd,SOL_SOCKET,NET_NBIO,&on,sizeof(on));}
    /* Do not pretend kernel descriptor flags were set on a native socket ID.
     * DNS uses c-ares' socket callback API, so never requests FD_CLOEXEC. */
    errno=EOPNOTSUPP;return -1;
}
int dkrr_net_ioctl(int fd,unsigned long request,...){
    if(request!=FIONBIO){errno=EOPNOTSUPP;return -1;}
    va_list args;va_start(args,request);int *on=va_arg(args,int*);va_end(args);
    if(!on){errno=EFAULT;return -1;}return dkrr_net_setsockopt(fd,SOL_SOCKET,NET_NBIO,on,sizeof(*on));
}
int dkrr_net_wakeup_pair(int pair[2]){
    if(!pair){errno=EINVAL;return -1;}pair[0]=pair[1]=-1;
    struct sockaddr_in address;memset(&address,0,sizeof(address));
#ifdef __PROSPERO__
    address.sin_len=sizeof(address);
#endif
    address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    socklen_t len=sizeof(address);
    pair[0]=dkrr_net_socket(AF_INET,SOCK_DGRAM|SOCK_NONBLOCK,0);
    if(pair[0]<0)goto fail;
    pair[1]=dkrr_net_socket(AF_INET,SOCK_DGRAM|SOCK_NONBLOCK,0);
    if(pair[1]<0)goto fail;
    if(dkrr_net_bind(pair[0],(struct sockaddr*)&address,len)<0||
       dkrr_net_getsockname(pair[0],(struct sockaddr*)&address,&len)<0||
       dkrr_net_connect(pair[1],(struct sockaddr*)&address,len)<0)goto fail;
    len=sizeof(address);
    if(dkrr_net_getsockname(pair[1],(struct sockaddr*)&address,&len)<0||
       dkrr_net_connect(pair[0],(struct sockaddr*)&address,len)<0)goto fail;
    return 0;
fail:;
    const int saved=errno;
    for(int i=0;i<2;++i){if(pair[i]>=0)dkrr_net_close(pair[i]);pair[i]=-1;}
    errno=saved;return -1;
}
static uint32_t to_native(short e){return ((e&(POLLIN|POLLRDNORM))?NET_IN:0)|((e&(POLLOUT|POLLWRNORM))?NET_OUT:0);}
static short from_native(uint32_t e,short requested){
    return ((e&NET_IN)?(requested&(POLLIN|POLLRDNORM)):0)|((e&NET_OUT)?(requested&(POLLOUT|POLLWRNORM)):0)|
        ((e&NET_ERR)?POLLERR:0)|((e&NET_HUP)?POLLHUP:0);
}
int dkrr_net_poll(struct pollfd *fds,nfds_t count,int timeout){
    if((!fds&&count)||count>NET_MAX_POLL){errno=EINVAL;return -1;}
    atomic_fetch_add(&polls,1);
    struct dkrr_sce_epoll_event small[16],*events=small;
    if(count>16){events=calloc(count,sizeof(*events));if(!events){errno=ENOMEM;return -1;}}
    int ep=-1,result=-1,invalid=0,used=0;
    for(nfds_t i=0;i<count;++i)fds[i].revents=0;
    for(nfds_t i=0;i<count;++i){
        if(fds[i].fd<0)continue;
        if(ep<0){ep=net_result(sceNetEpollCreate("dkrr-poll",0));if(ep<0)goto done;atomic_fetch_add(&epolls_opened,1);}
        int duplicate=0;for(nfds_t j=0;j<i;++j)if(fds[j].fd==fds[i].fd){duplicate=1;break;}
        if(duplicate)continue;
        uint32_t mask=to_native(fds[i].events);
        for(nfds_t j=i+1;j<count;++j)if(fds[j].fd==fds[i].fd)mask|=to_native(fds[j].events);
        struct dkrr_sce_epoll_event event={mask,0,0,(uint64_t)i};
        if(net_result(sceNetEpollControl(ep,NET_ADD,fds[i].fd,&event))<0){
            const int error=errno;
            int invalid_socket=error==EBADF||error==ENOTSOCK;
            if(error==EINVAL){
                /* A peer can close after RTC snapshots its poll list. A new
                 * epoll owner may reuse that native ID; classify the stale
                 * non-socket as POLLNVAL instead of killing the RTC worker. */
                int type=0;socklen_t bytes=sizeof(type);
                invalid_socket=dkrr_net_getsockopt(fds[i].fd,SOL_SOCKET,SO_TYPE,&type,&bytes)<0&&
                    (errno==EBADF||errno==ENOTSOCK||errno==EINVAL);
            }
            errno=error;if(!invalid_socket)goto done;
            for(nfds_t j=i;j<count;++j)if(fds[j].fd==fds[i].fd){fds[j].revents=POLLNVAL;++invalid;}
        }else ++used;
    }
    if(invalid)timeout=0;
    if(!used){
        if(!invalid){
            /* poll with only negative IDs still honours its timeout. */
            struct timespec delay={timeout<0?86400:timeout/1000,timeout<0?0:(long)(timeout%1000)*1000000};
            do {if(nanosleep(&delay,&delay)<0)goto done;}while(timeout<0);
        }
        result=invalid;goto done;
    }
    result=net_result(sceNetEpollWait(ep,events,used,timeout<0?-1:timeout>INT_MAX/1000?INT_MAX:timeout*1000));
    if(result<0)goto done;
    for(int i=0;i<result;++i){nfds_t index=(nfds_t)events[i].data;if(index>=count)continue;
        for(nfds_t j=0;j<count;++j)if(fds[j].fd==fds[index].fd)
            fds[j].revents|=from_native(events[i].events,fds[j].events);}
    result=0;for(nfds_t i=0;i<count;++i)result+=fds[i].revents!=0;
done:;
    int saved=errno;
    if(ep>=0){if(net_result(sceNetEpollDestroy(ep))<0){saved=errno;result=-1;}else atomic_fetch_add(&epolls_closed,1);}
    if(events!=small)free(events);errno=saved;return result;
}
int dkrr_net_select(int n,fd_set *reads,fd_set *writes,fd_set *errors,struct timeval *tv){
    if(n<0||n>FD_SETSIZE||(tv&&(tv->tv_sec<0||tv->tv_usec<0||tv->tv_usec>=1000000))){errno=EINVAL;return -1;}
    struct pollfd fds[FD_SETSIZE];int used=0;
    for(int fd=0;fd<n;++fd){short mask=0;
        if(reads&&FD_ISSET(fd,reads))mask|=POLLIN;
        if(writes&&FD_ISSET(fd,writes))mask|=POLLOUT;
        if(errors&&FD_ISSET(fd,errors))mask|=POLLPRI;
        if(mask)fds[used++]=(struct pollfd){fd,mask,0};}
    int timeout=-1;if(tv){timeout=tv->tv_sec>=INT_MAX/1000?INT_MAX:(int)tv->tv_sec*1000+(int)((tv->tv_usec+999)/1000);}
    int result=dkrr_net_poll(fds,(nfds_t)used,timeout);if(result<0)return -1;
    for(int i=0;i<used;++i)if(fds[i].revents&POLLNVAL){errno=EBADF;return -1;}
    if(reads)FD_ZERO(reads);if(writes)FD_ZERO(writes);if(errors)FD_ZERO(errors);result=0;
    for(int i=0;i<used;++i){int fd=fds[i].fd;short e=fds[i].revents;
        if(reads&&(e&(POLLIN|POLLHUP|POLLERR))){FD_SET(fd,reads);++result;}
        if(writes&&(e&(POLLOUT|POLLERR))){FD_SET(fd,writes);++result;}
        if(errors&&(e&POLLPRI)){FD_SET(fd,errors);++result;}}
    return result;
}
