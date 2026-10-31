#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise the real manual/Store metadata commits with blocked pack-size I/O."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('native_domain', type=Path)
args = parser.parse_args()
domain = args.native_domain.resolve()
manual = (domain/'src/game/runtime_texture_packs.cpp').read_text()
store = (domain/'src/game/store_texture_adapter.inl').read_text()
start = manual.index('        const auto imported_size = ManagedSize(destination);')
manual = manual[start:manual.index('        record_track_pack(destination);', start)]
start = store.index('    const auto id = StableId(path);')
store = store[start:store.index('    refresh();', start)]
assert 'g_managed_sizes[id] = imported_size;' in store
assert 'ManagedSize(path)' not in store[store.index('std::lock_guard'):]

fixture = r'''
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <map>
#include <mutex>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#include <json/json.hpp>
using Json=nlohmann::json;
std::mutex g_mutex;
std::map<std::string,std::int64_t> g_imported_at;
std::map<std::string,std::uintmax_t> g_managed_sizes;
struct Owner {std::string track_id,digest,fingerprint;};
std::map<std::string,Owner> g_track_pack_owners;
std::set<std::string> g_enabled_ids,g_hidden_ids;
std::map<std::string,std::size_t> g_priorities;
std::atomic<bool> entered=false,release_io=false;
unsigned saved=0;
std::string StableId(const std::filesystem::path& path){return path.filename().string();}
std::string text(const Json& value,const char*key,unsigned){return value.value(key,std::string{});}
std::int64_t CurrentTimestamp(){return 1234;}
std::uintmax_t ManagedSize(const std::filesystem::path&){
 entered.store(true);
 while(!release_io.load())std::this_thread::yield();
 return 987654;
}
bool SaveSettingsLocked(){++saved;return true;}
void manual_commit(){const std::filesystem::path destination="pack";
@MANUAL@
}
void store_commit(){const std::filesystem::path path="pack";
 const Json output={{"owner","track"},{"textureDigest","digest"},{"fingerprint","fingerprint"}};
 const Json previous={{"enabled",true},{"hidden",false},{"priority",std::size_t(3)}};
@STORE@
}
void run(void(*commit)(),bool store){
 entered=false;release_io=false;saved=0;g_imported_at.clear();g_managed_sizes.clear();
 g_track_pack_owners.clear();g_enabled_ids.clear();g_hidden_ids.clear();g_priorities.clear();
 std::thread worker(commit);
 const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(5);
 while(!entered.load()&&std::chrono::steady_clock::now()<until)std::this_thread::yield();
 assert(entered.load());
 // The real commit is waiting on filesystem work. UI snapshot must remain
 // available, with no partially committed metadata visible.
 assert(g_mutex.try_lock());
 assert(g_imported_at.empty()&&g_managed_sizes.empty()&&saved==0);
 g_mutex.unlock();release_io=true;worker.join();
 assert(g_imported_at.at("pack")==1234&&g_managed_sizes.at("pack")==987654&&saved==1);
 if(store){assert(g_enabled_ids.contains("pack")&&!g_hidden_ids.contains("pack"));
  assert(g_priorities.at("pack")==3&&g_track_pack_owners.at("pack").track_id=="track");}
}
int main(){run(manual_commit,false);run(store_commit,true);}
'''.replace('@MANUAL@', manual).replace('@STORE@', store)
root = Path(__file__).resolve().parents[1]
json_include = root/'work/upstream-109-20261024/game/source/DKR-R/extern/n64-modern-runtime/thirdparty'
assert (json_include/'json/json.hpp').is_file()
with tempfile.TemporaryDirectory(prefix='dkrr-texture-lock-') as folder:
    temp = Path(folder)
    source = temp/'commit.cpp'
    source.write_text(fixture)
    binary = temp/'test'
    subprocess.run(['clang++','-std=c++20','-O1','-g','-pthread',
                    '-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
                    '-I'+str(json_include),str(source),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,timeout=15,
                   env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
print('PASS actual manual and Store metadata commits: blocked recursive I/O leaves UI snapshot mutex available; complete metadata, owner, enabled/hidden and priority preserved')
