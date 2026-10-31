/* SPDX-License-Identifier: GPL-3.0-or-later */
#define _GNU_SOURCE
#include "ps5_net.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <arpa/inet.h>
#include <string.h>
extern void dkrr_test_net_fail(int);
static int interface_mode;
int __real_getifaddrs(struct ifaddrs**);
int __wrap_getifaddrs(struct ifaddrs **out){if(!interface_mode)return __real_getifaddrs(out);*out=NULL;return interface_mode==2?-1:0;}
int sceNetCtlInit(void){return 0;}
int sceNetCtlGetInfo(int id,void*out){if(interface_mode==3)return -1;strcpy(out,id==14?"192.0.2.17":"255.255.255.0");return 0;}
static void balanced(void){struct dkrr_net_counters n=dkrr_net_counters_read();assert(n.opened==n.closed&&n.epolls_opened==n.epolls_closed&&!n.close_errors);}
int main(void){
 for(int fail=1;fail<=9;++fail){dkrr_test_net_fail(fail);int fds[2];int rc=dkrr_net_wakeup_pair(fds);if(rc>=0||errno!=EACCES)fprintf(stderr,"failure site=%d rc=%d errno=%d\n",fail,rc,errno);assert(rc<0&&errno==EACCES);assert(fds[0]==-1&&fds[1]==-1);balanced();}
 dkrr_test_net_fail(0);
 int pair[2];assert(!dkrr_net_wakeup_pair(pair));
 assert(fcntl(pair[0],F_GETFL)<0&&errno==EBADF); // Native ID is not a kernel FD.
 assert(dkrr_net_fcntl(pair[0],F_GETFL)&O_NONBLOCK);
 assert(dkrr_net_fcntl(pair[0],F_SETFD,FD_CLOEXEC)<0&&errno==EOPNOTSUPP);
 struct pollfd fds[4]={{pair[0],POLLIN,0},{pair[0],POLLRDNORM,0},{-1,POLLIN,0},{999999,POLLIN,0}};
 char b=17;assert(dkrr_net_send(pair[1],&b,1,0)==1);
 assert(dkrr_net_poll(fds,4,100)==3);assert((fds[0].revents&POLLIN)&&(fds[1].revents&POLLRDNORM)&&!fds[2].revents&&(fds[3].revents&POLLNVAL));
 assert(dkrr_net_recv(pair[0],&b,1,0)==1&&b==17);
 for(int i=0;i<10000;++i){assert(dkrr_net_send(pair[1],&b,1,0)==1);assert(dkrr_net_poll(fds,2,20)==2);assert(dkrr_net_recv(pair[0],&b,1,0)==1);}
 assert(dkrr_net_recv(pair[0],&b,1,0)<0&&errno==EAGAIN);
 for(int fail=1;fail<=3;++fail){dkrr_test_net_fail(fail);assert(dkrr_net_poll(fds,1,0)<0&&errno==EACCES);}
 dkrr_test_net_fail(0);assert(dkrr_net_poll(fds,1,0)==0);
 assert(dkrr_net_select(FD_SETSIZE+1,NULL,NULL,NULL,NULL)<0&&errno==EINVAL);
 struct timeval invalid={0,1000000};assert(dkrr_net_select(0,NULL,NULL,NULL,&invalid)<0&&errno==EINVAL);
 struct pollfd none={-1,0,99};struct timespec a,z;clock_gettime(CLOCK_MONOTONIC,&a);assert(!dkrr_net_poll(&none,1,20));clock_gettime(CLOCK_MONOTONIC,&z);assert((z.tv_sec-a.tv_sec)*1000000000LL+z.tv_nsec-a.tv_nsec>=15000000);
 assert(!dkrr_net_close(pair[0])&&!dkrr_net_close(pair[1]));balanced();
 for(int mode=0;mode<3;++mode){interface_mode=mode;
  for(int i=0;i<100;++i){struct ifaddrs *list=NULL;assert(!dkrr_net_getifaddrs(&list)&&list);
   if(mode){char address[INET_ADDRSTRLEN];assert(!list->ifa_next&&list->ifa_addr&&list->ifa_netmask);
    assert(inet_ntop(AF_INET,&((struct sockaddr_in*)list->ifa_addr)->sin_addr,address,sizeof(address))&&!strcmp(address,"192.0.2.17"));}
   dkrr_net_freeifaddrs(list);}}
 interface_mode=3;struct ifaddrs *list=(void*)1;assert(dkrr_net_getifaddrs(&list)<0&&!list&&errno==ENETDOWN);
 puts("PASS native-ID isolation, nine wakeup failure cleanups, duplicate/invalid/negative poll IDs, 10000 epoll cycles, poll failures, select bounds and no resource leaks");
}
