// SPDX-License-Identifier: GPL-3.0-or-later

#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <system_error>
#include <thread>
static bool fail_next=false;
struct FixtureThread {
 std::jthread real;FixtureThread()=default;FixtureThread(FixtureThread&&)=default;FixtureThread&operator=(FixtureThread&&)=default;
 template<class F>explicit FixtureThread(F f){if(fail_next){fail_next=false;throw std::system_error(std::make_error_code(std::errc::resource_unavailable_try_again));}real=std::jthread(std::move(f));}
 bool joinable()const{return real.joinable();}void join(){real.join();}bool request_stop(){return real.request_stop();}
};
static std::atomic_bool entered=false,finished=false,commit_released=false;
static bool committing=false;
namespace dkr::runtime::texture_packs {
struct TrackPackOwner {};
struct ImportProgress {float fraction=0;std::string stage;bool commit_started=false;};
bool import_archive(const std::filesystem::path&,std::string&status,const std::function<bool(const ImportProgress&)>& progress,const TrackPackOwner*){
 if(committing){
  assert(progress({0.8F,"Committing",true}));entered=true;
  while(!commit_released.load())std::this_thread::yield();
  // A requested stop must not interrupt an already-started atomic commit.
  assert(progress({1.0F,"Committed",true}));status="Complete";finished=true;return true;
 }
 entered=true;
 for(unsigned i=0;i<5000;++i){if(!progress({0.5F,"Reading",false})){status="Cancelled";finished=true;return false;}std::this_thread::sleep_for(std::chrono::milliseconds(1));}
 assert(false);return false;
}
}
namespace dkr::runtime::custom_tracks {void discard_install_temp(const std::filesystem::path&) {}}
#include "texture_state.inc"
#include "texture_start.inc"
std::jthread g_dialog_job_worker;std::mutex g_dialog_job_mutex;std::function<void()>g_dialog_job_finish;bool g_dialog_job_running=false;
void dkrr_ps5_cancel_file_picker(){}
#include "texture_shutdown.inc"
int main(int argc,char**argv){assert(argc==2);const std::string mode=argv[1];
 if(mode=="start"){
  fail_next=true;
  try{assert(!StartTexturePackImport("fixture.zip"));}catch(const std::exception&e){printf("REPRODUCED thread-start exception; import busy=%d: %s\n",int(g_texture_import_state.running),e.what());return 3;}
  assert(!g_texture_import_state.running&&g_texture_import_state.finished&&!g_texture_import_state.succeeded);
 }
 committing=mode=="commit";
 assert(StartTexturePackImport("fixture.zip"));
 while(!entered.load())std::this_thread::yield();
 std::jthread release_commit;
 if(committing)release_commit=std::jthread([]{while(!g_texture_import_cancel_requested.load()&&!commit_released.load())std::this_thread::yield();commit_released=true;});
 dkrr_ps5_shutdown_dialogs();
 const bool clean=!g_texture_import_worker.joinable()&&finished.load();
 if(!clean){puts("REPRODUCED native shutdown returns while texture import worker is active");g_texture_import_cancel_requested=true;commit_released=true;g_texture_import_worker.request_stop();g_texture_import_worker.join();return 4;}
 assert(g_texture_import_state.finished && g_texture_import_state.succeeded==committing);
 puts("PASS actual texture worker start-failure retry, joined cancellation and non-interrupted commit");
}

