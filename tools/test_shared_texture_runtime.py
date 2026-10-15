#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Run actual library scan/settings/delete code with synthetic archive inspection."""
import argparse,os,pathlib,re,subprocess,tempfile
r=pathlib.Path(__file__).resolve().parent.parent
p=argparse.ArgumentParser();p.add_argument('game_work',type=pathlib.Path);a=p.parse_args()
game=a.game_work/'source/DKR-R/runtime-recomp/src/game';text=(game/'runtime_texture_packs.cpp').read_text()
def function(name):
 match=re.search(r'(?m)^[A-Za-z_][A-Za-z0-9_:<> ,]*\b'+re.escape(name)+r'\(',text)
 if not match:raise ValueError('Function definition missing: '+name)
 start=match.start();at=text.index('(',start)+1;parentheses=1
 while parentheses:
  parentheses+=(text[at]=='(')-(text[at]==')');at+=1
 begin=text.index('{',at);end=begin+1;depth=1
 while depth:
  depth+=(text[end]=='{')-(text[end]=='}');end+=1
 return text[start:end]+'\n'
code=r'''
#include "runtime_texture_packs.hpp"
#include "texture_pack_priority_policy.hpp"
#include "ps5_profile_paths.hpp"
#include "ps5_texture_library.hpp"
#include <cassert>
#include <charconv>
#include <chrono>
#include <fstream>
#include <iterator>
#include <map>
#include <mutex>
#include <set>
#include <stop_token>
#include <type_traits>
#include <cstdio>
using namespace dkr::runtime::texture_packs;
namespace texture_priority=dkr::runtime::texture_priority;
namespace fs=std::filesystem;
namespace dkr::runtime::startup_performance {
using Clock=std::chrono::steady_clock;
void report(const char*,Clock::time_point){}
}
fs::path test_root,g_pack_directory,g_settings_path;
std::mutex g_mutex,g_refresh_mutex;
std::vector<PackInfo> g_packs;
std::set<std::string> g_enabled_ids,g_hidden_ids,g_applied_ids;
texture_priority::Preferences g_priorities;
std::string g_last_selected_id,g_status;
std::map<std::string,std::int64_t> g_imported_at;
std::map<std::string,std::uintmax_t> g_managed_sizes;
struct TrackPackOwnerRecord{std::string track_id,digest,fingerprint;};
std::map<std::string,TrackPackOwnerRecord> g_track_pack_owners;
struct CachedPackInfo{std::string fingerprint;PackInfo info;};
std::map<std::string,CachedPackInfo> g_pack_index;
struct PendingDeletion{std::string id;fs::path path;std::string name;};
std::vector<PendingDeletion> g_pending_deletions;
std::uint64_t g_generation=1;
void SavePackIndexLocked(){}
'''
for name in ('Lower','StableId','DisplayName','FileTimeText','FileSignature','PackFingerprint','ManagedSize','ManagedTimestamp','CurrentTimestamp','LoadSettingsLocked','SaveSettingsLocked','IsDirectManagedChild','RemoveManagedPath','PlaceholderInfo'):
 code+=function(name)
code+='''
// Format decoding is an inspection fixture; scan, persistence, preference and
// managed-deletion implementations below are the actual authored source.
PackInfo InspectArchive(const fs::path& p){auto info=PlaceholderInfo(p,false);info.compatible=true;info.format=Format::NativeRt64;return info;}
PackInfo InspectDirectory(const fs::path& p){return InspectArchive(p);}
#define __PROSPERO__ 1
'''
scan=function('ScanLibrary').replace('dkrr::ps5_textures::entries(directory, error)','dkrr::ps5_textures::entries(directory, error, test_root)')
code+=scan
code+='namespace dkr::runtime::texture_packs {\n'+function('set_enabled')+function('set_hidden')+function('delete_managed')+'}\n'
code+=r'''
std::string read(const fs::path&p){std::ifstream file(p);return {std::istreambuf_iterator<char>(file),{}};}
void write(const fs::path&p,const std::string&v){fs::create_directories(p.parent_path());std::ofstream file(p);file<<v;assert(file.good());}
int main(int argc,char**argv){
 assert(argc==2);test_root=argv[1];auto us10=test_root/"profile-us-v77",us11=test_root/"profile-us-v80";
 auto config10=dkrr::ps5_profiles::settings(us10,test_root),config11=dkrr::ps5_profiles::settings(us11,test_root);assert(config10==config11);
 write(us10/"texture-packs/common.zip","active original");write(us11/"texture-packs/common.zip","other duplicate");
 write(us11/"texture-packs/legacy.rtz","existing legacy");write(us11/"texture-packs.ini","prior settings retained");
 const auto oldstamp=fs::last_write_time(us11/"texture-packs.ini");
 g_pack_directory=config10/"texture-packs";g_settings_path=config10/"texture-packs.ini";
 ScanLibrary(true,false);assert(g_packs.size()==2);for(const auto& p:g_packs)assert(!p.enabled);
 ScanLibrary(false,false);assert(g_packs.size()==2);for(const auto& p:g_packs)assert(p.compatible&&!p.enabled);
 assert(g_packs[0].path==us10/"texture-packs/common.zip");
 set_enabled("legacy.rtz",true);assert(g_enabled_ids.contains("legacy.rtz"));
 assert(read(g_settings_path).find("enabled=legacy.rtz")!=std::string::npos);
 g_enabled_ids.clear();g_packs.clear();g_settings_path=config11/"texture-packs.ini";LoadSettingsLocked();ScanLibrary(true,false);
 assert(g_packs.size()==2);assert(g_packs[1].id=="legacy.rtz"&&g_packs[1].enabled);
 std::string status;assert(!delete_managed("legacy.rtz",status));assert(fs::exists(us11/"texture-packs/legacy.rtz"));
 assert(set_hidden("legacy.rtz",true,status));assert(!g_enabled_ids.contains("legacy.rtz"));
 LoadSettingsLocked();ScanLibrary(true,false);assert(g_packs[1].hidden&&!g_packs[1].enabled);
 assert(read(us11/"texture-packs.ini")=="prior settings retained"&&fs::last_write_time(us11/"texture-packs.ini")==oldstamp);
 assert(read(us10/"texture-packs/common.zip")=="active original"&&read(us11/"texture-packs/common.zip")=="other duplicate");
 assert(!fs::exists(us10/"texture-packs/legacy.rtz"));
 puts("PASS actual texture runtime scan, common authored settings round-trip, cached enabled state, local precedence, shared hide and deletion refusal; archive decoding is a fixture");
}
'''
with tempfile.TemporaryDirectory(prefix='dkrr-shared-texture-runtime-') as tmp:
 work=pathlib.Path(tmp);(work/'test.cpp').write_text(code);out=work/'test'
 subprocess.run(['clang++','-std=c++20','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+str(game),'-I'+str(r/'src'),str(work/'test.cpp'),'-o',str(out)],check=True)
 subprocess.run([str(out),str(work/'own')],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
