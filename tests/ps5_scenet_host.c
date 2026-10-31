/* SPDX-License-Identifier: GPL-3.0-or-later */
/* SceNet fault double: native IDs deliberately differ from kernel file FDs. */
#define _GNU_SOURCE
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stddef.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#define OFFSET 10000
struct sce_event { uint32_t events,pad;uint64_t ident,data; };
static _Thread_local int net_errno;
static _Atomic int fail_at,step;
void dkrr_test_net_fail(int at){atomic_store(&step,0);atomic_store(&fail_at,at);}
static int fail(void){int at=atomic_load(&fail_at);if(at&&atomic_fetch_add(&step,1)+1==at){net_errno=EACCES;return 1;}return 0;}
static int result(int r){if(r<0)net_errno=errno;return r;}
int *sceNetErrnoLoc(void){return &net_errno;}
int sceNetInit(void){return 0;}
int sceNetSocket(const char*n,int d,int t,int p){(void)n;if(fail())return -1;int r=result(socket(d,t,p));return r<0?r:r+OFFSET;}
int sceNetSocketClose(int fd){return result(close(fd-OFFSET));}
int sceNetBind(int fd,const struct sockaddr*a,socklen_t n){if(fail())return -1;return result(bind(fd-OFFSET,a,n));}
int sceNetConnect(int fd,const struct sockaddr*a,socklen_t n){if(fail())return -1;return result(connect(fd-OFFSET,a,n));}
int sceNetListen(int fd,int n){return result(listen(fd-OFFSET,n));}
int sceNetAccept(int fd,struct sockaddr*a,socklen_t*n){int r=result(accept(fd-OFFSET,a,n));return r<0?r:r+OFFSET;}
int sceNetShutdown(int fd,int how){return result(shutdown(fd-OFFSET,how));}
int sceNetGetsockopt(int fd,int level,int opt,void*v,socklen_t*n){
 if(opt==0x1200&&level==SOL_SOCKET){int r=fcntl(fd-OFFSET,F_GETFL);if(r<0)return result(r);if(!v||!n||*n<sizeof(int)){errno=EINVAL;return result(-1);}*(int*)v=(r&O_NONBLOCK)!=0;*n=sizeof(int);return 0;}
 return result(getsockopt(fd-OFFSET,level,opt,v,n));}
int sceNetSetsockopt(int fd,int level,int opt,const void*v,socklen_t n){
 if(fail())return -1;
 if(opt==0x1200&&level==SOL_SOCKET){int flags=fcntl(fd-OFFSET,F_GETFL);if(flags<0)return result(-1);return result(fcntl(fd-OFFSET,F_SETFL,*(const int*)v?flags|O_NONBLOCK:flags&~O_NONBLOCK));}
 return result(setsockopt(fd-OFFSET,level,opt,v,n));}
int sceNetGetsockname(int fd,struct sockaddr*a,socklen_t*n){if(fail())return -1;return result(getsockname(fd-OFFSET,a,n));}
int sceNetGetpeername(int fd,struct sockaddr*a,socklen_t*n){return result(getpeername(fd-OFFSET,a,n));}
int sceNetSend(int fd,const void*b,size_t n,int flags){return result((int)send(fd-OFFSET,b,n,flags|MSG_NOSIGNAL));}
int sceNetSendto(int fd,const void*b,size_t n,int flags,const struct sockaddr*a,socklen_t len){return result((int)sendto(fd-OFFSET,b,n,flags|MSG_NOSIGNAL,a,len));}
int sceNetRecv(int fd,void*b,size_t n,int flags){return result((int)recv(fd-OFFSET,b,n,flags));}
int sceNetRecvfrom(int fd,void*b,size_t n,int flags,struct sockaddr*a,socklen_t*len){return result((int)recvfrom(fd-OFFSET,b,n,flags,a,len));}
int sceNetRecvmsg(int fd,struct msghdr*m,int flags){return result((int)recvmsg(fd-OFFSET,m,flags));}
int sceNetSendmsg(int fd,const struct msghdr*m,int flags){return result((int)sendmsg(fd-OFFSET,m,flags|MSG_NOSIGNAL));}
int sceNetEpollCreate(const char*n,int flags){(void)n;(void)flags;if(fail())return -1;int fd=result(epoll_create1(EPOLL_CLOEXEC));return fd<0?fd:fd+OFFSET;}
int sceNetEpollControl(int ep,int op,int fd,struct sce_event*e){
 if(fail())return -1;struct epoll_event event;memset(&event,0,sizeof(event));
 event.events=((e->events&1)?EPOLLIN:0)|((e->events&2)?EPOLLOUT:0);event.data.u64=e->data;
 int rc=epoll_ctl(ep-OFFSET,op,fd-OFFSET,&event);
 return result(rc);}
int sceNetEpollWait(int ep,struct sce_event*events,int count,int us){
 if(fail())return -1;struct epoll_event tmp[1024];if(count>1024){errno=EINVAL;return result(-1);}
 int n=result(epoll_wait(ep-OFFSET,tmp,count,us<0?-1:(int)(((int64_t)us+999)/1000)));
 for(int i=0;i<n;++i){events[i]=(struct sce_event){0};events[i].data=tmp[i].data.u64;uint32_t e=tmp[i].events;
  events[i].events=((e&EPOLLIN)?1:0)|((e&EPOLLOUT)?2:0)|((e&EPOLLERR)?8:0)|((e&EPOLLHUP)?16:0);}
 return n;}
int sceNetEpollDestroy(int ep){return result(close(ep-OFFSET));}
