#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Compare actual PS5 mixing with pinned upstream, including gain-read counts."""
import hashlib,json,os,pathlib,subprocess,sys,tarfile,tempfile
r=pathlib.Path(__file__).resolve().parents[1]
if len(sys.argv)!=2:raise SystemExit('Usage: test_ps5_audio_block.py ABSOLUTE_GAME_WORK')
w=pathlib.Path(sys.argv[1]);game=w/'source/DKR-R/runtime-recomp/src/game'
s=(game/'runtime_platform.cpp').read_text();queue=s.split('void dkr::runtime::platform::queue_audio(',1)[1]
start=queue.index('#if defined(__PROSPERO__)\n        // Exact neutral stereo conversion');actual=queue[start:queue.index('        const auto byte_count',start)]
name='runtime-recomp/src/game/runtime_platform.cpp';spec=next(v for v in json.loads((r/'GAME_SOURCES.json').read_text())['sources'] if v.get('name')=='DKR-R');archive=w/'downloads'/spec['file'];assert hashlib.sha256(archive.read_bytes()).hexdigest()==spec['sha256']
with tarfile.open(archive) as a:
 matches=[m for m in a if m.isfile() and m.name.endswith('/'+name)];assert len(matches)==1;upstream=a.extractfile(matches[0]).read().decode()
start=upstream.index('        for (std::size_t i = 0; i < sample_count; i += 2) {\n            // RDRAM');legacy=upstream[start:upstream.index('        const auto byte_count',start)]
with tempfile.TemporaryDirectory(prefix='dkrr-block-gain-') as tmp:
 t=pathlib.Path(tmp);(t/'actual.inc').write_text(actual);(t/'legacy.inc').write_text(legacy)
 (t/'test.cpp').write_text('''#include "ps5_neutral_audio.h"
#include "audio_equalizer.hpp"
#include <algorithm>
#include <atomic>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>
struct CountedGain{float value=1;unsigned calls=0;float load(std::memory_order){++calls;return value;}}g_master_volume;
std::vector<std::int16_t> g_audio_swap_buffer;
std::vector<float> g_custom_music_buffer;
dkr::runtime::audio::StereoEqualizer g_audio_equalizer;
std::size_t g_ui_tone_frames_remaining=0,g_ui_tone_total_frames=0;
std::uint32_t g_audio_frequency=48000;float g_ui_tone_active_frequency_hz=440;double g_ui_tone_phase=0;
void actual(std::int16_t*samples,std::size_t sample_count,bool custom_music_active){
 #include "actual.inc"
}
void legacy(std::int16_t*samples,std::size_t sample_count,bool custom_music_active){
 #include "legacy.inc"
}
using Mixer=void(*)(std::int16_t*,std::size_t,bool);
std::vector<std::int16_t> sequence(Mixer mix,std::uint32_t rate,float gain,bool eq,bool music,bool tone){
 g_audio_frequency=rate;g_audio_equalizer={};g_audio_equalizer.configure(rate,eq?6:0,eq?-3:0,eq?4:0);
 g_ui_tone_phase=0;g_ui_tone_total_frames=g_ui_tone_frames_remaining=tone?2100:0;
 std::vector<std::int16_t> all;
 for(unsigned block=0;block<4;++block){
  const unsigned n=block==0?1440:block==1?2880:block==2?2:8192;
  std::vector<std::int16_t> samples(n);for(unsigned i=0;i<n;++i)samples[i]=static_cast<std::int16_t>((i*73+block*1583)&65535);
  g_audio_swap_buffer.resize(n);g_custom_music_buffer.resize(n);for(unsigned i=0;i<n;++i)g_custom_music_buffer[i]=float(int(i%39)-19)*211;
  // A later user change must be observed by the next submitted block.
  g_master_volume.value=block==3?1-gain:gain;g_master_volume.calls=0;
  mix(samples.data(),n,music);assert(g_master_volume.calls==(mix==actual?1:n/2));
  all.insert(all.end(),g_audio_swap_buffer.begin(),g_audio_swap_buffer.end());
 }
 return all;
}
int main(){
 unsigned cases=0;
 for(auto rate:{21600U,32000U,48000U})for(float gain:{0.0F,0.125F,0.5F,1.0F})
  for(bool eq:{false,true})for(bool music:{false,true})for(bool tone:{false,true}){
   auto old=sequence(legacy,rate,gain,eq,music,tone);auto now=sequence(actual,rate,gain,eq,music,tone);assert(old==now);++cases;
  }
 std::vector<std::int16_t> values(65536);for(unsigned i=0;i<values.size();++i)values[i]=static_cast<std::int16_t>(i);
 g_audio_swap_buffer.resize(values.size());g_audio_equalizer={};g_audio_equalizer.configure(48000,0,0,0);g_master_volume.value=1;g_master_volume.calls=0;g_ui_tone_frames_remaining=0;
 actual(values.data(),values.size(),false);assert(g_master_volume.calls==1);for(unsigned i=0;i<values.size();i+=2){assert(g_audio_swap_buffer[i]==values[i+1]&&g_audio_swap_buffer[i+1]==values[i]);}
 std::printf("PASS actual PS5 mixer: %u gain/EQ/music/tone/rate cases, multiple block sizes and next-block gain changes; bit-identical to pinned upstream at stable gain; one gain read/block; all int16 neutral values\\n",cases);
}
''')
 subprocess.run(['clang++','-std=c++20','-D__PROSPERO__','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I',str(t),'-I',str(r/'src'),'-I',str(game),str(t/'test.cpp'),str(game/'audio_equalizer.cpp'),'-o',str(t/'test')],check=True)
 subprocess.run([str(t/'test')],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
