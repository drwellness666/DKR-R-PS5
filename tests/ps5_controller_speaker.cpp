// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5_controller_speaker.hpp"
#include <atomic>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <thread>
using namespace dkrr;
static std::atomic<int> opens{0},outputs{0},closes{0},active{0},loaded{-1},fail_open{0};
extern "C" int sceAudioOutOpen(int user,int type,int index,unsigned length,unsigned hz,unsigned format) {
    assert(user==42&&type==4&&index==0&&length==256&&hz==48000&&format==0);
    ++opens;if(fail_open)return -77;++active;return 17;
}
extern "C" int sceAudioOutOutput(int h,const void*p) {
    assert(h==17);
    if(p){const auto*s=static_cast<const std::int16_t*>(p);for(unsigned i=0;i<256;++i)assert(std::abs(int(s[i]))<=12000);++outputs;std::this_thread::sleep_for(std::chrono::milliseconds(2));}
    return 0;
}
extern "C" int sceAudioOutClose(int h){assert(h==17);++closes;--active;return 0;}
static std::vector<std::int16_t> load(unsigned rev,unsigned clip){assert(rev==80&&clip<9);loaded=static_cast<int>(clip);return std::vector<std::int16_t>(24000,12000);}
template<class Test>static void wait(Test test){for(int i=0;i<1000&&!test();++i)std::this_thread::sleep_for(std::chrono::milliseconds(1));assert(test());}
int main(){
    ps5_items::Item held{0x80401100,0,1,true,0},spent=held;spent.quantity=0;
    for(unsigned type=0;type<5;++type)for(unsigned level=0;level<3;++level){held.type=type;held.level=level;ps5_speaker::Event event;assert(event.update(true,held,1,true)==-1);spent=held;spent.quantity=0;assert(event.update(true,spent,20,false)==static_cast<int>(ps5_speaker::clip_for(held)));assert(event.update(true,spent,21,false)==-1);}
    held={0x80401100,1,1,true,0};spent=held;spent.quantity=0;
    ps5_speaker::Event events;
    assert(events.update(true,held,1,false)==-1&&events.update(true,spent,2,false)==-1); // Lost item without use.
    assert(events.update(true,held,10,true)==-1);assert(events.update(false,spent,11,false)==-1);assert(events.update(true,spent,12,false)==-1);
    {
        ps5_speaker::Speaker speaker(load);
        speaker.update(false,42,held,1,true,0.25F,80);assert(opens==0);
        speaker.update(true,42,held,2,true,0.25F,80);assert(opens==0);
        speaker.update(true,42,spent,20,false,0.25F,80);wait([]{return outputs.load()>0;});assert(loaded==3);
        const auto before=outputs.load();
        speaker.update(true,42,{},25,false,0.25F,80);wait([]{return active.load()==0;});assert(outputs<=before+1&&closes==1);
        fail_open=1;
        speaker.update(true,42,held,30,true,0.25F,80);speaker.update(true,42,spent,40,false,0.25F,80);wait([&]{return speaker.status()<0;});const auto failures=opens.load();
        speaker.update(true,42,held,50,true,0.25F,80);speaker.update(true,42,spent,60,false,0.25F,80);std::this_thread::sleep_for(std::chrono::milliseconds(10));assert(opens==failures);
        speaker.update(false,42,{},70,false,0.25F,80);fail_open=0;
        speaker.update(true,42,held,80,true,0.25F,80);speaker.update(true,42,spent,90,false,0.25F,80);wait([]{return active.load()==1;});
    }
    assert(active==0&&opens==closes+1); // One failed open owns no port.
    puts("PASS actual controller audio worker: all five item types/levels, release-use edge, no pickup/loss sound, no port while disabled/idle, correct native user/route/format, bounded gain, pause cancellation, failure isolation and shutdown close");
}
