#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Compile the actual PS5 reserve/resize operations with a counted allocator."""
import pathlib,subprocess,sys,tempfile
root=pathlib.Path(__file__).resolve().parents[1]
if len(sys.argv)!=2:raise SystemExit('Usage: test_ps5_audio_scratch.py ABSOLUTE_GAME_WORK')
s=(pathlib.Path(sys.argv[1])/'source/DKR-R/runtime-recomp/src/game/runtime_platform.cpp').read_text()
setup=s.split('    g_audio_frequency = static_cast<std::uint32_t>(obtained.freq);\n',1)[1].split('    g_audio_callback_frames =',1)[0]
queue=s.split('void dkr::runtime::platform::queue_audio(',1)[1].split('void dkr::runtime::platform::set_audio_frequency(',1)[0]
resize=next(line.strip() for line in queue.splitlines() if 'g_audio_swap_buffer.resize(' in line)
music=next(line.strip() for line in queue.splitlines() if 'g_custom_music_buffer.assign(' in line)
with tempfile.TemporaryDirectory(prefix='dkrr-audio-scratch-') as tmp:
 t=pathlib.Path(tmp)
 (t/'setup.inc').write_text(setup)
 (t/'test.cpp').write_text('''#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <vector>
static unsigned allocations=0;
template<class T> struct Counted {
 using value_type=T;
 Counted()=default;template<class U> Counted(const Counted<U>&){}
 T* allocate(std::size_t n){++allocations;return std::allocator<T>{}.allocate(n);}
 void deallocate(T* p,std::size_t n){std::allocator<T>{}.deallocate(p,n);}
 template<class U> bool operator==(const Counted<U>&)const{return true;}
};
std::vector<std::int16_t,Counted<std::int16_t>> g_audio_swap_buffer;
std::vector<float,Counted<float>> g_custom_music_buffer;
std::uint32_t g_audio_frequency;
void initialize(std::uint32_t rate){g_audio_frequency=rate;
 #include "setup.inc"
}
void deliver(std::size_t sample_count){
'''+resize+'\n'+music+'''
 assert(g_audio_swap_buffer.size()==sample_count);
 assert(g_custom_music_buffer.size()==sample_count);
 for(float value:g_custom_music_buffer)assert(value==0.0F);
 // The next custom-music block must start at silence, including retained storage.
 std::fill(g_custom_music_buffer.begin(),g_custom_music_buffer.end(),0.5F);
}
int main(){
 for(auto rate:{21600U,32000U,48000U,96000U,22050U}){
  initialize(rate);const unsigned before=allocations;
  const std::size_t limit=static_cast<std::size_t>(std::max(rate,48000U))*2U;
  assert(g_audio_swap_buffer.capacity()>=limit);
  assert(g_custom_music_buffer.capacity()>=limit);
  for(std::size_t count:{0U,2U,1440U,2880U,8192U})deliver(count);
  deliver(limit);deliver(2);deliver(limit-2);deliver(limit);
  for(std::size_t i=0;i<127;++i)deliver((i*3571U%limit)&~std::size_t(1));
  assert(allocations==before);
 }
 puts("PASS actual PS5 audio setup/delivery scratch operations: no vector allocation for accepted blocks; rate changes and zeroed custom-music reuse");
}
''')
 subprocess.run(['clang++','-std=c++20','-D__PROSPERO__','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I',str(t),str(t/'test.cpp'),'-o',str(t/'test')],check=True)
 subprocess.run([str(t/'test')],check=True)
