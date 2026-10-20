// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <span>

namespace dkrr::ps5_lights {
struct Color {
    std::uint8_t r=0, g=0, b=0;
    bool system_default=false;
    constexpr bool operator==(const Color&) const = default;
};
// System default when empty; dim item colors: boost/blue, missile/red, trap/green, shield/yellow,
// magnet/multicolored balloon represented by violet. No flashing animation.
inline constexpr std::array<Color,6> palette{{
    {0,0,0,true}, {0,24,96}, {96,4,0}, {0,72,8}, {88,64,0}, {64,4,88}
}};
struct Addresses {
    std::uint32_t mode, paused, tracks_menu, load_timer, racer_count, racers_by_port;
};
// DKR's original 8 MiB guest range only, in the runtime's word-swapped format.
// The existing HUD callback owns this memory. Never read live guest RAM from SDL.
class Guest {
    std::span<const std::uint8_t> ram;
public:
    explicit Guest(std::span<const std::uint8_t> bytes):ram(bytes){}
    bool valid(std::uint32_t address, std::size_t size) const {
        if (address<0x80000000U || address>=0x80800000U) return false;
        const auto offset=address-0x80000000U;
        return offset<ram.size() && size<=ram.size()-offset && size<=0x800000U-offset;
    }
    std::uint8_t byte(std::uint32_t address) const { return ram[(address-0x80000000U)^3U]; }
    std::uint32_t word(std::uint32_t address) const {
        return (std::uint32_t(byte(address))<<24)|(std::uint32_t(byte(address+1))<<16)|
               (std::uint32_t(byte(address+2))<<8)|byte(address+3);
    }
};
inline std::uint16_t sample(std::span<const std::uint8_t> ram, const Addresses& a) {
    if (ram.size()<0x800000U) return 0;
    const Guest g(ram);
    for (auto p:{a.mode,a.tracks_menu,a.load_timer,a.racer_count,a.racers_by_port})
        if ((p&3U) || !g.valid(p,4)) return 0;
    if (!g.valid(a.paused,1) || g.word(a.mode)!=0 || g.byte(a.paused)!=0 ||
        g.word(a.tracks_menu)!=0 || g.word(a.load_timer)!=0) return 0;
    const auto count=g.word(a.racer_count), table=g.word(a.racers_by_port);
    if (!count || count>10 || (table&3U) || !g.valid(table,count*4U)) return 0;
    std::uint16_t colors=0;
    for (std::uint32_t port=0;port<4 && port<count;++port) {
        const auto object=g.word(table+port*4U);
        if ((object&3U) || !g.valid(object,0x68)) continue;
        const auto racer=g.word(object+0x64U);
        // Object.racer +0x64; Object_Racer.playerIndex +0, balloon_type +0x172,
        // quantity +0x173. Authored decomp fields, identical for US77/US80.
        if ((racer&3U) || !g.valid(racer,0x1D9U)) continue;
        const auto owner=(std::uint32_t(g.byte(racer))<<8)|g.byte(racer+1);
        const auto type=g.byte(racer+0x172U), quantity=g.byte(racer+0x173U);
        // raceFinished is set at the finish line, before the results/award
        // cutscene changes GameMode. Inventory can remain populated there.
        if (owner==port && !g.byte(racer+0x1D8U) && type<5 && quantity>0 && quantity<=10)
            colors|=static_cast<std::uint16_t>((type+1U)<<(port*3U));
    }
    return colors;
}
class Mailbox {
    std::atomic<std::uint64_t> value{0};
public:
    void publish(std::uint16_t colors,std::uint64_t ms) {
        value.store((ms<<16)|(colors&0xFFFU),std::memory_order_release);
    }
    Color read(std::size_t port,std::uint64_t ms,bool blocked) const {
        const auto state=value.load(std::memory_order_acquire), at=state>>16;
        if (blocked || port>=4 || !at || ms<at || ms-at>500) return palette[0];
        const auto code=(state>>(port*3U))&7U;
        return palette[code<palette.size()?code:0];
    }
};
struct Output {
    Color last{};
    bool owned=false, failed=false;
    // Returns -1 on the first backend failure, 1 on a send, 0 on no work.
    // Suppress retries after failure until explicit disable. Closing/disabling
    // still tries one reset, then re-arms the next enabled session.
    template<class Send> int update(bool enabled,Color color,Send send) {
        const bool first_error=!failed;
        if (!enabled) failed=false;
        if ((enabled && failed) || (!enabled && !owned)) return 0;
        const Color target=enabled?color:palette[0];
        if (enabled && owned && target==last) return 0;
        if (send(target)!=0) {
            failed=enabled;
            if (!enabled) owned=false;
            return first_error?-1:0;
        }
        last=target;owned=enabled;
        return 1;
    }
};
} // namespace dkrr::ps5_lights
