// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5_neutral_audio.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>
int main(){
 std::vector<std::int16_t> source(65536),out(source.size());
 for(int i=0;i<65536;++i)source[i]=std::int16_t(i-32768);
 assert(dkrr_ps5_neutral_audio(out.data(),source.data(),source.size(),true,1,false,false));
 for(std::size_t i=0;i<source.size();i+=2){assert(out[i]==std::lround(float(source[i+1])));assert(out[i+1]==std::lround(float(source[i])));}
 auto previous=out;
 assert(!dkrr_ps5_neutral_audio(out.data(),source.data(),source.size(),false,1,false,false));
 assert(!dkrr_ps5_neutral_audio(out.data(),source.data(),source.size(),true,0.5F,false,false));
 assert(!dkrr_ps5_neutral_audio(out.data(),source.data(),source.size(),true,1,true,false));
 assert(!dkrr_ps5_neutral_audio(out.data(),source.data(),source.size(),true,1,false,true));
 assert(!dkrr_ps5_neutral_audio(out.data(),source.data(),3,true,1,false,false));assert(out==previous);
 assert(dkrr_ps5_neutral_audio(nullptr,nullptr,0,true,1,false,false));
 puts("PASS neutral-audio adapter all int16 outputs, stereo order and enhanced/fallback guards");
}
