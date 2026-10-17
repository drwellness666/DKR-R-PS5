/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef DKRR_NETWORK_GATE_H
#define DKRR_NETWORK_GATE_H
#include <poll.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
/* Only the network edition waits. No directory, lock or notification is
 * touched until the game explicitly commits over its inherited connection. */
static inline int dkrr_network_commit(void) {
 const char ready[]="DKRR UPDATE READY v1\n",expected[]="DKRRGO1\n";
 size_t at=0;
 while(at<sizeof(ready)-1){ssize_t n=write(STDOUT_FILENO,ready+at,sizeof(ready)-1-at);if(n<0&&errno==EINTR)continue;if(n<=0)return -1;at+=(size_t)n;}
 struct timespec start,now;if(clock_gettime(CLOCK_MONOTONIC,&start)<0)return -1;
 char input[sizeof(expected)-1];at=0;
 while(at<sizeof(input)) {
  if(clock_gettime(CLOCK_MONOTONIC,&now)<0||now.tv_sec-start.tv_sec>=30)return -1;
  struct pollfd fd={STDIN_FILENO,POLLIN,0};int result=poll(&fd,1,1000);
  if(result<0&&errno==EINTR)continue;if(result<0)return -1;if(!result)continue;
  if(!(fd.revents&POLLIN))return -1;
  ssize_t n=read(STDIN_FILENO,input+at,sizeof(input)-at);if(n<0&&errno==EINTR)continue;if(n<=0)return -1;at+=(size_t)n;
 }
 return memcmp(input,expected,sizeof(input))?-1:0;
}
#endif
