#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise actual guest-entry/online-route checks with split config/save roots."""
import argparse,os,pathlib,subprocess,tempfile
r=pathlib.Path(__file__).resolve().parent.parent;p=argparse.ArgumentParser();p.add_argument('game_work',type=pathlib.Path);a=p.parse_args();game=a.game_work/'source/DKR-R/runtime-recomp/src/game'
def function(file,signature):
 text=(game/file).read_text();start=text.index(signature);begin=text.index('{',start);end=begin+1;depth=1
 while depth:
  depth+=(text[end]=='{')-(text[end]=='}');end+=1
 return text[start:end]+'\n'
code=r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
namespace fs=std::filesystem;
struct recomp_context{};
unsigned entries,quits,mutable_changes,net_failures;
bool ready;
fs::path active_path,owned_path,readback_path;
namespace ultramodern{
void quit(){++quits;}
fs::path get_save_file_path(){return active_path;}
void change_save_file(const std::u8string&,const std::u8string&){++mutable_changes;quit();}
}
namespace recomp {constexpr std::size_t mem_size=8*1024*1024;}
namespace dkr::runtime {
struct GamePayload{void(*entrypoint)(std::uint8_t*,recomp_context*);};
void enter(std::uint8_t*,recomp_context*){++entries;}
GamePayload payload{enter};const GamePayload*active_payload(){return &payload;}
namespace netplay{struct Session{void fail_runtime_start(const std::string&){++net_failures;}};Session object;Session&session(){return object;}}
namespace legacy{
struct Prepared{bool session=true;fs::path save_subfolder,save_path;};
std::shared_ptr<Prepared> mods;auto prepared_launch(){return mods;}
}
namespace code_mods {void boot(std::uint8_t*,std::size_t){}}
namespace saves {
struct Selection{bool online=false,host=false;std::uint64_t match=0,hash=0;} selection;
struct Status{bool active=false,verified=false;std::uint64_t hash=0;fs::path path;};
bool runtime_save_context_ready(){return ready;}
std::string runtime_save_failure(){return "fixture route not loaded";}
fs::path runtime_save_path(){return owned_path;}
auto runtime_save_selection(){return selection;}
void reset_runtime_online_save_status();
bool activate_online_save_for_runtime(bool,std::uint64_t,std::uint64_t,std::string&);
bool read_online_adventure(bool,std::uint64_t,std::vector<std::uint8_t>&v,fs::path&path,std::string&){v.assign(512,7);path=readback_path;return true;}
fs::path online_adventure_subfolder(bool,std::uint64_t){return "online/fixture";}
}}
std::mutex g_runtime_save_mutex;
struct{bool online=false,host=false,loaded=false;std::uint64_t match=0,hash=0;std::string failure;} g_context;
dkr::runtime::saves::Status g_runtime_save_status;
std::uint64_t HashSave(const std::vector<std::uint8_t>&v){assert(v.size()==512);return 1234;}
void dkr::runtime::saves::reset_runtime_online_save_status(){g_runtime_save_status={};}
#define __PROSPERO__ 1
'''
code+=function('runtime_save_routing.cpp','bool dkr::runtime::saves::activate_online_save_for_runtime(')
code+=function('game_registration.cpp','void RunDkrEntrypoint(')
code+=r'''
void reset(){entries=quits=mutable_changes=net_failures=0;ready=true;g_context={};g_runtime_save_status={};dkr::runtime::saves::selection={};dkr::runtime::legacy::mods.reset();}
int main(){
 for(const auto* profile:{"profile-us-v77","profile-us-v80"}){
  reset();owned_path=fs::path("/data/dkrr")/profile/"saves/dkr.us.v77.bin";active_path=owned_path;RunDkrEntrypoint(nullptr,nullptr);assert(entries==1&&!quits&&!mutable_changes);
 }
 reset();owned_path="/data/dkrr/profile-us-v80/saves/dkr.us.v77.bin";active_path="/data/dkrr/profile-us-v77/saves/dkr.us.v77.bin";RunDkrEntrypoint(nullptr,nullptr);assert(!entries&&quits==1&&!mutable_changes);
 reset();ready=false;RunDkrEntrypoint(nullptr,nullptr);assert(!entries&&quits==1);
 reset();owned_path="/data/dkrr/profile-us-v80/saves/mods/fingerprint/dkr.us.v77.bin";active_path=owned_path;
 dkr::runtime::legacy::mods=std::make_shared<dkr::runtime::legacy::Prepared>();dkr::runtime::legacy::mods->save_path=owned_path;dkr::runtime::legacy::mods->save_subfolder="mods/fingerprint";
 RunDkrEntrypoint(nullptr,nullptr);assert(entries==1&&!quits&&!mutable_changes);
 entries=quits=0;dkr::runtime::legacy::mods->save_path="/data/dkrr/profile-us-v77/saves/mods/fingerprint/dkr.us.v77.bin";RunDkrEntrypoint(nullptr,nullptr);assert(entries==1&&!quits&&!mutable_changes); // Frozen context wins over a later mutable mod selection.
 reset();owned_path="/data/dkrr/profile-us-v80/saves/online/fixture/dkr.us.v77.bin";active_path=readback_path=owned_path;
 dkr::runtime::saves::selection={true,true,42,1234};g_context={true,true,true,42,1234,{}};
 RunDkrEntrypoint(nullptr,nullptr);assert(entries==1&&!quits&&!mutable_changes&&g_runtime_save_status.active&&g_runtime_save_status.verified);
 entries=quits=0;g_context.hash=999;RunDkrEntrypoint(nullptr,nullptr);assert(!entries&&quits==1&&net_failures==1&&!mutable_changes);
 puts("PASS actual native guest-entry and online route code: both US roots enter, wrong/unloaded contexts refuse, mutable mod selection cannot change the frozen save owner, verified online context retained; no mutable route reconstructed from shared config");
}
'''
with tempfile.TemporaryDirectory(prefix='dkrr-shared-save-entry-') as tmp:
 t=pathlib.Path(tmp);(t/'test.cpp').write_text(code);out=t/'test'
 subprocess.run(['clang++','-std=c++20','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(t/'test.cpp'),'-o',str(out)],check=True)
 subprocess.run([str(out)],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
