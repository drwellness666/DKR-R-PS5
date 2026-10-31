/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ifaddrs.h>
#ifdef DKRR_TEST_NATIVE_NET
#include "ps5_net.h"
#endif
static int empty_interfaces;
void dkrr_test_nonblocking_checked(void);
int __real_getifaddrs(struct ifaddrs **);
void __real_freeifaddrs(struct ifaddrs *);
int __wrap_getifaddrs(struct ifaddrs **out){if(empty_interfaces){*out=NULL;return 0;}return __real_getifaddrs(out);}
void __wrap_freeifaddrs(struct ifaddrs *p){if(p)__real_freeifaddrs(p);}
int dkrr_rtc_entropy_available(void){return 1;}
int dkrr_secure_random(void *out,size_t bytes){return getentropy(out,bytes);}
int __wrap_getaddrinfo(const char*,const char*,const struct addrinfo*,struct addrinfo**);
void __wrap_freeaddrinfo(struct addrinfo*);
int sceNetCtlInit(void){return 0;}
int sceNetCtlGetInfo(int id,void *info){strcpy(info,id==17?"127.0.0.1":"0.0.0.0");return 0;}
static void check(const char *name,int family,const char *expected){
    struct addrinfo hints={0},*answer=NULL;hints.ai_family=family;hints.ai_socktype=SOCK_STREAM;hints.ai_protocol=IPPROTO_TCP;
    int rc=__wrap_getaddrinfo(name,"443",&hints,&answer);
    if(rc){fprintf(stderr,"DNS fixture %s failed %d\n",name,rc);assert(!rc);}assert(answer);
    int found=0;for(struct addrinfo *p=answer;p;p=p->ai_next){char text[INET6_ADDRSTRLEN];
        assert(p->ai_family==family&&p->ai_socktype==SOCK_STREAM&&p->ai_protocol==IPPROTO_TCP);
        const void *ip=family==AF_INET?(const void*)&((struct sockaddr_in*)p->ai_addr)->sin_addr:(const void*)&((struct sockaddr_in6*)p->ai_addr)->sin6_addr;
        assert(inet_ntop(family,ip,text,sizeof(text)));assert(ntohs(((struct sockaddr_in*)p->ai_addr)->sin_port)==443);
        found|=strcmp(text,expected)==0;
    }assert(found);__wrap_freeaddrinfo(answer);
}
static void *concurrent(void *unused){(void)unused;for(int i=0;i<20;++i)check("answer.test",AF_INET,"192.0.2.17");return NULL;}
int main(int argc,char **argv){
    if(argc>1&&strcmp(argv[1],"timeout")==0){struct addrinfo *p=NULL;int rc=__wrap_getaddrinfo("timeout.test","443",NULL,&p);assert(rc==EAI_AGAIN&&!p);puts("PASS bounded resolver timeout");return 0;}
    check("127.0.0.1",AF_INET,"127.0.0.1");check("::1",AF_INET6,"::1");
    check("answer.test",AF_INET,"192.0.2.17");check("answer.test",AF_INET6,"2001:db8::17");
    check("alias.test",AF_INET,"192.0.2.17");check("tcp.test",AF_INET,"192.0.2.17");check("wrong-id.test",AF_INET,"192.0.2.17");
    struct addrinfo hints={0},*p=(void*)1;hints.ai_family=AF_UNSPEC;hints.ai_socktype=SOCK_DGRAM;
    assert(__wrap_getaddrinfo("missing.test","0",&hints,&p)==EAI_NONAME&&!p);
    hints.ai_flags=AI_NUMERICHOST;assert(__wrap_getaddrinfo("answer.test","443",&hints,&p)==EAI_NONAME&&!p);
    hints.ai_flags=0;assert(__wrap_getaddrinfo("127.0.0.1","65536",&hints,&p)==EAI_SERVICE&&!p);
    hints.ai_flags=AI_PASSIVE;assert(!__wrap_getaddrinfo(NULL,"0",&hints,&p));__wrap_freeaddrinfo(p);
    empty_interfaces=1;hints.ai_flags=AI_ADDRCONFIG;hints.ai_family=AF_UNSPEC;
    assert(!__wrap_getaddrinfo("answer.test","443",&hints,&p)&&p);__wrap_freeaddrinfo(p);empty_interfaces=0;
    hints.ai_flags=AI_NUMERICHOST|AI_CANONNAME;hints.ai_family=AF_INET6;
    assert(!__wrap_getaddrinfo("fe80::1%23","443",&hints,&p)&&p);
    assert(((struct sockaddr_in6*)p->ai_addr)->sin6_scope_id==23&&p->ai_canonname);__wrap_freeaddrinfo(p);
    hints.ai_family=AF_INET;assert(__wrap_getaddrinfo("::1","443",&hints,&p)==EAI_NONAME&&!p);
    hints.ai_flags=1<<25;assert(__wrap_getaddrinfo("127.0.0.1","1",&hints,&p)==EAI_BADFLAGS&&!p);
    pthread_t threads[8];for(unsigned i=0;i<8;++i)assert(!pthread_create(&threads[i],NULL,concurrent,NULL));for(unsigned i=0;i<8;++i)pthread_join(threads[i],NULL);
#ifdef DKRR_TEST_NATIVE_NET
    const struct dkrr_net_counters n=dkrr_net_counters_read();
    assert(n.opened>160&&n.opened==n.closed&&n.epolls_opened==n.epolls_closed&&!n.close_errors);
#else
    dkrr_test_nonblocking_checked();
#endif
    puts("PASS native DNS with denied F_SETFL: numeric v4/v6/scope, UDP, CNAME, TCP fallback, ID rejection, NXDOMAIN, numeric-only, flags, passive bind, empty native interfaces and 160 concurrent lookups");
}
