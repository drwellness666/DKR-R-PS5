// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "ps5_item_feedback.hpp"
#include "ps5_controller_audio_events.hpp"
#include "ps5_custom_horn.hpp"
#include <memory>
#include <chrono>
#include <vector>
namespace dkrr::ps5_speaker {
// Indices in the private PCM bank, produced from each user's owned revision.
inline unsigned clip_for(ps5_items::Item item) {
    switch(item.type) {
    case 0:return item.level>=2?1:0; // Nitro charge; its level-three boost follows.
    case 1:return item.level==1?4:3;
    case 2:return item.level==0?5:6;
    case 3:return 7;
    case 4:return 8;
    default:return 9;
    }
}
class Event {
    ps5_items::Item previous{};
    std::uint64_t last_press=0;
    bool pressed_before=false;
public:
    int update(bool enabled,ps5_items::Item item,std::uint64_t ms,bool pressed) {
        if(!enabled||!item.valid){previous={};pressed_before=false;return -1;}
        if(pressed){last_press=ms;pressed_before=true;}
        int clip=-1;
        if(pressed_before&&ms-last_press<=250&&previous.valid&&previous.racer==item.racer&&
           previous.quantity>0&&(item.quantity==0||(previous.type==item.type&&item.quantity<previous.quantity)))
            clip=static_cast<int>(clip_for(previous));
        previous=item;return clip;
    }
};
class Speaker {
    struct Worker;
    std::unique_ptr<Worker> worker;
    Event event;
    using Loader=std::vector<std::int16_t>(*)(unsigned,unsigned);
    Loader loader;
    bool start_failed=false;
    struct PendingCue {unsigned clip=0;std::chrono::steady_clock::time_point starts{},expires{};std::shared_ptr<const HornBank> custom;};
    std::array<PendingCue,ps5_sounds::voice_limit> unsent{};
    unsigned unsent_count=0;int unsent_user=-1;unsigned unsent_revision=0;
public:
    Speaker();explicit Speaker(Loader load);~Speaker();Speaker(Speaker&&) noexcept;Speaker&operator=(Speaker&&) noexcept;
    Speaker(const Speaker&)=delete;Speaker&operator=(const Speaker&)=delete;
    void update(bool enabled,int user,ps5_items::Item item,std::uint64_t ms,bool pressed,float volume,unsigned revision,int game_clip=-1,unsigned audio_delay_ms=0,unsigned categories=1023,std::shared_ptr<const HornBank> custom=nullptr);
    int status() const;
};
}
