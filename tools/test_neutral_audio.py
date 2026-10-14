#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Compare PS5 neutral copy with the actual upstream mixer loop; host benchmark."""
import pathlib,subprocess,sys,tempfile
if len(sys.argv)!=2:raise SystemExit('Usage: test_neutral_audio.py ABSOLUTE_GAME_WORK')
r=pathlib.Path(__file__).resolve().parent.parent;game=pathlib.Path(sys.argv[1])/'source/DKR-R/runtime-recomp/src/game'
s=(game/'runtime_platform.cpp').read_text();start=s.index('        for (std::size_t i = 0; i < sample_count; i += 2) {\n            // RDRAM');end=s.index('        const auto byte_count',start);loop=s[start:end]
assert 'dkrr_ps5_neutral_audio(' in s
with tempfile.TemporaryDirectory(prefix='dkrr-audio-test-') as tmp:
 t=pathlib.Path(tmp);(t/'actual_mixer.inc').write_text(loop)
 (t/'test.cpp').write_text('''#include "ps5_neutral_audio.h"
#include "audio_equalizer.hpp"
#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <vector>
std::vector<std::int16_t> g_audio_swap_buffer;
std::vector<float> g_custom_music_buffer;
dkr::runtime::audio::StereoEqualizer g_audio_equalizer;
std::atomic<float> g_master_volume{1.0F};
std::size_t g_ui_tone_frames_remaining=0,g_ui_tone_total_frames=0;
std::uint32_t g_audio_frequency=48000;
float g_ui_tone_active_frequency_hz=440;
double g_ui_tone_phase=0;
void original(std::int16_t* samples,std::size_t sample_count){
 bool custom_music_active=false;
 #include "actual_mixer.inc"
}
int main(){
 std::vector<std::int16_t> in(65536),out(in.size());
 for(std::size_t i=0;i<in.size();++i)in[i]=static_cast<std::int16_t>(static_cast<int>(i)-32768);
 g_audio_swap_buffer.resize(in.size());g_audio_equalizer.configure(48000,0,0,0);
 original(in.data(),in.size());assert(dkrr_ps5_neutral_audio(out.data(),in.data(),in.size(),true,1,false,false));assert(out==g_audio_swap_buffer);
 for(auto c:{1,2,3,4}){auto before=out;assert(!dkrr_ps5_neutral_audio(out.data(),in.data(),in.size(),c!=1,c==2?0.5F:1.0F,c==3,c==4));assert(before==out);}
 assert(!dkrr_ps5_neutral_audio(out.data(),in.data(),3,true,1,false,false));
 assert(dkrr_ps5_neutral_audio(nullptr,nullptr,0,true,1,false,false));
 // Fixed work, repeated windows; timings are descriptive, never a pass gate.
 std::vector<double> old_times,new_times;volatile std::int16_t sink=0;
 for(int window=0;window<11;++window){
  auto a=std::chrono::steady_clock::now();for(int n=0;n<100;++n){original(in.data(),in.size());sink=g_audio_swap_buffer[n];}
  auto b=std::chrono::steady_clock::now();for(int n=0;n<100;++n){dkrr_ps5_neutral_audio(out.data(),in.data(),in.size(),true,1,false,false);sink=out[n];}
  auto c=std::chrono::steady_clock::now();old_times.push_back(std::chrono::duration<double,std::milli>(b-a).count());new_times.push_back(std::chrono::duration<double,std::milli>(c-b).count());
 }
 std::sort(old_times.begin(),old_times.end());std::sort(new_times.begin(),new_times.end());
 std::printf("PASS actual neutral mixer parity all int16 values; enhanced/volume/tone guards. Host median per 100 x 65536 samples: original=%.3fms copy=%.3fms. No PS5/frame-rate claim.\\n",old_times[5],new_times[5]);(void)sink;
}
''')
 # Sanitized functional parity first; a separate optimized build measures work.
 for flags,name in ((['-O1','-fsanitize=address,undefined'],'sanitized'),(['-O3'],'optimized')):
  subprocess.run(['clang++','-std=c++20','-Wall','-Wextra','-Werror',*flags,'-I',str(r/'src'),'-I',str(game),'-I',str(t),str(t/'test.cpp'),str(game/'audio_equalizer.cpp'),'-o',str(t/name)],check=True)
  subprocess.run([str(t/name)],check=True)
