// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <array>
#include <atomic>
#include <cstdint>
namespace dkrr::ps5_sounds {
// All media are generated locally from the owner's ROM; these are indices only.
inline constexpr unsigned horn=9,pickup=19,golden=22,zipper=23,silver=24,count=32;
inline constexpr unsigned priority(unsigned clip){return clip<9?3U:clip==zipper?2U:1U;}
class Mailbox {
    std::array<std::atomic<std::uint64_t>,4> values{};
    std::array<std::array<std::atomic<std::uint64_t>,count>,4> last{};
public:
    void publish(unsigned port,unsigned clip,std::uint64_t ms){
        if(port>=4||clip>=count||!ms)return;
        auto& stamp=last[port][clip];auto previous=stamp.load(std::memory_order_relaxed);
        if(previous&&ms>=previous&&ms-previous<180)return;
        if(!stamp.compare_exchange_strong(previous,ms,std::memory_order_relaxed))return;
        // One latest event, no queue. A same-millisecond event of another kind
        // still has a distinct token; repeated calls for one cue are coalesced.
        auto pending=values[port].load(std::memory_order_acquire);
        const auto token=(ms<<8)|(clip+1U);
        do {
            const auto at=pending>>8,old_clip=(pending&255U)-1U;
            // Within one guest frame a quiet pickup must not overwrite an
            // unconsumed zipper/item cue. Still only one pending event.
            if(at&&ms>=at&&ms-at<=40&&old_clip<count&&priority(unsigned(old_clip))>priority(clip))return;
        }while(!values[port].compare_exchange_weak(pending,token,std::memory_order_release,std::memory_order_acquire));
    }
    int take(unsigned port,std::uint64_t ms,bool blocked,std::uint64_t& seen)const{
        if(port>=4)return -1;const auto token=values[port].load(std::memory_order_acquire);
        if(token==seen)return -1;seen=token;const auto at=token>>8;
        if(blocked||!at||ms<at||ms-at>150)return -1;
        const auto clip=(token&255U)-1U;return clip<count?int(clip):-1;
    }
};
}
