// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "ps5_hdr_options.h"
#include <algorithm>
#include <array>
#include <cmath>
namespace dkrr::hdr {
inline bool valid(dkrr_ps5_hdr_choice c) {
    return c.enabled<=1 && c.paper_white>=80 && c.paper_white<=300 &&
           c.peak>=400 && c.peak<=2000 && c.peak>=c.paper_white && c.expansion<=100;
}
inline double linear(double x) {
    x=std::clamp(x,0.0,1.0);
    return x<=0.04045 ? x/12.92 : std::pow((x+0.055)/1.055,2.4);
}
// ST 2084 absolute luminance, nits to PQ code. Black stays precisely black.
inline double pq(double nits) {
    if(!(nits>0))return 0;
    const double v=std::pow(std::clamp(nits/10000.0,0.0,1.0),2610.0/16384.0);
    return std::pow((3424.0/4096.0+(2413.0/128.0)*v)/(1+(2392.0/128.0)*v),2523.0/32.0);
}
inline std::array<double,3> rec2020(std::array<double,3> c) {
    return {0.627404*c[0]+0.329283*c[1]+0.043313*c[2],
            0.069097*c[0]+0.919540*c[1]+0.011362*c[2],
            0.016391*c[0]+0.088013*c[1]+0.895595*c[2]};
}
// Preserve dark/mid tones. Only the top 30% of linear scene luminance expands.
// UI is independently mapped at paper white, never at the scene highlight peak.
inline std::array<double,3> compose(std::array<double,3> scene,std::array<double,4> ui,dkrr_ps5_hdr_choice c) {
    std::array<double,3> s{},u{};const double a=std::clamp(ui[3],0.0,1.0);
    for(unsigned i=0;i<3;++i){s[i]=linear(scene[i]);u[i]=linear(a>1e-6?ui[i]/a:0);}
    const double l=s[0]*0.2126+s[1]*0.7152+s[2]*0.0722;
    double k=std::clamp((l-0.7)/0.3,0.0,1.0);k=k*k*(3-2*k);
    const double scale=c.paper_white+(c.peak-c.paper_white)*(c.expansion/100.0)*k;
    s=rec2020(s);u=rec2020(u);
    for(unsigned i=0;i<3;++i)s[i]=pq(std::clamp(s[i]*scale*(1-a)+u[i]*c.paper_white*a,0.0,double(c.peak)));
    return s;
}
}
