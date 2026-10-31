// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5_controller_speaker.hpp"
#include <cassert>
#include <thread>
#include <atomic>
#include <stdexcept>
using namespace dkrr::ps5_speaker;
static std::atomic<unsigned> blocks{0},loads{0},opens{0},closes{0};
extern "C" int sceAudioOutOpen(int,int,int,unsigned,unsigned,unsigned){++opens;return 19;}
extern "C" int sceAudioOutOutput(int h,const void*p){assert(h==19);if(p){++blocks;std::this_thread::sleep_for(std::chrono::milliseconds(1));}return 0;}
extern "C" int sceAudioOutClose(int h){assert(h==19);++closes;return 0;}
static std::vector<std::int16_t> retail(unsigned,unsigned){++loads;return std::vector<std::int16_t>(1000,100);}
struct Data {std::vector<std::uint8_t> c=std::vector<std::uint8_t>(160),s=std::vector<std::uint8_t>(16000);};
static void put(std::vector<std::uint8_t>& b,unsigned p,unsigned n){for(unsigned i=0;i<4;++i)b.at(p+i)=std::uint8_t(n>>(24-8*i));}
static std::shared_ptr<const HornBank> bank() {
 auto d=std::make_shared<Data>();auto&c=d->c;c[0]=0x42;c[1]=0x31;c[3]=1;put(c,4,8);put(c,12,16000);put(c,20,24);c[39]=1;put(c,40,48);
 put(c,48,64);put(c,52,80);put(c,56,96);c[61]=127;c[84]=60;c[76]=127;c[77]=127;put(c,100,unsigned(d->s.size()));c[104]=1;
 for(unsigned i=0;i<d->s.size();i+=2){d->s[i]=0x27;d->s[i+1]=0x10;}
 return std::make_shared<const HornBank>(HornBank{d,d->c,d->s,1,127,100});
}
template<class F>static void wait(F f){for(unsigned i=0;i<2000&&!f();++i)std::this_thread::sleep_for(std::chrono::milliseconds(1));assert(f());}
int main(){
 auto source=bank();const auto pcm=decode_horn(*source);assert(pcm.size()==24000&&pcm.front()==0&&pcm.back()==0&&pcm[500]>9800&&pcm[500]<10000);
 for(unsigned n=0;n<160;++n){auto bad=*source;bad.control=bad.control.first(n);bool refused=false;try{decode_horn(bad);}catch(const std::exception&){refused=true;}if(n<116)assert(refused);}
 {auto bad=*source;bad.samples=bad.samples.first(10);bool refused=false;try{decode_horn(bad);}catch(const std::exception&){refused=true;}assert(refused);}
 {auto bad=*source;bad.sound=1024;bool refused=false;try{decode_horn(bad);}catch(const std::exception&){refused=true;}assert(refused);}
 {
  Speaker speaker(retail);const auto before=blocks.load();
  speaker.update(true,42,{},1000,false,.25F,77,dkrr::ps5_sounds::custom_horn,0,1023,source);
  wait([&]{return blocks>before;});assert(loads==0);
  // Admission ownership survives the caller and repeated voices overlap.
  std::weak_ptr<const HornBank> held=source;source.reset();assert(!held.expired());
  auto second=bank();speaker.update(true,42,{},1001,false,.25F,77,dkrr::ps5_sounds::custom_horn,0,1023,second);
  wait([&]{return closes>0;});assert(opens==1&&loads==0);
  auto bad=std::make_shared<HornBank>(*second);bad->control=bad->control.first(1);
  speaker.update(true,42,{},1002,false,.25F,77,dkrr::ps5_sounds::custom_horn,0,1023,bad);
  std::this_thread::sleep_for(std::chrono::milliseconds(15));
  const auto old=blocks.load();speaker.update(true,42,{},1003,false,.25F,77,dkrr::ps5_sounds::horn);
  wait([&]{return blocks>old;});assert(loads==1); // Bad optional mod audio cannot disable retail sound.
 }
 assert(opens==closes);
}
