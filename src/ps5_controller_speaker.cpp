// SPDX-License-Identifier: GPL-3.0-or-later
// Optional app-owned speaker worker. No separate payload/process or TV reroute.
#include "ps5_controller_speaker.hpp"
#include "ps5_pcm_reader.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cmath>
#include <cstdio>
#include <mutex>
#include <thread>
#include <vector>
extern "C" int sceAudioOutOpen(int,int,int,unsigned,unsigned,unsigned);
extern "C" int sceAudioOutOutput(int,const void*);
extern "C" int sceAudioOutClose(int);
namespace dkrr::ps5_speaker {
struct Speaker::Worker {
    std::mutex mutex;std::condition_variable wake;
    bool quit=false,enabled=false;int user=-1,clip=-1;unsigned revision=77,generation=0,cancel_generation=0;
    float volume=0.25F;std::atomic<int> result{0};
    Loader loader;
    int output_handle=-1;
    unsigned cached_revision=0,active_priority=0;
    bool playing=false,pending=false,shield_active=false;
    ShieldLoop shield_loop;
    std::chrono::steady_clock::time_point audible_at{};
    std::array<std::vector<std::int16_t>,ps5_sounds::count> cached_cues;
    std::thread thread;
    explicit Worker(Loader load):loader(load),thread([this]{
        try { run(); } catch(const std::exception& e) {
            if(output_handle>=0){sceAudioOutOutput(output_handle,nullptr);sceAudioOutClose(output_handle);output_handle=-1;}
            result.store(-3);std::fprintf(stderr,"[input][speaker] worker stopped: %s\n",e.what());
        }
    }){}
    ~Worker(){ {std::lock_guard lock(mutex);quit=true;++generation;}wake.notify_one();if(thread.joinable())thread.join(); }
    std::vector<std::int16_t> load(unsigned rev,unsigned index) {
        if(loader)return loader(rev,index);
        for(const char*root:{"/app0/assets/controller-audio","/data/homebrew/PPSA94678/assets/controller-audio"}) {
            char path[192];
            const int n=std::snprintf(path,sizeof(path),"%s/us-v%u/%u.pcm",root,rev,index);
            if(n<0||static_cast<std::size_t>(n)>=sizeof(path))continue;
            auto data=read_pcm(path);
            if(!data.empty())return data;
        }
        return {};
    }
    const std::vector<std::int16_t>& cue(unsigned rev,unsigned index) {
        // Only the audio worker owns this bounded cache. Repeated item use
        // performs no filesystem reads, sample copies or missile mixing.
        if(cached_revision!=rev){cached_cues={};cached_revision=rev;shield_loop={};}
        auto& data=cached_cues[index];
        if(!data.empty())return data;
        data=load(rev,index);
        if(index==7&&!data.empty())shield_loop=unpack_shield(data);
        if(index==1&&!data.empty()) {
            const auto& follow=cue(rev,2);
            if(!follow.empty()) {
                data.resize(std::max<std::size_t>(data.size(),48000+follow.size()),0);
                for(std::size_t i=0;i<follow.size();++i)
                    data[48000+i]=static_cast<std::int16_t>(std::clamp<int>(int(data[48000+i])+follow[i],-32768,32767));
            }
        }
        return data;
    }
    bool publish_result(unsigned expected,int value) {
        std::lock_guard lock(mutex);
        // Disable/re-enable or a new user can overtake slow file/native I/O.
        // A cancelled request must not overwrite the newer request's status.
        if(quit||!enabled||generation!=expected)return false;
        result.store(value);return true;
    }
    void run() {
        using Clock=std::chrono::steady_clock;
        struct Voice {unsigned clip=0;std::size_t position=0;Clock::time_point starts{};unsigned released=0;bool releasing=false;};
        // Simultaneous live cues, never an audio backlog. Four voices bound
        // both mixing cost and gain when several item/zipper events overlap.
        std::vector<Voice> voices;voices.reserve(4);
        int& handle=output_handle;int open_user=-1;unsigned seen=0,epoch=0;
        std::unique_lock lock(mutex);
        auto state=[&]{playing=false;active_priority=0;for(const auto&v:voices)if(v.clip!=7){playing=true;active_priority=std::max(active_priority,ps5_sounds::priority(v.clip));}};
        while(!quit) {
            if(voices.empty()&&generation==seen)wake.wait(lock,[&]{return quit||generation!=seen;});
            if(quit)break;
            if(!enabled||cancel_generation!=epoch){
                epoch=cancel_generation;voices.clear();state();
                lock.unlock();if(handle>=0){sceAudioOutOutput(handle,nullptr);sceAudioOutClose(handle);handle=-1;}lock.lock();
            }
            if(generation!=seen){
                seen=generation;const bool play=enabled&&pending;pending=false;
                const auto requested=clip;const auto id=user;const auto rev=revision;
                const auto start_at=audible_at;const auto request_epoch=cancel_generation;
                if(play&&requested>=0&&requested<int(ps5_sounds::count)&&result.load()>=0){
                    playing=true;active_priority=std::max(active_priority,ps5_sounds::priority(unsigned(requested)));
                    lock.unlock();const auto&data=cue(rev,unsigned(requested));lock.lock();
                    if(quit||!enabled||cancel_generation!=request_epoch)continue;
                    if(data.empty()){
                        lock.unlock();if(publish_result(seen,-2))std::fprintf(stderr,"[input][speaker] private PCM unavailable\n");lock.lock();state();continue;
                    }
                    if(handle>=0&&open_user!=id){lock.unlock();sceAudioOutOutput(handle,nullptr);sceAudioOutClose(handle);handle=-1;lock.lock();}
                    if(handle<0){
                        // Native PADSPK=4, S16 mono=0, 48 kHz, 256-frame paced blocks.
                        lock.unlock();handle=sceAudioOutOpen(id,4,0,256,48000,0);open_user=id;lock.lock();
                        if(handle<0){lock.unlock();if(publish_result(seen,-1))std::fprintf(stderr,"[input][speaker] native open failed rc=0x%08x\n",unsigned(handle));lock.lock();state();continue;}
                        if(quit||!enabled||cancel_generation!=request_epoch)continue;
                    }
                    // At capacity replace the oldest, lowest-priority voice;
                    // a new event is heard now, never queued for later playback.
                    if(requested==7){
                        const auto existing=std::find_if(voices.begin(),voices.end(),[](const Voice&v){return v.clip==7;});
                        if(existing!=voices.end()){existing->releasing=false;existing->released=0;state();continue;}
                    }
                    if(voices.size()==4){
                        auto victim=std::min_element(voices.begin(),voices.end(),[](const Voice&a,const Voice&b){return (a.clip==7?4U:ps5_sounds::priority(a.clip))<(b.clip==7?4U:ps5_sounds::priority(b.clip));});
                        voices.erase(victim);
                    }
                    voices.push_back({unsigned(requested),0,start_at});state();
                }
            }
            if(voices.empty())continue;
            auto first=voices.front().starts;for(const auto&v:voices)first=std::min(first,v.starts);
            if(Clock::now()<first){wake.wait_until(lock,first,[&]{return quit||generation!=seen;});continue;}
            const float gain=volume;const unsigned output_epoch=cancel_generation;const auto at=Clock::now();
            std::array<int,256> mix{};std::array<std::int16_t,256> block{};
            for(auto&v:voices){
                if(v.starts>at)continue;
                const auto&data=cached_cues[v.clip];
                if(v.clip==7&&shield_loop){
                    if(!shield_active)v.releasing=true;
                    for(std::size_t i=0;i<block.size();++i){
                        if(v.releasing&&v.released>=shield_loop.release)break;
                        if(v.position>=shield_loop.end)v.position=shield_loop.start+(v.position-shield_loop.end)%(shield_loop.end-shield_loop.start);
                        const double fade=v.releasing?1.0-double(v.released)/shield_loop.release:1.0;
                        mix[i]+=int(double(data[v.position++])*fade);if(v.releasing)++v.released;
                    }
                }else{
                    for(std::size_t i=0;i<block.size()&&v.position+i<data.size();++i)mix[i]+=data[v.position+i];
                    v.position+=block.size();
                }
            }
            std::erase_if(voices,[&](const Voice&v){return v.clip==7&&shield_loop ? v.releasing&&v.released>=shield_loop.release : v.position>=cached_cues[v.clip].size();});
            for(std::size_t i=0;i<block.size();++i)block[i]=static_cast<std::int16_t>(std::clamp(float(mix[i])*gain,-32768.0F,32767.0F));
            lock.unlock();const int rc=sceAudioOutOutput(handle,block.data());lock.lock();
            if(rc<0){
                voices.clear();state();lock.unlock();if(publish_result(seen,-1))std::fprintf(stderr,"[input][speaker] native output failed rc=0x%08x\n",unsigned(rc));lock.lock();
            }else if(cancel_generation==output_epoch&&enabled)result.store(1);
            state();
            if(voices.empty()&&handle>=0){
                block.fill(0);
                for(unsigned tail=0;tail<2;++tail){
                    if(quit||!enabled||cancel_generation!=output_epoch||generation!=seen)break;
                    lock.unlock();const int tail_rc=sceAudioOutOutput(handle,block.data());lock.lock();if(tail_rc<0)break;
                }
                lock.unlock();sceAudioOutOutput(handle,nullptr);sceAudioOutClose(handle);handle=-1;lock.lock();
            }
        }
        lock.unlock();if(handle>=0){sceAudioOutOutput(handle,nullptr);sceAudioOutClose(handle);handle=-1;}
    }
};
Speaker::Speaker():Speaker(nullptr){};Speaker::Speaker(Loader load):loader(load){};Speaker::~Speaker()=default;
Speaker::Speaker(Speaker&&) noexcept=default;Speaker&Speaker::operator=(Speaker&&) noexcept=default;
int Speaker::status() const {return start_failed?-3:worker?worker->result.load():0;}
void Speaker::update(bool enabled,int user,ps5_items::Item item,std::uint64_t ms,bool pressed,float volume,unsigned revision,int game_clip,unsigned audio_delay_ms) {
    enabled=enabled&&user>=0&&(revision==77||revision==80);
    int clip=event.update(enabled,item,ms,pressed);
    if(clip<0&&game_clip>=9&&game_clip<int(ps5_sounds::count))clip=game_clip;
    if(!enabled)start_failed=false;
    if(!worker&&enabled&&!start_failed)try {worker=std::make_unique<Worker>(loader);}
    catch(const std::exception& e){start_failed=true;std::fprintf(stderr,"[input][speaker] unavailable: %s\n",e.what());}
    if(!worker)return;
    {std::lock_guard lock(worker->mutex);
        if(!enabled||worker->user!=user||worker->revision!=revision) {
            if(worker->enabled){++worker->generation;++worker->cancel_generation;worker->wake.notify_one();}
            worker->enabled=false;worker->pending=false;if(worker->result.load()!=-3)worker->result.store(0);
        }
        worker->user=user;worker->revision=revision;worker->volume=std::isfinite(volume)?std::clamp(volume,0.0F,1.0F):0.0F;
        worker->enabled=enabled;
        const bool shield=enabled&&item.valid&&item.shield;
        if(shield!=worker->shield_active){
            worker->shield_active=shield;
            if(shield&&clip<0)clip=7;
            ++worker->generation;worker->wake.notify_one();
        }
        // Item uses and distinct zipper boosts can overlap. Other incidental
        // cues keep the quiet single-sound policy; no delayed playback queue.
        const bool busy=worker->playing||worker->pending;
        if(clip>=0&&(!busy||clip==int(ps5_sounds::zipper)||ps5_sounds::priority(unsigned(clip))>=2)){
            worker->clip=clip;worker->pending=true;
            worker->audible_at=std::chrono::steady_clock::now()+std::chrono::milliseconds(std::min(audio_delay_ms,250U));
            ++worker->generation;worker->wake.notify_one();
        }
    }
}
}
