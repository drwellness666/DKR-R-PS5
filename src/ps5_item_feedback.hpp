// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "ps5_controller_lights.hpp"
namespace dkrr::ps5_items {
struct Item { std::uint32_t racer=0; std::uint8_t type=0, quantity=0; bool valid=false; std::uint8_t level=0; };
struct Command { std::uint32_t magic=0x44545247U; std::uint8_t mode=0, strength=3, reserved[2]={}; };
static_assert(sizeof(Command)==8);
inline std::array<Item,4> sample(std::span<const std::uint8_t> ram, const ps5_lights::Addresses& a) {
    std::array<Item,4> items{};
    if (ram.size()<0x800000U) return items;
    const ps5_lights::Guest g(ram);
    for(auto p:{a.mode,a.tracks_menu,a.load_timer,a.racer_count,a.racers_by_port})
        if((p&3U)||!g.valid(p,4))return items;
    if(!g.valid(a.paused,1)||g.word(a.mode)!=0||g.byte(a.paused)||g.word(a.tracks_menu)||g.word(a.load_timer))return items;
    const auto count=g.word(a.racer_count),table=g.word(a.racers_by_port);
    if(!count||count>10||(table&3U)||!g.valid(table,count*4U))return items;
    for(std::uint32_t port=0;port<4&&port<count;++port) {
        const auto object=g.word(table+port*4U);
        if((object&3U)||!g.valid(object,0x68))continue;
        const auto racer=g.word(object+0x64U);
        if((racer&3U)||!g.valid(racer,0x175U))continue;
        const auto owner=(std::uint32_t(g.byte(racer))<<8)|g.byte(racer+1);
        const auto type=g.byte(racer+0x172U),q=g.byte(racer+0x173U);
        if(owner==port&&type<5&&q<=10)items[port]={racer,type,q,true,std::min<std::uint8_t>(2,g.byte(racer+0x174U))};
    }
    return items;
}
class Mailbox {
    std::array<std::atomic<std::uint64_t>,4> values{};
public:
    void publish(const std::array<Item,4>& items, std::uint64_t ms) {
        for(std::size_t p=0;p<4;++p) {
            const auto& i=items[p];
            values[p].store(i.racer|(std::uint64_t(i.type&7U)<<32)|(std::uint64_t(i.quantity&15U)<<35)|
                (std::uint64_t(i.valid)<<39)|(std::uint64_t(i.level&3U)<<40)|((ms&0x3FFFFFU)<<42),std::memory_order_release);
        }
    }
    Item read(std::size_t p,std::uint64_t ms,bool blocked) const {
        if(p>=4||blocked)return {};
        const auto v=values[p].load(std::memory_order_acquire);
        if(((ms-(v>>42))&0x3FFFFFU)>250U)return {};
        return {std::uint32_t(v),std::uint8_t((v>>32)&7U),std::uint8_t((v>>35)&15U),bool((v>>39)&1U),std::uint8_t((v>>40)&3U)};
    }
};
class Output {
    Item previous{};
    std::uint64_t until=0,last_pull=0;
    std::uint8_t last=0;
    bool owned=false,failed=false,had_pull=false;
public:
    template<class Send> int update(bool enabled,Item item,std::uint64_t ms,bool pulled,Send send) {
        const bool first_error=!failed;
        if(!enabled)failed=false;
        if(!enabled||!item.valid) { item={};until=0;had_pull=false; }
        if(item.valid&&pulled){last_pull=ms;had_pull=true;}
        // DKR consumes weapons on Z release. Keep that confirmed item-use
        // impulse even after L2 has returned, without holding resistance on.
        if(had_pull&&ms-last_pull<=250&&item.valid&&previous.valid&&item.racer==previous.racer&&
           (previous.type==1||previous.type==2)&&previous.quantity>0&&
           (item.quantity==0||(item.type==previous.type&&item.quantity<previous.quantity)))until=ms+70;
        previous=item;
        std::uint8_t mode=0;
        if(item.valid) {
            if(ms<until)mode=2;
            else if(pulled&&item.quantity&&(item.type==1||item.type==2))mode=1;
        }
        if((failed&&enabled&&mode!=0)||(mode==0&&!owned)||(owned&&mode==last))return 0;
        Command cmd;cmd.mode=mode;
        if(send(cmd)!=0) { failed=enabled;owned=mode!=0;last=mode;return first_error?-1:0; }
        owned=mode!=0;last=mode;return 1;
    }
};
}
