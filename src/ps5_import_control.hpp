// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <chrono>
#include <functional>
#include <stdexcept>
namespace dkrr::imports {
struct Control {
    const std::function<bool()>* keep_running=nullptr;
    std::chrono::steady_clock::time_point deadline{};
    bool cancelled=false,timed_out=false;
};
inline thread_local Control* active=nullptr;
inline void checkpoint() {
    if(!active)return;
    if(std::chrono::steady_clock::now()>=active->deadline){active->timed_out=true;throw std::runtime_error("Import timed out; the installed library is unchanged.");}
    if(active->keep_running&&!(*active->keep_running)()){active->cancelled=true;throw std::runtime_error("Import cancelled; the installed library is unchanged.");}
}
struct Scope {
    Control control;
    Scope(const std::function<bool()>& callback,std::chrono::milliseconds timeout):control{&callback,std::chrono::steady_clock::now()+timeout} {
        if(active)throw std::runtime_error("Nested import jobs are unavailable.");active=&control;
    }
    ~Scope(){active=nullptr;}
};
}
