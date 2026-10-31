/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
int __wrap_getaddrinfo(const char*,const char*,const struct addrinfo*,struct addrinfo**);
void __wrap_freeaddrinfo(struct addrinfo*);
/* The native service supplies entropy internally; this path must not depend
 * on a separate c-ares initialization succeeding before the first request. */
int dkrr_rtc_entropy_available(void){return 0;}
int dkrr_secure_random(void *p,size_t n){return getentropy(p,n);}
static _Atomic int pools,resolvers,queries,pool_closes,resolver_closes,next_id=1;
static int fault;
int sceNetPoolCreate(const char *name,int bytes,int flags){assert(!strcmp(name,"dkrr-dns")&&bytes==0x4000&&!flags);atomic_fetch_add(&pools,1);return fault==1?-100:atomic_fetch_add(&next_id,1);}
int sceNetPoolDestroy(int id){assert(id>0);atomic_fetch_add(&pool_closes,1);return fault==6?-106:0;}
int sceNetResolverCreate(const char *name,int pool,int flags){assert(!strcmp(name,"dkrr-dns")&&pool>0&&!flags);atomic_fetch_add(&resolvers,1);return fault==2?-102:atomic_fetch_add(&next_id,1);}
int sceNetResolverDestroy(int id){assert(id>0);atomic_fetch_add(&resolver_closes,1);return fault==5?-105:0;}
int sceNetResolverStartNtoa(int id,const char *name,in_addr_t *out,int timeout,int retry,int flags){
 assert(id>0&&name&&timeout==2000000&&retry==1&&!flags);atomic_fetch_add(&queries,1);
 if(fault==3)return -103;
 assert(inet_pton(AF_INET,fault==4?"0.0.0.0":"192.0.2.17",out)==1);return 0;
}
static void check(int type,int protocol,int flags){
 struct addrinfo hints={0},*out=NULL;hints.ai_family=AF_UNSPEC;hints.ai_socktype=type;hints.ai_protocol=protocol;hints.ai_flags=flags;
 assert(!__wrap_getaddrinfo("answer.test","443",&hints,&out)&&out);int count=0;
 for(struct addrinfo *p=out;p;p=p->ai_next){
  struct sockaddr_in *a=(void*)p->ai_addr;assert(p->ai_family==AF_INET&&p->ai_addrlen==sizeof(*a)&&ntohs(a->sin_port)==443);
  assert(a->sin_addr.s_addr==htonl(0xc0000211));assert(p->ai_protocol==(p->ai_socktype==SOCK_DGRAM?IPPROTO_UDP:IPPROTO_TCP));
  if(type)assert(p->ai_socktype==type);if(protocol)assert(p->ai_protocol==protocol);++count;
 }
 assert(count==(type||protocol?1:2));assert(!(flags&AI_CANONNAME)||!strcmp(out->ai_canonname,"answer.test"));__wrap_freeaddrinfo(out);
}
static void *run(void *unused){(void)unused;for(int i=0;i<200;++i)check(SOCK_STREAM,IPPROTO_TCP,AI_CANONNAME);return NULL;}
int main(void){
 check(0,0,0);check(0,IPPROTO_UDP,0);check(SOCK_DGRAM,0,0);check(SOCK_STREAM,IPPROTO_TCP,AI_CANONNAME);
 pthread_t threads[8];for(int i=0;i<8;++i)assert(!pthread_create(&threads[i],0,run,0));for(int i=0;i<8;++i)pthread_join(threads[i],0);
 assert(pools==pool_closes&&resolvers==resolver_closes&&queries==1604);
 for(fault=1;fault<=6;++fault){int p=pools,r=resolvers,pc=pool_closes,rc=resolver_closes;struct addrinfo *out=(void*)1;
  assert(__wrap_getaddrinfo("answer.test","443",NULL,&out)==EAI_FAIL&&!out);
  assert(pools==p+1&&pool_closes==pc+(fault!=1));
  assert(resolvers==r+(fault!=1)&&resolver_closes==rc+(fault>2));
 }
 int n=queries;struct addrinfo *out=NULL,h={0};h.ai_family=AF_INET6;
 assert(__wrap_getaddrinfo("answer.test","443",&h,&out)==EAI_FAIL&&!out&&queries==n);
 h.ai_family=AF_INET;h.ai_flags=AI_NUMERICHOST;assert(__wrap_getaddrinfo("answer.test","443",&h,&out)==EAI_NONAME&&!out&&queries==n);
 assert(__wrap_getaddrinfo("answer.test","65536",NULL,&out)==EAI_SERVICE&&!out&&queries==n);
 puts("PASS actual native DNS path: 1604 owned lookups, TCP/UDP/canonical answers, correct microsecond timeout, native success before optional c-ares, all six resource failure branches, no IPv6/numeric-policy bypass");
}
