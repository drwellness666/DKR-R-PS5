// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5_controller_lights.hpp"
#include "ps5_item_feedback.hpp"
#include "revision_addresses.hpp"
#include <cassert>
#include <cstdio>
#include <vector>
#include <cstring>
#define SDL_memcmp std::memcmp
#define SDL_memcpy std::memcpy
#define SDL_zero(v) std::memset(&(v),0,sizeof(v))

// Exercise the actual pinned SDL SetLED function with a modeled ScePad endpoint.
using Uint8=std::uint8_t;
using Uint32=std::uint32_t;
struct SDL_Joystick {};
struct PS5_PadContext { int handle=17; };
struct PS5_PadColor { std::uint8_t r,g,b,a; };
static PS5_PadContext ctx;
static bool attached=true;
static int calls=0,returned=0,handle=0,resets=0;
static int SDL_Unsupported() { return -1; }
static int scePadResetLightBar(int h) { ++calls;++resets;handle=h;return returned; }
static PS5_PadColor last{};
static PS5_PadContext* PS5_JoystickGetPadContext(SDL_Joystick*) { return attached?&ctx:nullptr; }
static int SDL_SetError(const char*,...) { return -1; }
static int scePadSetLightBar(int h,const PS5_PadColor* c) {
    ++calls;handle=h;last=*c;return returned;
}
static unsigned char trigger_packet[120]{};
static int trigger_calls=0;
int scePadSetTriggerEffect(int,const void*) __attribute__((weak));
int scePadSetTriggerEffect(int h,const void*p) { ++trigger_calls;handle=h;std::memcpy(trigger_packet,p,120);return returned; }
#include "ps5_led.inc"

int main() {
    using namespace dkrr::ps5_lights;
    using namespace dkr::runtime::revision_addresses;
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free);
    for (const auto& a:{kUsV77,kUsV80}) {
        std::vector<std::uint8_t> ram(0x800000);
        const Addresses addresses{a.GameMode,a.IsPaused,a.IsInTracksMenu,
                                  a.LevelLoadTimer,a.NumberOfRacers,a.RacersByPort,a.CutsceneCameraActive,a.RaceEndStage,a.RaceEndTimer};
        auto byte=[&](std::uint32_t address,std::uint8_t value) { ram[(address-0x80000000U)^3U]=value; };
        auto word=[&](std::uint32_t address,std::uint32_t value) {
            for (unsigned i=0;i<4;++i)byte(address+i,static_cast<std::uint8_t>(value>>(24-i*8)));
        };
        constexpr std::uint32_t table=0x80400000U;
        word(a.RacersByPort,table);word(a.NumberOfRacers,4);
        for (unsigned port=0;port<4;++port) {
            const auto object=0x80401000U+port*0x1000U,racer=object+0x100U;
            word(table+port*4,object);word(object+0x64U,racer);byte(object+0x49U,1);
            byte(racer,0);byte(racer+1,port);
            byte(racer+0x172U,port);byte(racer+0x173U,1);
        }
        const auto expected=std::uint16_t(1U|(2U<<3)|(3U<<6)|(4U<<9));
        const auto before=ram;assert(sample(ram,addresses)==expected);assert(before==ram);
        const auto live_items=dkrr::ps5_items::sample(ram,addresses);
        for(unsigned port=0;port<4;++port){assert(live_items[port].valid);assert(dkrr::ps5_items::color(live_items[port])==palette[port+1]);}
        // IsPaused is an s8, not a word: nonzero neighboring bytes are unrelated.
        byte(a.IsPaused+1,0x7F);assert(sample(ram,addresses)==expected);
        byte(a.IsPaused,1);assert(sample(ram,addresses)==0);byte(a.IsPaused,0);
        for (auto address:{a.GameMode,a.IsInTracksMenu,a.LevelLoadTimer}) {
            word(address,1);assert(sample(ram,addresses)==0);word(address,0);
        }
        byte(a.CutsceneCameraActive,1);assert(sample(ram,addresses)==0);assert(!dkrr::ps5_items::sample(ram,addresses)[0].valid);byte(a.CutsceneCameraActive,0);
        // Actual victory flow: raceFinished is cleared for the award, while
        // balloon inventory remains. Ignore stale stage after a new scene.
        byte(a.RaceEndStage,4);byte(a.RaceEndTimer,255);byte(a.RaceEndTimer+1,255);
        assert(sample(ram,addresses)==0);assert(!dkrr::ps5_items::sample(ram,addresses)[0].valid);
        byte(a.RaceEndTimer,0);byte(a.RaceEndTimer+1,0);assert(sample(ram,addresses)==expected);
        byte(a.RaceEndStage,0);
        // The original spatial shield sound runs only while its timer >60.
        constexpr auto local_racer=0x80401100U;
        byte(local_racer+0x189U,1);byte(local_racer+0x18eU,1);byte(local_racer+0x18fU,44);
        assert(dkrr::ps5_items::sample(ram,addresses)[0].shield);
        byte(local_racer+0x18eU,0);byte(local_racer+0x18fU,60);assert(!dkrr::ps5_items::sample(ram,addresses)[0].shield);
        byte(local_racer+0x189U,0);byte(local_racer+0x18fU,0);
        word(a.NumberOfRacers,11);assert(sample(ram,addresses)==0);
        word(a.NumberOfRacers,0);assert(sample(ram,addresses)==0);word(a.NumberOfRacers,4);
        for (auto bad:{0U,0x80400001U,0x807FFFFCU,0xFFFFFFFFU}) {
            word(a.RacersByPort,bad);assert(sample(ram,addresses)==0);
        }
        word(a.RacersByPort,table);
        constexpr auto racer=0x80401100U;
        for (auto type:{0,1,2,3,4}) {
            byte(racer+0x172U,type);assert((sample(ram,addresses)&7U)==unsigned(type+1));
        }
        byte(racer+0x1D8U,1);assert((sample(ram,addresses)&7U)==0); // Finished, balloon still held.
        assert((sample(ram,addresses)&(7U<<3))!=0); // Other local racer still racing.
        byte(racer+0x1D8U,0);assert((sample(ram,addresses)&7U)==5);
        // A freed object can retain plausible player/inventory bytes.
        byte(0x80401049U,2);assert((sample(ram,addresses)&7U)==0);
        assert(!dkrr::ps5_items::sample(ram,addresses)[0].valid);
        assert(dkrr::ps5_items::color(dkrr::ps5_items::sample(ram,addresses)[0])==palette[0]);
        byte(0x80401049U,1);
        byte(racer+0x174U,255);assert((sample(ram,addresses)&7U)==0);assert(!dkrr::ps5_items::sample(ram,addresses)[0].valid);byte(racer+0x174U,0);
        byte(racer+0x172U,5);assert((sample(ram,addresses)&7U)==0);
        byte(racer+0x172U,0);
        for (auto quantity:{0,11,255}) {
            byte(racer+0x173U,quantity);assert((sample(ram,addresses)&7U)==0);
            assert(dkrr::ps5_items::color(dkrr::ps5_items::sample(ram,addresses)[0])==palette[0]);
        }
        byte(racer+0x173U,1);byte(racer,0xFF);byte(racer+1,0xFF);
        assert((sample(ram,addresses)&7U)==0); // AI is not a local controller
        byte(racer,0);byte(racer+1,0);
        for (auto bad:{0U,0x80401101U,0x807FFFFCU,0xFFFFFFFFU}) {
            word(0x80401064U,bad);assert((sample(ram,addresses)&7U)==0);
        }
        assert(sample(std::span(ram).first(1024),addresses)==0);
    }
    Mailbox box;
    assert(box.read(0,100,false)==palette[0]);
    box.publish(1U|(2U<<3)|(3U<<6)|(5U<<9),100);
    assert(box.read(0,100,false)==palette[1] && box.read(1,100,false)==palette[2]);
    assert(box.read(2,100,false)==palette[3] && box.read(3,100,false)==palette[5]);
    assert(box.read(3,600,false)==palette[5] && box.read(3,601,false)==palette[0]);
    assert(box.read(0,99,false)==palette[0] && box.read(0,101,true)==palette[0]);
    assert(box.read(4,101,false)==palette[0]);

    SDL_Joystick pad;
    auto send=[&](Color color){
        static constexpr char reset[]="DKRR-RESET-LIGHTBAR-1";
        return color.system_default?PS5_JoystickSendEffect(&pad,reset,sizeof(reset)):
                                    PS5_JoystickSetLED(&pad,color.r,color.g,color.b);
    };
    Output out;
    assert(out.update(false,palette[1],send)==0 && calls==0);
    assert(out.update(true,palette[1],send)==1 && calls==1 && handle==17);
    assert(last.r==0 && last.g==24 && last.b==96 && last.a==255);
    for (int frame=0;frame<600;++frame)assert(out.update(true,palette[1],send)==0);
    assert(calls==1);
    assert(out.update(true,palette[2],send)==1 && calls==2);
    assert(out.update(true,palette[0],send)==1 && calls==3);
    assert(out.update(false,{},send)==1 && calls==4 && !out.owned);
    assert(resets==2 && out.last.system_default);
    assert(out.update(false,{},send)==0 && calls==4);
    assert(out.update(true,palette[4],send)==1 && calls==5);
    returned=-23;
    assert(out.update(true,palette[1],send)==-1 && calls==6 && out.failed);
    for (int frame=0;frame<600;++frame)assert(out.update(true,palette[2],send)==0);
    assert(calls==6);
    returned=0;assert(out.update(false,{},send)==1 && calls==7 && !out.owned);
    assert(out.update(true,palette[4],send)==1 && calls==8);
    assert(out.update(false,{},send)==1 && calls==9);
    const auto fresh_calls=calls;Output fresh;attached=false;
    assert(fresh.update(true,palette[1],send)==-1 && calls==fresh_calls);
    assert(fresh.update(true,palette[2],send)==0 && calls==fresh_calls);
    static constexpr char command[]="DKRR-RESET-LIGHTBAR-1";
    attached=true;const auto before_calls=calls;
    assert(PS5_JoystickSendEffect(&pad,nullptr,sizeof(command))==-1);
    assert(PS5_JoystickSendEffect(&pad,command,sizeof(command)-1)==-1);
    assert(PS5_JoystickSendEffect(&pad,"BAD!-RESET-LIGHTBAR-1",sizeof(command))==-1);
    assert(calls==before_calls);
    returned=-31;assert(PS5_JoystickSendEffect(&pad,command,sizeof(command))==-1);
    assert(calls==before_calls+1 && handle==17);
    returned=0;
    PS5_ItemEffect effect{0x44545247,1,3,{}};
    assert(PS5_JoystickSendEffect(&pad,&effect,sizeof(effect))==0 && trigger_calls==1);
    assert(trigger_packet[0]==1 && trigger_packet[8]==2 && trigger_packet[16]==3 && trigger_packet[17]==6 && trigger_packet[18]==3);
    for(int i=64;i<120;++i)assert(trigger_packet[i]==0); // R2 never touched.
    effect.mode=2;assert(PS5_JoystickSendEffect(&pad,&effect,sizeof(effect))==0);
    assert(trigger_packet[8]==3 && trigger_packet[16]==0 && trigger_packet[17]==3 && trigger_packet[18]==50);
    effect.mode=0;assert(PS5_JoystickSendEffect(&pad,&effect,sizeof(effect))==0);
    assert(trigger_packet[0]==1);for(int i=1;i<120;++i)assert(trigger_packet[i]==0);
    effect.mode=3;assert(PS5_JoystickSendEffect(&pad,&effect,sizeof(effect))==-1);
    effect.mode=1;effect.strength=6;assert(PS5_JoystickSendEffect(&pad,&effect,sizeof(effect))==-1);
    effect.strength=3;effect.reserved[0]=1;assert(PS5_JoystickSendEffect(&pad,&effect,sizeof(effect))==-1);
    assert(trigger_calls==3);
    // A transient transport failure is retried only after explicit disable.
    Output retry;returned=-17;
    assert(retry.update(true,palette[1],send)==-1);
    const auto retry_calls=calls;
    assert(retry.update(true,palette[2],send)==0 && calls==retry_calls);
    assert(retry.update(false,{},send)==0);
    returned=0;assert(retry.update(true,palette[2],send)==1);
    returned=-17;assert(retry.update(true,palette[1],send)==-1);
    assert(retry.update(false,{},send)==0); // Failed reset still ends ownership.
    const auto disabled_calls=calls;
    for(unsigned i=0;i<600;++i)assert(retry.update(false,{},send)==0);
    assert(calls==disabled_calls);
    returned=0;assert(retry.update(true,palette[4],send)==1);
    assert(retry.update(false,{},send)==1);
    std::puts("PASS bounded native L2 packet, weapon/pulse/off modes and malformed packet refusal; R2 untouched");
    std::puts("PASS both actual US address tables, read-only guest inventory, bounds/owner/menu/pause guards, atomic expiry, actual SDL/ScePad colors and system reset, exact command refusal, deduplication, failure suppression, disable cleanup and explicit-toggle recovery");
}
