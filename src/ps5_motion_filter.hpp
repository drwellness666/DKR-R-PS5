// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
namespace dkrr::ps5_motion {
// SDL units and axes: rad/s; m/s^2 including positive Y gravity.
// Tilt has a gravity reference; unlike a pure gyro integral its centre cannot
// walk away after slow left/right turns. Yaw has no gravity reference.
class Tilt {
    bool ready=false;
    std::array<float,2> neutral{},angle{};
public:
    void reset(){ready=false;angle={};}
    bool update(const float* gyro,const float* acceleration,float dt,float bias_roll,float bias_pitch) {
        for(unsigned i=0;i<3;++i)if(!std::isfinite(gyro[i])||!std::isfinite(acceleration[i]))return false;
        const float norm=std::sqrt(acceleration[0]*acceleration[0]+acceleration[1]*acceleration[1]+acceleration[2]*acceleration[2]);
        const bool gravity=norm>8.3F&&norm<11.3F;
        if(!ready&&!gravity)return false;
        const std::array<float,2> measured{std::atan2(acceleration[0],std::hypot(acceleration[1],acceleration[2])),std::atan2(-acceleration[2],acceleration[1])};
        if(!ready){neutral=measured;ready=true;angle={};return true;}
        // A suspended/stalled process must not multiply its last rate by a
        // long wall-clock gap. Keep the same calibrated neutral pose.
        const bool integrate=dt>0&&dt<=0.05F;
        if(integrate){angle[0]+=(gyro[2]-bias_roll)*dt;angle[1]+=(gyro[0]-bias_pitch)*dt;}
        if(gravity&&dt>0){
            const float blend=dt>0.05F?1.0F:1.0F-std::exp(-dt/0.12F);
            for(unsigned i=0;i<2;++i){const float target=std::remainder(measured[i]-neutral[i],6.28318530718F);angle[i]+=blend*std::remainder(target-angle[i],6.28318530718F);}
        }
        for(auto&v:angle)v=std::clamp(v,-1.57079632679F,1.57079632679F);
        return true;
    }
    float roll()const{return angle[0];}float pitch()const{return angle[1];}
};
}
