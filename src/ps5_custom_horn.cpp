// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5_custom_horn.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
extern "C" int dkrr_vadpcm(const unsigned char*,std::size_t,unsigned,unsigned,const std::int16_t*,std::int16_t*);
namespace dkrr::ps5_speaker {
namespace {
struct Reader {
    std::span<const std::uint8_t> data;
    std::span<const std::uint8_t> read(std::size_t p,std::size_t n)const {
        if(p>data.size()||n>data.size()-p)throw std::runtime_error("Truncated character horn bank");
        return data.subspan(p,n);
    }
    unsigned byte(std::size_t p)const{return read(p,1)[0];}
    unsigned u16(std::size_t p)const{auto b=read(p,2);return (unsigned(b[0])<<8)|b[1];}
    std::uint32_t u32(std::size_t p)const{auto b=read(p,4);return (std::uint32_t(b[0])<<24)|(unsigned(b[1])<<16)|(unsigned(b[2])<<8)|b[3];}
};
}
std::vector<std::int16_t> decode_horn(const HornBank& source) {
    if(!source.sound)return {};
    if(source.control.size()>4*1024*1024||source.samples.size()>16*1024*1024||source.volume>127||source.pitch>255)
        throw std::runtime_error("Character horn exceeds its bank budget");
    const Reader c{source.control},s{source.samples};
    if(c.u16(0)!=0x4231||c.u16(2)!=1)throw std::runtime_error("Invalid character horn bank");
    const std::size_t bank=c.u32(4);const auto rate=c.u32(bank+4);const std::size_t inst=c.u32(bank+12);
    const auto count=c.u16(inst+14);
    if(rate<8000||rate>96000||!count||count>1023)throw std::runtime_error("Invalid character horn instrument");
    std::array<unsigned,64> seen{};unsigned used=0,index=source.sound;
    std::vector<double> mix;mix.reserve(192000);
    while(index) {
        if(index>count||used==seen.size()||std::find(seen.begin(),seen.begin()+used,index)!=seen.begin()+used)
            throw std::runtime_error("Invalid character horn sequence");
        seen[used++]=index;
        const std::size_t sound=c.u32(inst+16+4*(index-1)),env=c.u32(sound),key=c.u32(sound+4),wave=c.u32(sound+8);
        c.read(sound,16);c.read(env,16);c.read(key,6);c.read(wave,20);
        const std::size_t base=c.u32(wave);const auto length=c.u32(wave+4),kind=c.byte(wave+8);
        if(!length||length>2*1024*1024||kind>1)throw std::runtime_error("Invalid character horn sample");
        auto bytes=s.read(base,length);std::vector<std::int16_t> pcm;
        if(kind==0) {
            const std::size_t book=c.u32(wave+16);const auto order=c.u32(book),predictors=c.u32(book+4);
            if(order!=2||!predictors||predictors>16||length%9>1)throw std::runtime_error("Invalid character horn predictor book");
            std::array<std::int16_t,256> coefficients{};
            for(unsigned i=0;i<order*predictors*8;++i)coefficients[i]=std::int16_t(c.u16(book+8+2*i));
            pcm.resize((length/9)*16);
            if(pcm.empty()||dkrr_vadpcm(bytes.data(),length-length%9,order,predictors,coefficients.data(),pcm.data())<0)
                throw std::runtime_error("Invalid character horn ADPCM");
        }else {
            if(length%2)throw std::runtime_error("Invalid character horn PCM");
            pcm.resize(length/2);for(std::size_t i=0;i<pcm.size();++i)pcm[i]=std::int16_t(s.u16(base+2*i));
        }
        const double speed=source.pitch/100.0*std::exp2((int(c.byte(key+4))*100-6000+std::int8_t(c.byte(key+5)))/1200.0);
        if(!std::isfinite(speed)||speed<0.1||speed>10)throw std::runtime_error("Invalid character horn pitch");
        const auto frames=std::min<std::size_t>(144000,std::size_t(pcm.size()*48000/(rate*speed)));
        const auto delay=std::min<std::size_t>(48000,std::size_t(c.byte(key+1))*33333*48000/1000000);
        mix.resize(std::max(mix.size(),delay+frames),0);
        const double attack=std::int32_t(c.u32(env)),decay=std::int32_t(c.u32(env+4));
        const double av=c.byte(env+12),dv=c.byte(env+13),gain=c.byte(sound+13)/127.0*source.volume/128.0;
        for(std::size_t i=0;i<frames;++i) {
            const double position=i*rate*speed/48000;const auto j=std::size_t(position);const double f=position-j;
            if(j>=pcm.size())throw std::runtime_error("Character horn resampling overrun");
            const double sample=pcm[j]*(1-f)+pcm[std::min(j+1,pcm.size()-1)]*f,us=i*1000000.0/48000;
            const double envelope=attack>0&&us<attack?av/127*us/attack:decay>0?(av+(dv-av)*std::clamp((us-std::max(0.0,attack))/decay,0.0,1.0))/127:av/127;
            const double fade=std::clamp(std::min(i/96.0,(frames-1-i)/240.0),0.0,1.0);
            mix[delay+i]+=sample*gain*envelope*fade;
        }
        index=c.byte(key)+(c.byte(key+2)&0xc0)*4;
    }
    double peak=0;for(auto v:mix)peak=std::max(peak,std::abs(v));const double scale=peak>30000?30000/peak:1;
    std::vector<std::int16_t> result(mix.size());
    for(std::size_t i=0;i<mix.size();++i)result[i]=std::int16_t(std::nearbyint(std::clamp(mix[i]*scale,-32768.0,32767.0)));
    return result;
}
}
