// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5_hdr_policy.hpp"
#include <cassert>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <mutex>
#include <thread>
#include <unistd.h>
namespace fs=std::filesystem;
static bool close(double a,double b,double e=1e-5){return std::abs(a-b)<e;}
static std::mutex sync_mutex;
static std::condition_variable sync_wake;
static bool stall_sync=false,sync_entered=false,release_sync=false,snapshot_complete=false;
extern "C" int __real_fsync(int);
extern "C" int __wrap_fsync(int fd){
    {std::unique_lock lock(sync_mutex);if(stall_sync){sync_entered=true;sync_wake.notify_all();sync_wake.wait(lock,[]{return release_sync;});}}
    return __real_fsync(fd);
}
static void snapshot_during_save(){
    {std::lock_guard lock(sync_mutex);stall_sync=true;sync_entered=release_sync=snapshot_complete=false;}
    int saved=-1;std::thread writer([&]{saved=dkrr_ps5_choose_hdr({1,160,600,10});});
    {std::unique_lock lock(sync_mutex);assert(sync_wake.wait_for(lock,std::chrono::seconds(3),[]{return sync_entered;}));}
    std::thread renderer([]{
        const auto c=dkrr_ps5_hdr_calibration();assert(dkrr::hdr::valid(c));
        {std::lock_guard lock(sync_mutex);snapshot_complete=true;}sync_wake.notify_all();
    });
    bool completed_while_save_blocked;
    {std::unique_lock lock(sync_mutex);completed_while_save_blocked=sync_wake.wait_for(lock,std::chrono::seconds(3),[]{return snapshot_complete;});release_sync=true;stall_sync=false;}
    sync_wake.notify_all();writer.join();renderer.join();
    assert(saved==0 && completed_while_save_blocked);
    const dkrr_ps5_hdr_choice a{1,80,400,0},b{0,300,2000,100};
    dkrr_ps5_hdr_preview(a);std::atomic<bool> done{false};
    std::thread preview([&]{for(unsigned i=0;i<100000;++i)dkrr_ps5_hdr_preview(i&1?a:b);done=true;});
    do {const auto c=dkrr_ps5_hdr_calibration();assert((c.enabled==a.enabled&&c.paper_white==a.paper_white&&c.peak==a.peak&&c.expansion==a.expansion)||(c.enabled==b.enabled&&c.paper_white==b.paper_white&&c.peak==b.peak&&c.expansion==b.expansion));}while(!done);
    preview.join();
}
int main(){
    using namespace dkrr::hdr;
    assert(valid({0,200,800,35})&&!valid({2,200,800,35})&&!valid({1,79,800,35})&&!valid({1,200,399,35})&&!valid({1,200,800,101}));
    assert(pq(0)==0&&pq(-1)==0&&close(pq(100),0.5080784215)&&close(pq(1000),0.7518270962));
    assert(close(linear(0),0)&&close(linear(1),1)&&close(linear(0.04045),0.003130805));
    auto black=compose({0,0,0},{0,0,0,0},{1,200,800,100});assert(black[0]==0&&black[1]==0&&black[2]==0);
    auto white=compose({1,1,1},{0,0,0,0},{1,200,800,100});assert(close(white[0],pq(800)));
    auto ui=compose({1,1,1},{1,1,1,1},{1,200,800,100});assert(close(ui[0],pq(200)));
    auto transparent=compose({0.3,0.4,0.8},{1,1,1,0},{1,200,800,35});auto without=compose({0.3,0.4,0.8},{0,0,0,0},{1,200,800,35});assert(transparent==without);
    auto half=compose({0,0,0},{0.5,0.5,0.5,0.5},{1,200,800,100});assert(close(half[0],pq(100)));
    double previous=0;
    for(unsigned i=0;i<=10000;++i){double v=i/10000.0;auto c=compose({v,v,v},{0,0,0,0},{1,200,800,35});assert(std::isfinite(c[0])&&c[0]>=previous&&c[0]<=pq(800)+1e-6);previous=c[0];}
    fs::path root=DKR_DISPLAY_DATA_ROOT;fs::create_directories(root/"preferences");
    dkrr_ps5_hdr_choice c{};assert(dkrr_ps5_read_hdr_choice(&c)==0&&!c.enabled);
    assert(!dkrr_ps5_choose_hdr({1,180,700,25}));assert(dkrr_ps5_read_hdr_choice(&c)==1&&c.paper_white==180);
    dkrr_ps5_initialize_hdr();assert(dkrr_ps5_hdr_requested()==1&&dkrr_ps5_hdr_calibration().peak==700);
    assert(!dkrr_ps5_choose_hdr({0,200,900,0}));assert(dkrr_ps5_hdr_requested()==1); // next launch, never mid-frame
    dkrr_ps5_initialize_hdr();assert(!dkrr_ps5_hdr_requested());
    dkrr_ps5_hdr_preview({1,220,800,10});assert(dkrr_ps5_hdr_calibration().paper_white==220);
    assert(dkrr_ps5_read_hdr_choice(&c)==1&&c.paper_white==200); // preview never writes
    dkrr_ps5_hdr_calibration_pattern(1);assert(dkrr_ps5_hdr_pattern());dkrr_ps5_hdr_calibration_pattern(0);assert(!dkrr_ps5_hdr_pattern());
    dkrr_ps5_hdr_report(3,2);assert(dkrr_ps5_hdr_status().output==3&&dkrr_ps5_hdr_status().reason==2);
    snapshot_during_save();
    auto file=root/"preferences/hdr-output.txt";fs::remove(file);fs::create_symlink(root/"outside",file);
    assert(dkrr_ps5_read_hdr_choice(&c)==-1&&!c.enabled);assert(dkrr_ps5_choose_hdr({1,200,800,35})==-1&&!fs::exists(root/"outside"));
    fs::remove(file);std::ofstream(file)<<"hdr=1\npaper_white=200\npeak=800\nexpansion=35\nextra=1\n";
    assert(dkrr_ps5_read_hdr_choice(&c)==-1&&!c.enabled);assert(dkrr_ps5_choose_hdr({1,200,800,35})==-1);
    std::puts("PASS HDR reference/PQ/gamut, 10001 monotonic luminances, UI white/alpha, settings, next-launch semantics, preview isolation, unsafe-file refusal and coherent render snapshots during blocked fsync");
}
