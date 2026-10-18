// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5_storage_budget.h"
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
static unsigned mode;
extern "C" int __wrap_statvfs(const char*,struct statvfs* out) noexcept {
    std::memset(out,0,sizeof(*out));
    if(mode==1){errno=ENOSYS;return -1;}
    if(mode==2)return 0;
    out->f_frsize=512;out->f_bavail=8;
    if(mode==3){out->f_bavail=UINT64_MAX;out->f_frsize=4096;}
    if(mode==4){out->f_bsize=4096;out->f_frsize=0;out->f_bavail=1;}
    return 0;
}
int main() {
    uint64_t available{};
    assert(dkrr_space_check("/owned",4096,&available)==1&&available==4096);
    assert(dkrr_space_check("/owned",4097,&available)==-1&&available==4096);
    mode=1;assert(dkrr_space_check("/owned",1,&available)==0&&available==0);
    mode=2;assert(dkrr_space_check("/owned",1,&available)==0&&available==0);
    mode=3;assert(dkrr_space_check("/owned",UINT64_MAX,&available)==1&&available==UINT64_MAX);
    mode=4;assert(dkrr_space_check("/owned",4096,&available)==1&&available==4096);
    std::puts("PASS storage telemetry: exact-space boundary, shortage, unavailable/invalid telemetry, multiplication saturation and block-size fallback");
}
