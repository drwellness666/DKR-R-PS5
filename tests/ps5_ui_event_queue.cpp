// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5_ui_event_queue.hpp"
#include <cassert>
#include <thread>
#include <atomic>
#include <cstdio>
int main(){
    dkrr::UiEventQueue q;SDL_Event e{};e.type=SDL_CONTROLLERSENSORUPDATE;
    for(int i=0;i<500000;++i)assert(!q.push(e));assert(q.take().events.empty());
    e.type=SDL_DROPFILE;e.drop.file=(char*)"not owned";assert(!q.push(e));
    e={};e.type=SDL_KEYUP;e.key.keysym.scancode=SDL_SCANCODE_A;assert(q.push(e));auto batch=q.take();assert(batch.events.size()==1&&batch.events[0].key.keysym.scancode==SDL_SCANCODE_A);
    e.type=SDL_KEYDOWN;for(int i=0;i<513;++i)assert(q.push(e));batch=q.take();assert(batch.reset_inputs&&batch.events.size()==1);assert(!q.take().reset_inputs);
    // No render/frame mutex is part of push: a blocked UI consumer cannot
    // keep the SDL owner from draining the sensor stream or queuing releases.
    std::atomic<bool> done=false;std::thread producer([&]{for(int i=0;i<100000;++i){SDL_Event key{};key.type=i%2?SDL_KEYUP:SDL_KEYDOWN;q.push(key);}done=true;});
    for(int i=0;i<1000&&!done;++i){batch=q.take();assert(batch.events.size()<=512);std::this_thread::yield();}
    producer.join();batch=q.take();assert(batch.events.size()<=512);q.clear();assert(q.take().events.empty());
    puts("PASS bounded UI event handoff: 500k sensor packets, owned data only, overflow release reset and concurrent producer/consumer");
}
