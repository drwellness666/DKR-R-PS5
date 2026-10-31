// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5_controller_speaker.hpp"
#include "ps5_pcm_reader.hpp"
#include <cstring>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <thread>
using namespace dkrr;
static std::atomic<int> opens{0},outputs{0},closes{0},active{0},loaded{-1},fail_open{0},mixed_blocks{0},peak_sample{0};
static std::atomic<unsigned> load_calls{0},loaded_revision{0};
static std::atomic<bool> block_output{false},output_entered{false},output_release{false};
static std::atomic<bool> delay_open{false},open_entered{false},open_release{false};
extern "C" int sceAudioOutOpen(int user,int type,int index,unsigned length,unsigned hz,unsigned format) {
    assert(user==42&&type==4&&index==0&&length==256&&hz==48000&&format==0);
    ++opens;if(delay_open.exchange(false)){open_entered=true;while(!open_release.load())std::this_thread::yield();return -77;}
    if(fail_open)return -77;++active;return 17;
}
extern "C" int sceAudioOutOutput(int h,const void*p) {
    assert(h==17);
    if(p){if(block_output.load()){output_entered=true;while(!output_release.load())std::this_thread::yield();}const auto*s=static_cast<const std::int16_t*>(p);for(unsigned i=0;i<256;++i)assert(std::abs(int(s[i]))<=32768);if(s[0]>=6000)++mixed_blocks;auto peak=peak_sample.load();while(s[0]>peak&&!peak_sample.compare_exchange_weak(peak,s[0])){}++outputs;std::this_thread::sleep_for(std::chrono::milliseconds(2));}
    return 0;
}
extern "C" int sceAudioOutClose(int h){assert(h==17);++closes;--active;return 0;}
static std::vector<std::int16_t> load(unsigned rev,unsigned clip){assert((rev==77||rev==80)&&clip<ps5_sounds::count);++load_calls;loaded_revision=rev;loaded=static_cast<int>(clip);return std::vector<std::int16_t>(24000,12000);}
static std::vector<std::int16_t> loop_load(unsigned rev,unsigned clip){
 auto data=load(rev,clip);if(clip!=7)return data;
 const std::uint32_t meta[6]={0x4c524b44,1,0,unsigned(data.size()),4800,0};
 data.insert(data.begin(),12,0);std::memcpy(data.data(),meta,sizeof(meta));return data;
}
static std::atomic<bool> delayed_load{true},load_entered{false},load_release{false};
static std::vector<std::int16_t> delayed(unsigned rev,unsigned clip){
 if(delayed_load.exchange(false)){load_entered=true;while(!load_release.load())std::this_thread::yield();return {};}
 return load(rev,clip);
}
template<class Test>static void wait(Test test){for(int i=0;i<1000&&!test();++i)std::this_thread::sleep_for(std::chrono::milliseconds(1));assert(test());}
int main(){
    for(unsigned clip=0;clip<ps5_sounds::count;++clip){const auto category=ps5_sounds::category(clip);assert(category<10);if(category<10){assert(ps5_sounds::allowed(1023,clip));assert(!ps5_sounds::allowed(1023&~(1U<<category),clip));}}
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
        speaker.update(false,42,{},25,false,0.25F,80);wait([]{return active.load()==0;});assert(outputs<=before+1&&closes==1);
        fail_open=1;
        speaker.update(true,42,held,30,true,0.25F,80);speaker.update(true,42,spent,40,false,0.25F,80);wait([&]{return speaker.status()<0;});const auto failures=opens.load();
        speaker.update(true,42,held,50,true,0.25F,80);speaker.update(true,42,spent,60,false,0.25F,80);std::this_thread::sleep_for(std::chrono::milliseconds(10));assert(opens==failures);
        speaker.update(false,42,{},70,false,0.25F,80);fail_open=0;
        speaker.update(true,42,held,80,true,0.25F,80);speaker.update(true,42,spent,90,false,0.25F,80);wait([]{return active.load()==1;});
    }
    assert(active==0&&opens==closes+1); // One failed open owns no port.
    assert(load_calls==1); // The retry path reuses the already loaded cue.
    {
        ps5_speaker::Speaker speaker(load);
        const auto before=load_calls.load();
        for(unsigned i=0;i<3;++i) {
            const auto old_opens=opens.load(),old_closes=closes.load();
            const unsigned revision=i==2?77:80;
            speaker.update(true,42,held,100+i*30,true,0.25F,revision);
            speaker.update(true,42,spent,120+i*30,false,0.25F,revision);
            wait([&]{return opens.load()>old_opens;});wait([&]{return closes.load()>old_closes;});
            assert(load_calls==before+(i==2?2U:1U));
            assert(loaded_revision==revision);
        }
    }
    {
        ps5_speaker::Speaker speaker(load);
        const auto old=opens.load();
        speaker.update(true,42,{},500,false,0.25F,77,ps5_sounds::horn+9);
        wait([&]{return opens.load()>old;});assert(loaded==int(ps5_sounds::horn+9));
        speaker.update(false,42,{},501,false,0.25F,77);wait([]{return active.load()==0;});
        speaker.update(true,42,held,600,true,0.25F,77);
        speaker.update(true,42,spent,620,false,0.25F,77);wait([]{return active.load()==1;});
        const auto during=loaded.load();speaker.update(true,42,spent,621,false,0.25F,77,ps5_sounds::silver);
        wait([&]{return loaded.load()!=during;});assert(loaded==int(ps5_sounds::silver));
        speaker.update(false,42,{},630,false,0.25F,77);wait([]{return active.load()==0;});
    }
    {
        // Every selected event starts against the TV backlog deadline. A horn
        // can overlap a pickup; neither is discarded by a busy/cooldown gate.
        ps5_speaker::Speaker speaker(load);
        const auto before=outputs.load(),old_closes=closes.load();const auto old_loads=load_calls.load();
        const auto at=std::chrono::steady_clock::now();
        speaker.update(true,42,{},1000,false,0.25F,77,ps5_sounds::pickup,80);
        wait([&]{return load_calls.load()>old_loads;});
        std::this_thread::sleep_for(std::chrono::milliseconds(15));assert(outputs==before);
        wait([&]{return outputs.load()>before;});
        assert(std::chrono::steady_clock::now()-at>=std::chrono::milliseconds(70));
        speaker.update(true,42,{},1001,false,0.25F,77,ps5_sounds::horn,80);
        wait([&]{return closes.load()>old_closes;});
        assert(load_calls==old_loads+2&&outputs>before+96); // Both complete voices, including the later tail.
    }
    {
        // Two real boosts arrive before the first clip ends. They are mixed
        // immediately through one native output, with both complete tails.
        ps5_speaker::Speaker speaker(load);
        const auto before=opens.load(),old_closes=closes.load(),old_mix=mixed_blocks.load();
        speaker.update(true,42,{},1200,false,0.25F,77,ps5_sounds::zipper);
        wait([&]{return opens.load()>before;});
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
        speaker.update(true,42,{},1400,false,0.25F,77,ps5_sounds::zipper);
        wait([&]{return mixed_blocks.load()>old_mix;});
        wait([&]{return closes.load()>old_closes;});
        assert(opens==before+1&&active==0); // No extra output or delayed queue.
    }
    {
        // A rapid horn burst is not collapsed, serialized, or made to wait
        // for the first horn's end. All16 voices overlap on the one output.
        ps5_speaker::Speaker speaker(load);const auto old_closes=closes.load(),old_opens=opens.load();peak_sample=0;
        for(unsigned i=0;i<16;++i){speaker.update(true,42,{},2000+i*5,false,1.0F/64,77,ps5_sounds::horn,100);std::this_thread::sleep_for(std::chrono::milliseconds(1));}
        // Retry a contended handoff without inventing another sound event.
        for(unsigned i=0;i<10;++i){speaker.update(true,42,{},2100+i,false,1.0F/64,77);std::this_thread::sleep_for(std::chrono::milliseconds(1));}
        wait([&]{return peak_sample.load()>=16*187;});wait([&]{return closes.load()>old_closes;});assert(opens==old_opens+1);
    }
    {
        ps5_speaker::Speaker speaker(load);block_output=true;output_entered=false;output_release=false;
        speaker.update(true,42,{},2300,false,0.25F,77,ps5_sounds::horn);wait([]{return output_entered.load();});
        std::atomic<bool> returned{false};std::thread input([&]{for(unsigned i=0;i<16;++i)speaker.update(true,42,{},2301+i,false,0.25F,77,ps5_sounds::horn);returned=true;});
        for(unsigned i=0;i<100&&!returned.load();++i)std::this_thread::sleep_for(std::chrono::milliseconds(1));
        const bool nonblocking=returned;output_release=true;input.join();block_output=false;assert(nonblocking);
        speaker.update(false,42,{},2400,false,0.25F,77);wait([]{return active.load()==0;});
    }
    {
        auto data=loop_load(77,7);auto loop=ps5_speaker::unpack_shield(data);
        assert(loop&&loop.start==0&&loop.end==24000&&loop.release==4800&&data.size()==24000&&data.front()==12000);
        for(unsigned bad=0;bad<6;++bad){auto malformed=loop_load(77,7);std::uint32_t field=bad==2?25000:bad==3?0:bad==4?96001:99;std::memcpy(malformed.data()+bad*2,&field,4);assert(!ps5_speaker::unpack_shield(malformed)&&malformed.empty());}
        ps5_speaker::Speaker speaker(loop_load);ps5_items::Item shield=spent;shield.shield=true;
        const auto before=outputs.load(),old_closes=closes.load();
        speaker.update(true,42,shield,1500,false,0.25F,77);
        wait([&]{return outputs.load()>=before+120;}); // Longer than one 94-block source cycle.
        assert(active==1);
        const auto stopping=outputs.load();shield.shield=false;
        speaker.update(true,42,shield,1800,false,0.25F,77);
        wait([&]{return closes.load()>old_closes;});
        assert(outputs>=stopping+19&&outputs<=stopping+23); // Original metadata release + paced tail.
    }
    {
        // A cancelled load belongs to the old generation. Its missing-file
        // result must not disable a newer request after an off/on transition.
        ps5_speaker::Speaker speaker(delayed);const auto before=opens.load();
        speaker.update(true,42,held,800,true,0.25F,77);
        speaker.update(true,42,spent,820,false,0.25F,77);wait([]{return load_entered.load();});
        speaker.update(false,42,{},830,false,0.25F,77);
        speaker.update(true,42,held,840,true,0.25F,77);
        speaker.update(true,42,spent,860,false,0.25F,77);load_release=true;
        wait([&]{return opens.load()>before;});wait([]{return active.load()==0;});
        assert(speaker.status()==1);
    }
    {
        ps5_speaker::Speaker speaker(load);const auto before=opens.load();delay_open=true;
        speaker.update(true,42,held,900,true,0.25F,77);
        speaker.update(true,42,spent,920,false,0.25F,77);wait([]{return open_entered.load();});
        speaker.update(false,42,{},930,false,0.25F,77);
        speaker.update(true,42,held,940,true,0.25F,77);
        speaker.update(true,42,spent,960,false,0.25F,77);open_release=true;
        wait([&]{return opens.load()>before+1;});wait([]{return active.load()==0;});
        assert(speaker.status()==1);
    }
    {
        // All eight pitches and the original completion voice reach the worker
        // when only Silver coins is enabled; unrelated cues stay filtered.
        ps5_speaker::Speaker speaker(load);
        const unsigned old=load_calls.load();
        for(unsigned i=0;i<9;++i){
            speaker.update(true,42,{},6000+i*20,false,0.25F,77,ps5_sounds::silver+i,0,1U<<7);
            wait([&]{return load_calls.load()==old+i+1;});
            assert(loaded==int(ps5_sounds::silver+i));
        }
        speaker.update(true,42,{},6200,false,0.25F,77,ps5_sounds::horn,0,1U<<7);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));assert(load_calls==old+9);
        speaker.update(false,42,{},6300,false,0.25F,77);wait([]{return active.load()==0;});
    }
    puts("PASS actual controller audio worker: all five item types/levels, release-use edge, no invented pickup/loss event, real non-item cues, rapid horn and selected cue overlap without suppression, simultaneous zipper boosts, live shield loop and original release through one output, bounded metadata, and item priority, no port while disabled/idle, correct native user/route/format, bounded gain, pause cancellation, failure isolation, cancelled load/open errors cannot disable newer requests, and shutdown close");
}
