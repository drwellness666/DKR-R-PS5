// SPDX-License-Identifier: GPL-3.0-or-later
// Optional app-owned speaker worker. No separate payload/process or TV reroute.
#include "ps5_controller_speaker.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <thread>
#include <vector>
extern "C" int sceAudioOutOpen(int,int,int,unsigned,unsigned,unsigned);
extern "C" int sceAudioOutOutput(int,const void*);
extern "C" int sceAudioOutClose(int);
namespace dkrr::ps5_speaker {
struct Speaker::Worker {
    std::mutex mutex;std::condition_variable wake;
    bool quit=false,enabled=false;int user=-1,clip=-1;unsigned revision=77,generation=0;
    float volume=0.25F;std::atomic<int> result{0};
    Loader loader;
    int output_handle=-1;
    unsigned cached_revision=0;
    std::array<std::vector<std::int16_t>,9> cached_cues;
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
        std::vector<std::int16_t> data;
        for(const char*root:{"/app0/assets/controller-audio","/data/homebrew/PPSA94678/assets/controller-audio"}) {
            const auto p=std::filesystem::path(root)/("us-v"+std::to_string(rev))/(std::to_string(index)+".pcm");
            std::error_code ec;const auto n=std::filesystem::file_size(p,ec);
            if(ec||!n||n%2||n>48000U*2U*4U||!std::filesystem::is_regular_file(p,ec)||std::filesystem::is_symlink(p,ec))continue;
            std::ifstream f(p,std::ios::binary);data.resize(n/2);
            if(f.read(reinterpret_cast<char*>(data.data()),static_cast<std::streamsize>(n)))return data;
            data.clear();
        }
        return data;
    }
    const std::vector<std::int16_t>& cue(unsigned rev,unsigned index) {
        // Only the audio worker owns this bounded cache. Repeated item use
        // performs no filesystem reads, sample copies or missile mixing.
        if(cached_revision!=rev){cached_cues={};cached_revision=rev;}
        auto& data=cached_cues[index];
        if(!data.empty())return data;
        data=load(rev,index);
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
    void run() {
        int& handle=output_handle;int open_user=-1;unsigned seen=0;std::unique_lock lock(mutex);
        while(!quit) {
            wake.wait(lock,[&]{return quit||generation!=seen;});if(quit)break;
            seen=generation;const bool play=enabled;const auto requested=clip;const auto id=user;const auto rev=revision;
            if(!play||requested<0||requested>8){lock.unlock();if(handle>=0){sceAudioOutOutput(handle,nullptr);sceAudioOutClose(handle);handle=-1;}lock.lock();continue;}
            // A failure stays local to this optional output until it is toggled.
            if(result.load()<0)continue;
            lock.unlock();
            const auto& data=cue(rev,static_cast<unsigned>(requested));
            if(data.empty()){result.store(-2);std::fprintf(stderr,"[input][speaker] private PCM unavailable\n");lock.lock();continue;}
            if(handle>=0&&open_user!=id){sceAudioOutOutput(handle,nullptr);sceAudioOutClose(handle);handle=-1;}
            if(handle<0) {
                // Public AudioOut ABI: PADSPK=4, S16 mono=0, 48 kHz. Route is
                // experimental on native PS5 until audibly checked on hardware.
                handle=sceAudioOutOpen(id,4,0,256,48000,0);open_user=id;
                if(handle<0){result.store(-1);std::fprintf(stderr,"[input][speaker] native open failed rc=0x%08x\n",unsigned(handle));lock.lock();continue;}
            }
            result.store(1);std::array<std::int16_t,256> block{};
            for(std::size_t pos=0;pos<data.size();pos+=block.size()) {
                lock.lock();const bool cancelled=quit||!enabled||generation!=seen;const float gain=volume;lock.unlock();
                if(cancelled)break;
                block.fill(0);
                for(std::size_t i=0;i<block.size()&&pos+i<data.size();++i)block[i]=static_cast<std::int16_t>(float(data[pos+i])*gain);
                const int rc=sceAudioOutOutput(handle,block.data());
                if(rc<0){result.store(-1);std::fprintf(stderr,"[input][speaker] native output failed rc=0x%08x\n",unsigned(rc));break;}
            }
            // Drain/close after each short cue; never retain a silent speaker port.
            if(handle>=0){sceAudioOutOutput(handle,nullptr);sceAudioOutClose(handle);handle=-1;}
            lock.lock();
        }
        lock.unlock();if(handle>=0){sceAudioOutOutput(handle,nullptr);sceAudioOutClose(handle);}
    }
};
Speaker::Speaker():Speaker(nullptr){};Speaker::Speaker(Loader load):loader(load){};Speaker::~Speaker()=default;
Speaker::Speaker(Speaker&&) noexcept=default;Speaker&Speaker::operator=(Speaker&&) noexcept=default;
int Speaker::status() const {return start_failed?-3:worker?worker->result.load():0;}
void Speaker::update(bool enabled,int user,ps5_items::Item item,std::uint64_t ms,bool pressed,float volume,unsigned revision) {
    enabled=enabled&&user>=0&&item.valid&&(revision==77||revision==80);
    const int clip=event.update(enabled,item,ms,pressed);
    if(!enabled)start_failed=false;
    if(!worker&&enabled&&!start_failed)try {worker=std::make_unique<Worker>(loader);}
    catch(const std::exception& e){start_failed=true;std::fprintf(stderr,"[input][speaker] unavailable: %s\n",e.what());}
    if(!worker)return;
    {std::lock_guard lock(worker->mutex);
        if(!enabled||worker->user!=user) {
            if(worker->enabled){++worker->generation;worker->wake.notify_one();}
            worker->enabled=false;if(worker->result.load()!=-3)worker->result.store(0);
        }
        worker->user=user;worker->revision=revision;worker->volume=std::isfinite(volume)?std::clamp(volume,0.0F,1.0F):0.0F;
        worker->enabled=enabled;
        if(clip>=0){worker->clip=clip;++worker->generation;worker->wake.notify_one();}
    }
}
}
