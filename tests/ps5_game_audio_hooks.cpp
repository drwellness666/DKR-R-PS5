// SPDX-License-Identifier: GPL-3.0-or-later
// Actual guest-call observers plus actual native context/routing callbacks.
#include "ps5_controller_audio_events.hpp"
#include "ps5_controller_lights.hpp"
#include "revision_addresses.hpp"
#include "recomp.h"
#include <array>
#include <atomic>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
static std::atomic<bool> g_speaker_enabled{true};
static dkrr::ps5_sounds::Mailbox g_controller_sounds;
static std::uint64_t now=1000;
static std::uint64_t ControllerLightTime(){return now;}
namespace dkr::runtime::netplay {static bool allowed=true;bool external_side_effects_allowed(){return allowed;}}
#include "sound_callbacks.inc"
static unsigned original_calls=0;static int nested=0;
static std::uint8_t* memory=nullptr;
extern "C" void __wrap_sound_play(std::uint8_t*,recomp_context*);
extern "C" void __wrap_music_jingle_play(std::uint8_t*,recomp_context*);
extern "C" void __wrap_sound_play_spatial(std::uint8_t*,recomp_context*);
extern "C" void __wrap_dkr_v80_sound_play(std::uint8_t*,recomp_context*);
extern "C" void __wrap_dkr_v80_music_jingle_play(std::uint8_t*,recomp_context*);
extern "C" void __wrap_dkr_v80_sound_play_spatial(std::uint8_t*,recomp_context*);
#define REAL(name,prefix) extern "C" void __real_##prefix##name(std::uint8_t*r,recomp_context*c){assert(r==memory);++original_calls;c->r2=0x12345678;}
#define SCOPE(name,prefix) extern "C" void __real_##prefix##name(std::uint8_t*r,recomp_context*c){assert(r==memory);++original_calls;recomp_context x{};x.r4=nested==1?0xa1:nested==2?47:0x23d;if(nested==1)__wrap_##prefix##sound_play(r,&x);if(nested==2)__wrap_##prefix##music_jingle_play(r,&x);if(nested==3)__wrap_##prefix##sound_play_spatial(r,&x);c->r2=0x12345678;}
#define REALS(p) REAL(racer_play_sound,p) REAL(sound_play,p) REAL(audspat_play_sound_at_position,p) REAL(music_jingle_play,p) REAL(sound_play_spatial,p) SCOPE(obj_loop_weaponballoon,p) SCOPE(obj_loop_silvercoin,p) SCOPE(obj_loop_goldenballoon,p)
REALS() REALS(dkr_v80_)
#define DECLARE(name,p) extern "C" void __wrap_##p##name(std::uint8_t*,recomp_context*);
#define DECLS(p) DECLARE(racer_play_sound,p) DECLARE(sound_play,p) DECLARE(audspat_play_sound_at_position,p) DECLARE(music_jingle_play,p) DECLARE(sound_play_spatial,p) DECLARE(obj_loop_weaponballoon,p) DECLARE(obj_loop_silvercoin,p) DECLARE(obj_loop_goldenballoon,p)
DECLS() DECLS(dkr_v80_)
using Fn=void(*)(std::uint8_t*,recomp_context*);
struct Calls{Fn horn,pickup,silver,gold,sound,jingle,spatial;};
static void word(std::uint32_t at,std::uint32_t v){for(unsigned i=0;i<4;++i)memory[((at+i)-0x80000000U)^3U]=std::uint8_t(v>>(24-i*8));}
static void half(std::uint32_t at,unsigned v){memory[(at-0x80000000U)^3U]=std::uint8_t(v>>8);memory[((at+1)-0x80000000U)^3U]=std::uint8_t(v);}
int main(){
 std::vector<std::uint8_t> ram(0x800000);memory=ram.data();std::array<std::uint64_t,4> seen{};
 const std::array<Calls,2> calls{{{__wrap_racer_play_sound,__wrap_obj_loop_weaponballoon,__wrap_obj_loop_silvercoin,__wrap_obj_loop_goldenballoon,__wrap_sound_play,__wrap_music_jingle_play,__wrap_sound_play_spatial},{__wrap_dkr_v80_racer_play_sound,__wrap_dkr_v80_obj_loop_weaponballoon,__wrap_dkr_v80_obj_loop_silvercoin,__wrap_dkr_v80_obj_loop_goldenballoon,__wrap_dkr_v80_sound_play,__wrap_dkr_v80_music_jingle_play,__wrap_dkr_v80_sound_play_spatial}}};
 for(unsigned revision=0;revision<2;++revision){
  namespace a=dkr::runtime::revision_addresses;a::select(revision?dkr::runtime::rom::Revision::UsV80:dkr::runtime::rom::Revision::UsV77);
  // Two local racers; an AI and invalid object cannot emit a personal cue.
  word(a::GameMode,0);word(a::IsInTracksMenu,0);word(a::IsPaused&~3U,0);word(a::LevelLoadTimer,0);word(a::NumberOfRacers,4);word(a::RacersByPort,0x80001000);
  for(unsigned p=0;p<4;++p){auto o=0x80002000+p*0x100,r=0x80003000+p*0x200;word(0x80001000+4*p,o);word(o+0x64,r);half(r,p<2?p:0xffff);word(o+0xc,0x3f800000+p);word(o+0x10,0x40000000);word(o+0x14,0x40400000);}
  auto take=[&](unsigned p){return g_controller_sounds.take(p,now,false,seen[p]);};
  auto invoke=[&](Fn f,unsigned x,unsigned y=0){recomp_context c{};c.r4=x;c.r5=y;c.r8=0xdeadbeef;f(memory,&c);assert(c.r4==x&&c.r5==y&&c.r8==0xdeadbeef&&c.r2==0x12345678);now+=200;};
  const auto& c=calls[revision];invoke(c.horn,0x80002100,0x158);assert(take(1)==-1); // event age >150ms expires
  now+=200;recomp_context context{};context.r4=0x80002100;context.r5=0x158;c.horn(memory,&context);assert(take(1)==11&&take(0)==-1&&take(1)==-1);
  now+=200;context.r4=0x80002000;context.r5=0x107;c.horn(memory,&context);assert(take(0)==23);
  now+=200;nested=1;context={};context.r4=0x80004000;word(0x8000404c,0x80004100);word(0x80004100,0x80002000);c.pickup(memory,&context);assert(take(0)==20);
  now+=200;nested=2;c.silver(memory,&context);assert(take(0)==28);
  now+=200;nested=3;c.gold(memory,&context);assert(take(0)==22);
  now+=200;context={};context.r4=0xa1;c.sound(memory,&context);assert(take(0)==-1); // TLS scope restored
  now+=200;context.r4=47;c.jingle(memory,&context);assert(take(0)==-1); // unrelated global jingle
  context={};context.r4=0x107;context.r5=0x3f800001;context.r6=0x40000000;context.r7=0x40400000;c.spatial(memory,&context);assert(take(1)==23&&take(0)==-1);
  for(unsigned block=0;block<5;++block){now+=200;if(block==0)g_speaker_enabled=false;if(block==1)word(a::GameMode,1);if(block==2)word(a::LevelLoadTimer,1);if(block==3)word(a::IsInTracksMenu,1);if(block==4)dkr::runtime::netplay::allowed=false;
   context={};context.r4=0x80002000;context.r5=0x156;c.horn(memory,&context);assert(take(0)==-1);
   g_speaker_enabled=true;word(a::GameMode,0);word(a::LevelLoadTimer,0);word(a::IsInTracksMenu,0);dkr::runtime::netplay::allowed=true;
  }
  now+=200;memory[(a::IsPaused-0x80000000)^3]=1;context.r4=0x80002000;c.horn(memory,&context);assert(take(0)==-1);memory[(a::IsPaused-0x80000000)^3]=0;
  for(unsigned object:{0U,0x80002001U,0x807ffffcU,0x80002200U}){now+=200;context.r4=object;c.horn(memory,&context);assert(take(0)==-1&&take(2)==-1);}
 }
 dkrr::ps5_sounds::Mailbox box;std::uint64_t token=0;box.publish(0,9,100);assert(box.take(0,100,false,token)==9);box.publish(0,9,120);assert(box.take(0,120,false,token)==-1);box.publish(0,9,300);assert(box.take(0,300,true,token)==-1);assert(box.take(0,301,false,token)==-1);box.publish(0,9,600);assert(box.take(0,751,false,token)==-1);assert(original_calls>30);
 puts("PASS actual US77/US80 sound observers: original register flow, scoped collectible ownership, horns/jingles/zipper, local players only, pause/loading/menu/replay suppression, stale/deduplicated mailbox");
}
