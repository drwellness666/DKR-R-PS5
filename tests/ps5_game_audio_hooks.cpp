// SPDX-License-Identifier: GPL-3.0-or-later
// Actual guest-call observers plus actual native context/routing callbacks.
#include "ps5_controller_audio_events.hpp"
#include "ps5_custom_horn.hpp"
#include "ps5_controller_lights.hpp"
#include "revision_addresses.hpp"
#include "recomp.h"
#include <array>
#include <atomic>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
#include <thread>
static std::atomic<bool> g_speaker_enabled{true};
static dkrr::ps5_sounds::Mailbox g_controller_sounds;
static std::uint64_t now=1000;
static std::uint64_t ControllerLightTime(){return now;}
namespace dkr::runtime::netplay {static bool allowed=true;bool external_side_effects_allowed(){return allowed;}}
#include "sound_callbacks.inc"
static unsigned original_calls=0;static int nested=0;static unsigned jingle_id=47,pickup_id=0xa1;
static std::uint8_t* memory=nullptr;
extern "C" void __wrap_sound_play(std::uint8_t*,recomp_context*);
extern "C" void __wrap_music_jingle_play(std::uint8_t*,recomp_context*);
extern "C" void __wrap_sound_play_spatial(std::uint8_t*,recomp_context*);
extern "C" void __wrap_dkr_v80_sound_play(std::uint8_t*,recomp_context*);
extern "C" void __wrap_dkr_v80_music_jingle_play(std::uint8_t*,recomp_context*);
extern "C" void __wrap_dkr_v80_sound_play_spatial(std::uint8_t*,recomp_context*);
#define REAL(name,prefix) extern "C" void __real_##prefix##name(std::uint8_t*r,recomp_context*c){assert(r==memory);++original_calls;c->r2=0x12345678;}
#define SCOPE(name,prefix) extern "C" void __real_##prefix##name(std::uint8_t*r,recomp_context*c){assert(r==memory);++original_calls;recomp_context x{};x.r4=nested==1?pickup_id:nested==2?jingle_id:0x23d;if(nested==1)__wrap_##prefix##sound_play(r,&x);if(nested==2)__wrap_##prefix##music_jingle_play(r,&x);if(nested==3)__wrap_##prefix##sound_play_spatial(r,&x);c->r2=0x12345678;}
static bool racer_allows_sound=true;
#define RACER(p) extern "C" void __real_##p##racer_play_sound(std::uint8_t*r,recomp_context*c){assert(r==memory);++original_calls;const dkrr::ps5_lights::Guest g({r,0x800000});if(racer_allows_sound&&g.valid(std::uint32_t(c->r4),0x18)){recomp_context x{};x.r4=c->r5;x.r5=g.word(std::uint32_t(c->r4)+0xc);x.r6=g.word(std::uint32_t(c->r4)+0x10);x.r7=g.word(std::uint32_t(c->r4)+0x14);__wrap_##p##sound_play_spatial(r,&x);}c->r2=0x12345678;}
#define HUD(p) extern "C" void __real_##p##hud_silver_coins(std::uint8_t*r,recomp_context*c){assert(r==memory);++original_calls;if(nested==4){recomp_context x{};x.r4=0x147;__wrap_##p##sound_play(r,&x);}c->r2=0x12345678;}
#define REALS(p) HUD(p) RACER(p) REAL(sound_play,p) REAL(audspat_play_sound_at_position,p) REAL(music_jingle_play,p) REAL(sound_play_spatial,p) SCOPE(obj_loop_weaponballoon,p) SCOPE(obj_loop_silvercoin,p) SCOPE(obj_loop_goldenballoon,p)
REALS() REALS(dkr_v80_)
#define DECLARE(name,p) extern "C" void __wrap_##p##name(std::uint8_t*,recomp_context*);
#define DECLS(p) DECLARE(hud_silver_coins,p) DECLARE(racer_play_sound,p) DECLARE(sound_play,p) DECLARE(audspat_play_sound_at_position,p) DECLARE(music_jingle_play,p) DECLARE(sound_play_spatial,p) DECLARE(obj_loop_weaponballoon,p) DECLARE(obj_loop_silvercoin,p) DECLARE(obj_loop_goldenballoon,p)
DECLS() DECLS(dkr_v80_)
using Fn=void(*)(std::uint8_t*,recomp_context*);
struct Calls{Fn horn,pickup,silver,gold,sound,jingle,spatial,hud;};
static void word(std::uint32_t at,std::uint32_t v){for(unsigned i=0;i<4;++i)memory[((at+i)-0x80000000U)^3U]=std::uint8_t(v>>(24-i*8));}
static void half(std::uint32_t at,unsigned v){memory[(at-0x80000000U)^3U]=std::uint8_t(v>>8);memory[((at+1)-0x80000000U)^3U]=std::uint8_t(v);}
int main(){
 std::vector<std::uint8_t> ram(0x800000);memory=ram.data();std::array<std::uint64_t,4> seen{};
 const std::array<Calls,2> calls{{{__wrap_racer_play_sound,__wrap_obj_loop_weaponballoon,__wrap_obj_loop_silvercoin,__wrap_obj_loop_goldenballoon,__wrap_sound_play,__wrap_music_jingle_play,__wrap_sound_play_spatial,__wrap_hud_silver_coins},{__wrap_dkr_v80_racer_play_sound,__wrap_dkr_v80_obj_loop_weaponballoon,__wrap_dkr_v80_obj_loop_silvercoin,__wrap_dkr_v80_obj_loop_goldenballoon,__wrap_dkr_v80_sound_play,__wrap_dkr_v80_music_jingle_play,__wrap_dkr_v80_sound_play_spatial,__wrap_dkr_v80_hud_silver_coins}}};
 for(unsigned revision=0;revision<2;++revision){
  namespace a=dkr::runtime::revision_addresses;a::select(revision?dkr::runtime::rom::Revision::UsV80:dkr::runtime::rom::Revision::UsV77);
  // Two local racers; an AI and invalid object cannot emit a personal cue.
  word(a::GameMode,0);word(a::IsInTracksMenu,0);word(a::IsPaused&~3U,0);word(a::LevelLoadTimer,0);word(a::NumberOfRacers,4);word(a::RacersByPort,0x80001000);
  for(unsigned p=0;p<4;++p){auto o=0x80002000+p*0x100,r=0x80003000+p*0x200;word(0x80001000+4*p,o);word(o+0x64,r);half(o+0x48,1);half(r,p<2?p:0xffff);word(o+0xc,0x3f800000+p);word(o+0x10,0x40000000);word(o+0x14,0x40400000);}
  auto take=[&](unsigned p){return g_controller_sounds.take(p,now,false,seen[p]);};
  auto invoke=[&](Fn f,unsigned x,unsigned y=0){recomp_context c{};c.r4=x;c.r5=y;c.r8=0xdeadbeef;f(memory,&c);assert(c.r4==x&&c.r5==y&&c.r8==0xdeadbeef&&c.r2==0x12345678);now+=200;};
  const auto& c=calls[revision];invoke(c.horn,0x80002100,0x158);assert(take(1)==-1); // event age >150ms expires
  now+=200;recomp_context context{};context.r4=0x80002100;context.r5=0x158;c.horn(memory,&context);assert(take(1)==11&&take(0)==-1&&take(1)==-1);
  now+=200;context.r4=0x80002000;context.r5=0x107;c.horn(memory,&context);assert(take(0)==23);
  // Every actual original horn start, including fast repeats, is delivered.
  for(unsigned repeat=0;repeat<6;++repeat){now+=33;context.r5=0x156;c.horn(memory,&context);assert(take(0)==9&&take(0)==-1);}
  racer_allows_sound=false;now+=33;c.horn(memory,&context);assert(take(0)==-1);racer_allows_sound=true;

  context={};context.r4=0x80004000;word(0x8000404c,0x80004100);word(0x80004100,0x80002000);
  for(unsigned level=0;level<3;++level){now+=200;nested=1;pickup_id=0xa0+level;c.pickup(memory,&context);const auto clip=take(0);assert(clip==int(dkrr::ps5_sounds::pickup+level));assert(dkrr::ps5_sounds::allowed(1U<<6,unsigned(clip)));assert(!dkrr::ps5_sounds::allowed(1023U&~(1U<<6),unsigned(clip)));}
  for(unsigned i=0;i<8;++i){now+=200;nested=2;jingle_id=43+i;c.silver(memory,&context);const auto clip=take(0);assert(clip==int(dkrr::ps5_sounds::silver+i));assert(dkrr::ps5_sounds::allowed(1U<<7,unsigned(clip)));assert(!dkrr::ps5_sounds::allowed(1023U&~(1U<<7),unsigned(clip)));}
  // T.T. starts later from the original HUD, only after all eight coins.
  now+=200;nested=4;context={};context.r4=0x80003000;memory[(0x3202)^3]=8;c.hud(memory,&context);assert(take(0)==int(dkrr::ps5_sounds::go_for_it)&&take(1)==-1);
  memory[(0x3202)^3]=7;now+=200;c.hud(memory,&context);assert(take(0)==-1);
  context.r4=0x80003200;memory[(0x3402)^3]=8;now+=200;c.hud(memory,&context);assert(take(1)==int(dkrr::ps5_sounds::go_for_it)&&take(0)==-1);
  context={};context.r4=0x147;now+=200;c.sound(memory,&context);assert(take(0)==-1&&take(1)==-1); // Other T.T. uses outside the challenge are not mirrored.
  context.r4=0x80004000;
  now+=200;nested=3;c.gold(memory,&context);assert(take(0)==22);
  now+=200;context={};context.r4=0xa1;c.sound(memory,&context);assert(take(0)==-1); // TLS scope restored
  now+=200;context.r4=47;c.jingle(memory,&context);assert(take(0)==-1); // unrelated global jingle
  context={};context.r4=0x107;context.r5=0x3f800001;context.r6=0x40000000;context.r7=0x40400000;c.spatial(memory,&context);assert(take(1)==23&&take(0)==-1);
  // Both original zipper call paths, for every vehicle and retail revision.
  for(unsigned vehicle=0;vehicle<3;++vehicle){
   memory[((0x80003000+0x1d6)-0x80000000)^3]=vehicle;now+=200;
   context={};context.r4=0x107;context.r5=0x3f800000;context.r6=0x40000000;context.r7=0x40400000;
   c.spatial(memory,&context);assert(take(0)==23&&take(1)==-1);
   now+=200;context={};context.r4=0x80002000;context.r5=0x107;c.horn(memory,&context);assert(take(0)==23);
  }
  now+=200;memory[(a::CutsceneCameraActive-0x80000000)^3]=1;
  context={};context.r4=0x80002000;context.r5=0x107;c.horn(memory,&context);assert(take(0)==-1);memory[(a::CutsceneCameraActive-0x80000000)^3]=0;
  memory[(a::RaceEndStage-0x80000000)^3]=4;half(a::RaceEndTimer,0xffff);now+=200;c.horn(memory,&context);assert(take(0)==-1);
  half(a::RaceEndTimer,0);now+=200;context.r4=0x80002000;context.r5=0x107;c.horn(memory,&context);assert(take(0)==23);memory[(a::RaceEndStage-0x80000000)^3]=0;

#if DKRR_NATIVE_CUSTOM_HORN_AVAILABLE
  // The real native callback accepts an admitted mod bank for the owning
  // controller only. No guessed retail ID or character-name allowlist.
  now+=200;auto bytes=std::make_shared<const std::vector<std::uint8_t>>(16,0);
  dkrr::ps5_speaker::HornBank bank{bytes,*bytes,*bytes,1,127,100};
  dkrr_controller_mod_horn(memory,0,bank);
  assert(take(0)==int(dkrr::ps5_sounds::custom_horn)&&take(1)==-1);
  const auto admitted=dkrr::ps5_speaker::custom_horns[0].load();assert(admitted&&admitted->owner==bytes);
  now+=200;dkrr_controller_mod_horn(memory,0,bank);assert(take(0)==int(dkrr::ps5_sounds::custom_horn));
  assert(dkrr::ps5_speaker::custom_horns[0].load()==admitted);
  now+=200;dkr::runtime::netplay::allowed=false;dkrr_controller_mod_horn(memory,0,bank);assert(take(0)==-1);dkr::runtime::netplay::allowed=true;

#endif
  for(unsigned block=0;block<5;++block){now+=200;if(block==0)g_speaker_enabled=false;if(block==1)word(a::GameMode,1);if(block==2)word(a::LevelLoadTimer,1);if(block==3)word(a::IsInTracksMenu,1);if(block==4)dkr::runtime::netplay::allowed=false;
   context={};context.r4=0x80002000;context.r5=0x156;c.horn(memory,&context);assert(take(0)==-1);
   g_speaker_enabled=true;word(a::GameMode,0);word(a::LevelLoadTimer,0);word(a::IsInTracksMenu,0);dkr::runtime::netplay::allowed=true;
  }
  now+=200;memory[(a::IsPaused-0x80000000)^3]=1;context.r4=0x80002000;c.horn(memory,&context);assert(take(0)==-1);memory[(a::IsPaused-0x80000000)^3]=0;
  for(unsigned object:{0U,0x80002001U,0x807ffffcU,0x80002200U}){now+=200;context.r4=object;c.horn(memory,&context);assert(take(0)==-1&&take(2)==-1);}
 }
 dkrr::ps5_sounds::Mailbox box;std::uint64_t token=0;
 assert(box.publish(0,9,100));assert(box.take(0,100,false,token)==9);
 assert(box.publish(0,9,120));assert(box.take(0,120,false,token)==9);
 assert(box.publish(0,9,120));assert(box.take(0,120,false,token)==9); // Two real starts need distinct records even at same timestamp.
 box.publish(0,9,300);assert(box.take(0,300,true,token)==-1);assert(box.take(0,301,false,token)==-1);
 box.publish(0,9,600);assert(box.take(0,751,false,token)==-1);assert(original_calls>30);
 box.publish(1,dkrr::ps5_sounds::zipper,1000);box.publish(1,dkrr::ps5_sounds::pickup,1001);
 assert(box.take(1,1002,false,token)==int(dkrr::ps5_sounds::zipper));assert(box.take(1,1002,false,token)==int(dkrr::ps5_sounds::pickup));
 for(unsigned i=0;i<32;++i)assert(box.publish(2,i,2000));assert(!box.publish(2,9,2000));
 for(unsigned i=0;i<32;++i)assert(box.take(2,2001,false,token)==int(i));assert(box.take(2,2001,false,token)==-1);
 assert(box.publish(2,9,2002)&&box.take(2,2002,false,token)==9);
 // Bounded concurrent publication keeps every accepted live event exactly once.
 std::atomic<unsigned> accepted{0};std::array<std::thread,4> producers;
 for(unsigned i=0;i<4;++i)producers[i]=std::thread([&,i]{for(unsigned j=0;j<8;++j)if(box.publish(3,i*8+j,3000))++accepted;});
 for(auto&t:producers)t.join();std::array<bool,32> received{};unsigned taken=0;int cue;
 while((cue=box.take(3,3000,false,token))>=0){assert(!received[cue]);received[cue]=true;++taken;}assert(taken==accepted&&taken>0);
 puts("PASS actual US77/US80 observers: all eight silver-coin starts/category gates and original HUD T.T. completion ownership, original spatial starts and gates, no duplicate zipper, rapid/same-timestamp horns, simultaneous selected cues, 32-event bounded burst, concurrent publication, age/pause/ownership guards and untouched original register flow");
}
