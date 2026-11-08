// SPDX-License-Identifier: GPL-3.0-or-later
// payload_digest() must stay correct when the linked Mbed TLS has a larger
// SHA-256 context than this translation unit sees (the 1.0 updater bug).
#include "ps5_internet_update.hpp"
#include <cassert>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
using namespace dkrr::updates;
int main(int argc,char** argv) {
    assert(argc==3);
    int fd=open(argv[1],O_RDONLY);assert(fd>=0);
    Offer offer{};offer.bytes=static_cast<std::uint64_t>(lseek(fd,0,SEEK_END));offer.native_package=true;
    for(int i=0;i<32;++i){unsigned x=0;assert(std::sscanf(argv[2]+2*i,"%2x",&x)==1);offer.sha256[i]=static_cast<unsigned char>(x);}
    assert(payload_digest(fd,offer));
    offer.sha256[31]^=1;assert(!payload_digest(fd,offer));
    close(fd);std::puts("digest ok");
}
