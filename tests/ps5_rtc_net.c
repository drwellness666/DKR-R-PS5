/* SPDX-License-Identifier: GPL-3.0-or-later */
#define _POSIX_C_SOURCE 200809L
#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
extern int __wrap_getnameinfo(const struct sockaddr *,socklen_t,char *,socklen_t,char *,socklen_t,int);
static void compare(const struct sockaddr *address,socklen_t length) {
    char host[80],service[10],expected_host[80],expected_service[10];
    for(unsigned outputs=1;outputs<4;++outputs) {
        for(unsigned datagram=0;datagram<2;++datagram) {
            int flags=NI_NUMERICHOST|NI_NUMERICSERV|(datagram?NI_DGRAM:0);
            memset(host,0xa5,sizeof(host));memset(service,0xa5,sizeof(service));
            assert(getnameinfo(address,length,outputs&1?expected_host:NULL,sizeof(expected_host),outputs&2?expected_service:NULL,sizeof(expected_service),flags)==0);
            assert(__wrap_getnameinfo(address,length,outputs&1?host:NULL,sizeof(host),outputs&2?service:NULL,sizeof(service),flags)==0);
            if(outputs&1)assert(!strcmp(host,expected_host));
            if(outputs&2)assert(!strcmp(service,expected_service));
        }
    }
}
int main(void) {
    struct sockaddr_in ipv4={0};ipv4.sin_family=AF_INET;
    assert(inet_pton(AF_INET,"192.0.2.176",&ipv4.sin_addr)==1);
    for(unsigned port=0;port<=65535;++port) {
        ipv4.sin_port=htons((unsigned short)port);compare((const struct sockaddr *)&ipv4,sizeof(ipv4));
    }
    const char *addresses[]={"::","::1","::ffff:192.0.2.176","2001:db8::abc","fe80::1234"};
    for(unsigned i=0;i<sizeof(addresses)/sizeof(*addresses);++i) {
        struct sockaddr_in6 ipv6={0};ipv6.sin6_family=AF_INET6;ipv6.sin6_port=htons(65535);
        assert(inet_pton(AF_INET6,addresses[i],&ipv6.sin6_addr)==1);
        compare((const struct sockaddr *)&ipv6,sizeof(ipv6));
        ipv6.sin6_scope_id=0x12345678;compare((const struct sockaddr *)&ipv6,sizeof(ipv6));
    }
    char host[80]="unchanged",service[10]="unchanged";
    assert(__wrap_getnameinfo((const struct sockaddr *)&ipv4,sizeof(ipv4),host,2,service,sizeof(service),NI_NUMERICHOST|NI_NUMERICSERV)==EAI_OVERFLOW);
    assert(!strcmp(host,"unchanged")&&!strcmp(service,"unchanged"));
    assert(__wrap_getnameinfo((const struct sockaddr *)&ipv4,sizeof(ipv4),host,sizeof(host),service,2,NI_NUMERICHOST|NI_NUMERICSERV)==EAI_OVERFLOW);
    assert(!strcmp(host,"unchanged")&&!strcmp(service,"unchanged"));
    unsigned char tiny=0;
    assert(__wrap_getnameinfo((const struct sockaddr *)&tiny,1,host,sizeof(host),NULL,0,NI_NUMERICHOST)==EAI_FAMILY);
    assert(__wrap_getnameinfo(NULL,0,host,sizeof(host),NULL,0,NI_NUMERICHOST)==EAI_FAMILY);
    assert(__wrap_getnameinfo((const struct sockaddr *)&ipv4,2,host,sizeof(host),NULL,0,NI_NUMERICHOST)==EAI_FAMILY);
    assert(__wrap_getnameinfo((const struct sockaddr *)&ipv4,sizeof(ipv4),host,sizeof(host),NULL,0,NI_NAMEREQD)==EAI_BADFLAGS);
    assert(__wrap_getnameinfo((const struct sockaddr *)&ipv4,sizeof(ipv4),host,sizeof(host),NULL,0,0)==EAI_NONAME);
    assert(__wrap_getnameinfo((const struct sockaddr *)&ipv4,sizeof(ipv4),NULL,0,NULL,0,0)==EAI_NONAME);
    puts("PASS numeric RTC address formatting versus system resolver: IPv4 ports 0..65535, IPv6/scope, host-only, service-only, bounded atomic outputs and invalid-address/flag rejection");
}
