// SPDX-License-Identifier: GPL-3.0-or-later
#include <atomic>
#include <cassert>
#include <filesystem>
#include <functional>
#include <future>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
static bool fail_next=false;
template<class F>auto fixture_async(std::launch policy,F action){
    if(fail_next){fail_next=false;throw std::runtime_error("thread creation refused");}
    return std::async(policy,std::move(action));
}
static std::string g_save_manager_status;
static std::optional<int> g_save_builder_image;
static unsigned invalidations=0;
void InvalidateSaveManagerViewCache(){++invalidations;}
#include "shared_save_jobs.inc"
int main(){
    std::atomic_bool entered=false,released=false;
    fail_next=true;assert(!BeginSharedSaveJob([](std::string&){return true;},"done"));
    assert(!SharedSaveBusy()&&g_save_manager_status.find("could not start")!=std::string::npos);
    assert(BeginSharedSaveJob([&](std::string&){entered=true;while(!released.load())std::this_thread::yield();return true;},"saved"));
    while(!entered.load())std::this_thread::yield();assert(SharedSaveBusy());
    assert(!BeginSharedSaveJob([](std::string&){assert(false);return false;},"wrong"));
    g_save_builder_image=1;released=true;g_shared_save_job.wait();PumpSharedSaveJob();
    assert(!SharedSaveBusy()&&!g_save_builder_image&&invalidations==1&&g_save_manager_status=="saved");
    assert(BeginSharedSaveJob([](std::string&)->bool{throw std::runtime_error("disk failure");},"wrong"));
    g_shared_save_job.wait();PumpSharedSaveJob();assert(!SharedSaveBusy()&&invalidations==1&&g_save_manager_status=="disk failure");
    assert(BeginSharedSaveJob([](std::string& error){error="save refused";return false;},"wrong"));
    g_shared_save_job.wait();PumpSharedSaveJob();assert(!SharedSaveBusy()&&invalidations==1&&g_save_manager_status=="save refused");
}
