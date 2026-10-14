// SPDX-License-Identifier: GPL-3.0-or-later
#include "recomp.h"
#include <cassert>
#include <cstdio>
#include <cstring>
extern "C" void dkr_european_language_cycle(uint8_t*,recomp_context*);
int main(){
    // Retail IDs English=0, German=1, French=2; original Options reverses on left.
    const unsigned expected[3][3]={{1,0,2},{2,1,0},{0,2,1}};
    unsigned count=0;
    for(unsigned language=0;language<3;++language)for(int direction=-1;direction<=1;++direction){
        recomp_context actual;std::memset(&actual,0x55,sizeof(actual));
        actual.r2=language;actual.r7=(uint64_t)(int64_t)direction;
        recomp_context before=actual;
        // The adapter must not access RDRAM or change any non-argument register.
        dkr_european_language_cycle(nullptr,&actual);
        assert(actual.r4==expected[language][direction+1]);
        before.r4=actual.r4;assert(std::memcmp(&actual,&before,sizeof(actual))==0);
        ++count;
    }
    recomp_context invalid={};invalid.r2=3;invalid.r7=1;
    dkr_european_language_cycle(nullptr,&invalid);assert(invalid.r4==0);
    std::printf("PASS: actual language bridge, %u valid transitions, invalid fallback, untouched registers and no RDRAM access\n",count);
}
