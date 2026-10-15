// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5_controller_lights.hpp"
#include "revision_addresses.hpp"
#include <cassert>
#include <cstdio>
#include <vector>
#include <cstring>
#define SDL_memcmp std::memcmp

// Exercise the actual pinned SDL SetLED function with a modeled ScePad endpoint.
using Uint8=std::uint8_t;
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
#include "ps5_led.inc"

int main() {
    using namespace dkrr::ps5_lights;
    using namespace dkr::runtime::revision_addresses;
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free);
    for (const auto& a:{kUsV77,kUsV80}) {
        std::vector<std::uint8_t> ram(0x800000);
        const Addresses addresses{a.GameMode,a.IsPaused,a.IsInTracksMenu,
                                  a.LevelLoadTimer,a.NumberOfRacers,a.RacersByPort};
        auto byte=[&](std::uint32_t address,std::uint8_t value) { ram[(address-0x80000000U)^3U]=value; };
        auto word=[&](std::uint32_t address,std::uint32_t value) {
            for (unsigned i=0;i<4;++i)byte(address+i,static_cast<std::uint8_t>(value>>(24-i*8)));
        };
        constexpr std::uint32_t table=0x80400000U;
        word(a.RacersByPort,table);word(a.NumberOfRacers,4);
        for (unsigned port=0;port<4;++port) {
            const auto object=0x80401000U+port*0x1000U,racer=object+0x100U;
            word(table+port*4,object);word(object+0x64U,racer);
            byte(racer,0);byte(racer+1,port);
            byte(racer+0x172U,port);byte(racer+0x173U,1);
        }
        const auto expected=std::uint16_t(1U|(2U<<3)|(3U<<6)|(4U<<9));
        const auto before=ram;assert(sample(ram,addresses)==expected);assert(before==ram);
        // IsPaused is an s8, not a word: nonzero neighboring bytes are unrelated.
        byte(a.IsPaused+1,0x7F);assert(sample(ram,addresses)==expected);
        byte(a.IsPaused,1);assert(sample(ram,addresses)==0);byte(a.IsPaused,0);
        for (auto address:{a.GameMode,a.IsInTracksMenu,a.LevelLoadTimer}) {
            word(address,1);assert(sample(ram,addresses)==0);word(address,0);
        }
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
        byte(racer+0x172U,5);assert((sample(ram,addresses)&7U)==0);
        byte(racer+0x172U,0);
        for (auto quantity:{0,11,255}) {
            byte(racer+0x173U,quantity);assert((sample(ram,addresses)&7U)==0);
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
    Output fresh;attached=false;
    assert(fresh.update(true,palette[1],send)==-1 && calls==7);
    assert(fresh.update(true,palette[2],send)==0 && calls==7);
    static constexpr char command[]="DKRR-RESET-LIGHTBAR-1";
    attached=true;const auto before_calls=calls;
    assert(PS5_JoystickSendEffect(&pad,nullptr,sizeof(command))==-1);
    assert(PS5_JoystickSendEffect(&pad,command,sizeof(command)-1)==-1);
    assert(PS5_JoystickSendEffect(&pad,"BAD!-RESET-LIGHTBAR-1",sizeof(command))==-1);
    assert(calls==before_calls);
    returned=-31;assert(PS5_JoystickSendEffect(&pad,command,sizeof(command))==-1);
    assert(calls==before_calls+1 && handle==17);
    std::puts("PASS both actual US address tables, read-only guest inventory, bounds/owner/menu/pause guards, atomic expiry, actual SDL/ScePad colors and system reset, exact command refusal, deduplication, failure suppression and disable cleanup");
}
