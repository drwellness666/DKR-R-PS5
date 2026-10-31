// SPDX-License-Identifier: GPL-3.0-or-later
// Observe actual retail audio calls. Never alter guest registers, saves or audio.
#include "ps5_controller_audio_events.hpp"
#include "ps5_controller_lights.hpp"
#include "recomp.h"
#include <cstdint>
extern "C" bool dkrr_controller_sounds_enabled();
extern "C" void dkrr_controller_game_sound(const std::uint8_t*,unsigned,unsigned);
namespace {
using dkrr::ps5_lights::Guest;
thread_local int owner=-1;
int racer_owner(const Guest& g,std::uint32_t object){
    if((object&3U)||!g.valid(object,0x68))return -1;
    const auto racer=g.word(object+0x64);
    if((racer&3U)||!g.valid(racer,4))return -1;
    const unsigned port=(unsigned(g.byte(racer))<<8)|g.byte(racer+1);
    return port<4?int(port):-1;
}
int interaction_owner(const std::uint8_t* ram,std::uint32_t object){
    const Guest g({ram,0x800000});if((object&3U)||!g.valid(object,0x50))return -1;
    const auto interaction=g.word(object+0x4c);
    return (interaction&3U)||!g.valid(interaction,4)?-1:racer_owner(g,g.word(interaction));
}
struct Scope {int old;Scope(int value):old(owner){owner=value;}~Scope(){owner=old;}};
void emit(const std::uint8_t* ram,int port,unsigned clip){if(port>=0&&port<4)dkrr_controller_game_sound(ram,unsigned(port),clip);}
}
// The source symbols are weak, with the v80 payload's explicit namespace.
// --wrap delegates to each matching original, preserving its entire context.
#define OBJECT_SCOPE(name,prefix) \
 extern "C" void __real_##prefix##name(std::uint8_t*,recomp_context*); \
 extern "C" void __wrap_##prefix##name(std::uint8_t* r,recomp_context* c){Scope scope(dkrr_controller_sounds_enabled()?interaction_owner(r,std::uint32_t(c->r4)):-1);__real_##prefix##name(r,c);}
#define SOUND_HOOKS(prefix) \
 OBJECT_SCOPE(obj_loop_weaponballoon,prefix) \
 OBJECT_SCOPE(obj_loop_silvercoin,prefix) \
 OBJECT_SCOPE(obj_loop_goldenballoon,prefix) \
 extern "C" void __real_##prefix##hud_silver_coins(std::uint8_t*,recomp_context*); \
 extern "C" void __wrap_##prefix##hud_silver_coins(std::uint8_t*r,recomp_context*c){ \
  const Guest g({r,0x800000});const auto racer=std::uint32_t(c->r4); \
  const int port=!(racer&3U)&&g.valid(racer,0x203)&&g.byte(racer+0x202)==8?int((unsigned(g.byte(racer))<<8)|g.byte(racer+1)):-1; \
  Scope scope(dkrr_controller_sounds_enabled()&&port>=0&&port<4?port:-1);__real_##prefix##hud_silver_coins(r,c); } \
 extern "C" void __real_##prefix##racer_play_sound(std::uint8_t*,recomp_context*); \
 extern "C" void __wrap_##prefix##racer_play_sound(std::uint8_t*r,recomp_context*c){ \
  const auto id=std::uint32_t(c->r5); \
  if(id>=0x156&&id<=0x15f&&dkrr_controller_sounds_enabled()){const Guest g({r,0x800000});Scope scope(racer_owner(g,std::uint32_t(c->r4)));__real_##prefix##racer_play_sound(r,c);} \
  else __real_##prefix##racer_play_sound(r,c); } \
 extern "C" void __real_##prefix##sound_play(std::uint8_t*,recomp_context*); \
 extern "C" void __wrap_##prefix##sound_play(std::uint8_t*r,recomp_context*c){ \
  const auto id=std::uint32_t(c->r4);if(id>=0xa0&&id<=0xa2)emit(r,owner,dkrr::ps5_sounds::pickup+id-0xa0);if(id==0x147)emit(r,owner,dkrr::ps5_sounds::go_for_it);__real_##prefix##sound_play(r,c); } \
 extern "C" void __real_##prefix##audspat_play_sound_at_position(std::uint8_t*,recomp_context*); \
 extern "C" void __wrap_##prefix##audspat_play_sound_at_position(std::uint8_t*r,recomp_context*c){ \
  if(std::uint32_t(c->r4)==0xe)emit(r,owner,dkrr::ps5_sounds::pickup);__real_##prefix##audspat_play_sound_at_position(r,c); } \
 extern "C" void __real_##prefix##music_jingle_play(std::uint8_t*,recomp_context*); \
 extern "C" void __wrap_##prefix##music_jingle_play(std::uint8_t*r,recomp_context*c){ \
  const auto id=std::uint32_t(c->r4);if(id>=43&&id<=50)emit(r,owner,dkrr::ps5_sounds::silver+id-43);__real_##prefix##music_jingle_play(r,c); } \
 extern "C" void __real_##prefix##sound_play_spatial(std::uint8_t*,recomp_context*); \
 extern "C" void __wrap_##prefix##sound_play_spatial(std::uint8_t*r,recomp_context*c){ \
  const auto id=std::uint32_t(c->r4);if(id==0x23d)emit(r,owner,dkrr::ps5_sounds::golden); \
  if(id>=0x156&&id<=0x15f)emit(r,owner,dkrr::ps5_sounds::horn+id-0x156); \
  if(id==0x107){dkrr_controller_zipper_sound(r,std::uint32_t(c->r5),std::uint32_t(c->r6),std::uint32_t(c->r7));} \
  __real_##prefix##sound_play_spatial(r,c); }
extern "C" void dkrr_controller_zipper_sound(const std::uint8_t*,std::uint32_t,std::uint32_t,std::uint32_t);
#if !defined(DKR_PAYLOAD_V77_ENABLED) || DKR_PAYLOAD_V77_ENABLED
SOUND_HOOKS()
#endif
#if !defined(DKR_PAYLOAD_V80_ENABLED) || DKR_PAYLOAD_V80_ENABLED
SOUND_HOOKS(dkr_v80_)
#endif
