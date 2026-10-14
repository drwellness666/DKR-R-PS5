/* SPDX-License-Identifier: GPL-3.0-or-later
 * Optional /dev/klog relay, following the public klogsrv transport.
 * Bind first; never kill another logger or inspect process/kernel structures.
 * One PC client, one bounded chunk; slow clients disconnect after two seconds.
 */
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <unistd.h>
#ifndef DKR_LOG_DEVICE
#define DKR_LOG_DEVICE "/dev/klog"
#endif
#ifndef DKR_LOG_PORT
#define DKR_LOG_PORT 3232
#endif
static volatile sig_atomic_t stopped;
static void stop(int signal_number){(void)signal_number;stopped=1;}
static unsigned long long milliseconds(void){struct timespec t;return clock_gettime(CLOCK_MONOTONIC,&t)==0?(unsigned long long)t.tv_sec*1000+(unsigned long long)t.tv_nsec/1000000:0;}
static int nonblocking(int fd){int flags=fcntl(fd,F_GETFL,0);return flags<0?-1:fcntl(fd,F_SETFL,flags|O_NONBLOCK);}
int main(void){
    int listener=socket(AF_INET,SOCK_STREAM,0);if(listener<0)return 1;
    struct sockaddr_in address={0};address.sin_family=AF_INET;address.sin_port=htons(DKR_LOG_PORT);address.sin_addr.s_addr=htonl(INADDR_ANY);
#if defined(__PROSPERO__)
    address.sin_len=sizeof(address);
#endif
    if(bind(listener,(struct sockaddr*)&address,sizeof(address))<0||listen(listener,1)<0||nonblocking(listener)<0){perror("DKRR relay bind/listen (existing service retained)");close(listener);return 2;}
    int log= open(DKR_LOG_DEVICE,O_RDONLY|O_NONBLOCK);if(log<0){perror("DKRR relay log open");close(listener);return 3;}
    if(listener>=FD_SETSIZE||log>=FD_SETSIZE){close(log);close(listener);return 4;}
    struct sigaction action={0};action.sa_handler=stop;sigemptyset(&action.sa_mask);(void)sigaction(SIGTERM,&action,0);(void)sigaction(SIGINT,&action,0);
    action.sa_handler=SIG_IGN;(void)sigaction(SIGPIPE,&action,0);
    int client=-1;unsigned char chunk[4096];size_t used=0,sent=0;unsigned long long since=0;
    fprintf(stderr,"DKRR relay ready port=%u; no process inspection, launch or GPU hooks\n",(unsigned)DKR_LOG_PORT);fflush(stderr);
    while(!stopped){
        fd_set readset,writeset;FD_ZERO(&readset);FD_ZERO(&writeset);FD_SET(listener,&readset);int maximum=listener;
        if(client>=0){FD_SET(client,&readset);if(client>maximum)maximum=client;
            if(used>sent)FD_SET(client,&writeset);else{FD_SET(log,&readset);if(log>maximum)maximum=log;}}
        struct timeval timeout={1,0};int ready=select(maximum+1,&readset,&writeset,0,&timeout);
        if(ready<0){if(errno==EINTR)continue;break;}
        if(client>=0 && used>sent && milliseconds()-since>2000){close(client);client=-1;used=sent=0;continue;}
        if(FD_ISSET(listener,&readset)){int incoming=accept(listener,0,0);if(incoming>=0){
            if(client>=0||incoming>=FD_SETSIZE||nonblocking(incoming)<0)close(incoming);else{client=incoming;used=sent=0;}
            /* FD sets predate the new connection; inspect it on the next loop. */
            continue;
        }}
        if(client<0)continue;
        if(FD_ISSET(client,&readset)){unsigned char ignored;ssize_t n=recv(client,&ignored,1,0);if(n==0||n>0||(n<0&&errno!=EAGAIN&&errno!=EWOULDBLOCK&&errno!=EINTR)){close(client);client=-1;used=sent=0;continue;}}
        if(FD_ISSET(log,&readset)){ssize_t n=read(log,chunk,sizeof(chunk));if(n>0){used=(size_t)n;sent=0;since=milliseconds();}
            else if(n==0){break;}else if(errno!=EAGAIN&&errno!=EWOULDBLOCK&&errno!=EINTR)break;}
        if(used>sent&&FD_ISSET(client,&writeset)){ssize_t n=send(client,chunk+sent,used-sent,0);
            if(n>0)sent+=(size_t)n;else if(n==0||(errno!=EAGAIN&&errno!=EWOULDBLOCK&&errno!=EINTR)){close(client);client=-1;used=sent=0;}}
    }
    if(client>=0)close(client);close(log);close(listener);return 0;
}
