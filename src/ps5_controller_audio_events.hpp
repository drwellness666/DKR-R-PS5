// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "ps5_controller_audio_bank.h"
#include <array>
#include <atomic>
#include <cstdint>
namespace dkrr::ps5_sounds {
inline constexpr unsigned horn=9,pickup=19,golden=22,zipper=23,silver=24,go_for_it=DKRR_CONTROLLER_GO_FOR_IT_SLOT,custom_horn=DKRR_CONTROLLER_CUE_COUNT,count=DKRR_CONTROLLER_CUE_COUNT+1;
inline constexpr unsigned voice_limit=32; // Original DKR sound-state budget.
inline constexpr unsigned category(unsigned clip){
    return clip<3?0U:clip<5?1U:clip<7?2U:clip==7?3U:clip==8?4U:
        ((clip>=horn&&clip<horn+10)||clip==custom_horn)?5U:(clip>=pickup&&clip<pickup+3)?6U:((clip>=silver&&clip<silver+DKRR_CONTROLLER_JINGLE_COUNT)||clip==go_for_it)?7U:clip==golden?8U:clip==zipper?9U:10U;
}
inline constexpr bool allowed(unsigned mask,unsigned clip){return category(clip)<10&&(mask&(1U<<category(clip)));}
inline constexpr unsigned priority(unsigned clip){return clip<9?3U:clip==zipper?2U:1U;}
// Fixed live-event inbox between guest and input threads. Drain the current
// burst each poll; expire old events instead of playing a delayed backlog.
// Sequence ownership makes publication bounded and nonblocking, including
// simultaneous producers. Original calls need no artificial cooldown.
class Mailbox {
    struct Slot {std::atomic<std::uint64_t> sequence{0};std::uint64_t event=0;};
    struct Port {
        std::array<Slot,voice_limit> slots;
        std::atomic<std::uint64_t> write{0},read{0};
        Port(){for(unsigned i=0;i<voice_limit;++i)slots[i].sequence.store(i,std::memory_order_relaxed);}
    };
    std::array<Port,4> ports;
public:
    bool publish(unsigned port,unsigned clip,std::uint64_t ms){
        if(port>=4||clip>=count||!ms||ms>(UINT64_MAX>>8))return false;
        auto&p=ports[port];auto at=p.write.load(std::memory_order_relaxed);
        for(unsigned attempt=0;attempt<8;++attempt){
            auto&slot=p.slots[at%voice_limit];const auto sequence=slot.sequence.load(std::memory_order_acquire);
            if(sequence<at)return false; // Full: never block gameplay or grow memory.
            if(sequence!=at){at=p.write.load(std::memory_order_relaxed);continue;}
            if(!p.write.compare_exchange_weak(at,at+1,std::memory_order_relaxed))continue;
            slot.event=(ms<<8)|(clip+1U);slot.sequence.store(at+1,std::memory_order_release);return true;
        }
        return false;
    }
    int take(unsigned port,std::uint64_t ms,bool blocked,std::uint64_t& seen){
        if(port>=4)return -1;auto&p=ports[port];
        for(unsigned consumed=0;consumed<voice_limit;++consumed){
            auto at=p.read.load(std::memory_order_relaxed);auto&slot=p.slots[at%voice_limit];
            if(slot.sequence.load(std::memory_order_acquire)!=at+1)return -1;
            if(!p.read.compare_exchange_weak(at,at+1,std::memory_order_relaxed))continue;
            const auto token=slot.event;slot.sequence.store(at+voice_limit,std::memory_order_release);seen=at+1;
            const auto time=token>>8;const auto clip=(token&255U)-1U;
            if(!blocked&&time&&ms>=time&&ms-time<=150&&clip<count)return int(clip);
        }
        return -1;
    }
};
}
