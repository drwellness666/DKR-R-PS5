// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>
#include <version>
namespace dkrr::ps5_speaker {
// Immutable, already admitted character bank. The owner keeps its samples
// alive across a ROM/mod transition; decoding belongs to the speaker worker.
struct HornBank {
    std::shared_ptr<const void> owner;
    std::span<const std::uint8_t> control,samples;
    unsigned sound=0,volume=0,pitch=100;
};
class HornSlot {
#if defined(__cpp_lib_atomic_shared_ptr)
    std::atomic<std::shared_ptr<const HornBank>> value;
public:
    std::shared_ptr<const HornBank> load()const{return value.load();}
    void store(std::shared_ptr<const HornBank> next){value.store(std::move(next));}
#else
    // Match upstream AtomicSnapshot on the SDK's libc++.
    std::shared_ptr<const HornBank> value;
public:
    std::shared_ptr<const HornBank> load()const{return std::atomic_load(&value);}
    void store(std::shared_ptr<const HornBank> next){std::atomic_store(&value,std::move(next));}
#endif
};
inline std::array<HornSlot,4> custom_horns;
std::vector<std::int16_t> decode_horn(const HornBank&);
}
