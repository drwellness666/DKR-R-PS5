// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <array>
#include <string>
#include <string_view>
namespace dkrr::ps5_pad {
inline constexpr unsigned action_count=20;
inline constexpr std::array<const char*,action_count> action_names{
    "Steer up","Steer down","Steer left","Steer right","Accelerate","Brake / reverse",
    "Item / horn","Pause","D-pad up","D-pad down","D-pad left","D-pad right",
    "N64 L","Drift / N64 R","C-Up","C-Down","C-Left","C-Right","Look back","Modern accelerate"};
struct Bindings {std::array<int,action_count> primary{},alternate{};};
// SDL's stable logical source numbers, including axis direction encoding.
inline constexpr std::array<int,14> sources{0,1,2,3,4,6,9,10,1009,1011,11,12,13,14};
inline constexpr std::array<const char*,14> names{"Cross","Circle","Square","Triangle","Touchpad","Options","L1","R1","L2","R2","D-pad up","D-pad down","D-pad left","D-pad right"};
inline std::string assigned(const Bindings& b,int source) {
    std::string result;
    for(unsigned i=0;i<action_count;++i)if(b.primary[i]==source||b.alternate[i]==source){
        if(!result.empty())result+=" / ";result+=action_names[i];
    }
    // Touchpad is owned by the settings shortcut independently of race mappings.
    if(result.empty()&&source==4)return "Settings";
    return result.empty()?"Not assigned":result;
}
inline std::string stick_label(const Bindings& b,bool right) {
    const int base=right?1004:1000;bool any=false;
    for(unsigned i=0;i<action_count;++i)if((b.primary[i]>=base&&b.primary[i]<base+4)||(b.alternate[i]>=base&&b.alternate[i]<base+4))any=true;
    const std::array<int,4> normal=right?std::array<int,4>{1006,1007,1004,1005}:std::array<int,4>{1002,1003,1000,1001};
    const unsigned first=right?14:0;bool original=true;
    for(unsigned i=0;i<4;++i)original&=b.primary[first+i]==normal[i]||b.alternate[first+i]==normal[i];
    return std::string(right?"R stick: ":"L stick: ")+(original?(right?"C-buttons":"steering"):(any?"custom":"unassigned"));
}

inline bool selected(const Bindings& b,int source,int action) {
    return action>=0&&action<int(action_count)&&(b.primary[action]==source||b.alternate[action]==source);
}
}
