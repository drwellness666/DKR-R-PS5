#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise real Store tree copy/hash routines and count durability barriers."""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('game',type=Path)
a=p.parse_args();root=Path(__file__).resolve().parents[1];game=a.game.resolve()
new=game/'game-ps5/native-domain/src/game/mod_store_install.cpp'
old=game/'source/DKR-R/runtime-recomp/src/game/mod_store_install.cpp'

def function(text, signature):
    start=text.index(signature);begin=text.index('{',start)
    # Skip an empty default argument before the actual body.
    while text[begin:begin+2]=='{}':begin=text.index('{',begin+2)
    at=begin+1;depth=1
    while depth:
        depth+=(text[at]=='{')-(text[at]=='}');at+=1
    return text[start:at]+'\n'

fixture=r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <span>
#include <stop_token>
#include <stdexcept>
#include <string>
#include <vector>
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <openssl/sha.h>
#include <json/json.hpp>
namespace fs=std::filesystem;
using Json=nlohmann::json;
struct Failure:std::runtime_error{using std::runtime_error::runtime_error;};
constexpr std::uint64_t MaxTree=8ULL*1024*1024*1024;
fs::path store_io_path(const fs::path&p){return p;}
unsigned file_syncs=0,directory_syncs=0;
bool fail_file_sync=false;
extern "C" int __real_fsync(int);
extern "C" int __wrap_fsync(int fd){struct stat st{};assert(!fstat(fd,&st));
 if(S_ISDIR(st.st_mode))++directory_syncs;
 else {++file_syncs;if(fail_file_sync){errno=EIO;return -1;}}
 return __real_fsync(fd);
}
namespace dkr::updates {
std::map<fs::path,unsigned> hash_reads;
std::string hash(std::span<const std::uint8_t>bytes){unsigned char digest[32];assert(SHA256(bytes.data(),bytes.size(),digest));
 const char*digits="0123456789abcdef";std::string out;for(auto v:digest){out+=digits[v>>4];out+=digits[v&15];}return out;}
std::string hash_file(const fs::path&path,std::uint64_t limit){++hash_reads[path];std::ifstream in(path,std::ios::binary);
 std::vector<std::uint8_t>bytes((std::istreambuf_iterator<char>(in)),{});assert(in.eof()||in.good());assert(bytes.size()<=limit);return hash(bytes);}
}
using IoProgress=std::function<void(std::uint64_t,std::uint64_t)>;
struct CopyProgress{IoProgress report;std::uint64_t copied=0,total=0;};
@FUNCTIONS@
#if FIXED
// The actual put() runs with small journal/rename delegates. This checks
// publication order and digest reuse, not the separately tested recovery log.
bool managed_target(const std::string&s){return s.starts_with("texture-packs/");}
bool hash_string(const std::string&s){return s.size()==64&&s.find_first_not_of("0123456789abcdef")==s.npos;}
void discard(const fs::path&,const fs::path&p){fs::remove_all(p);}
void write(const fs::path&p,const Json&j){std::ofstream f(p);f<<j.dump();assert(f.good());}
namespace dkr::updates {void replace_file(const fs::path&a,const fs::path&b){fs::rename(a,b);}}
struct InstallManager {struct Impl;};
struct InstallManager::Impl {
 fs::path config,transaction;Json journal=Json::object();
 void protect(const std::string&name){auto p=child(config,name);if(path_exists(p))copy_plain(p,transaction/"backup");}
 IoProgress io_progress(float,float,const char*){return [](auto,auto){};}
 void put(const fs::path&,const std::string&,std::stop_token,const std::string&);
};
@PUT@
void publication_test(const fs::path&base,const fs::path&source){
 const auto proof=tree_hash(source);
 for(unsigned trial=0;trial<3;++trial){
  InstallManager::Impl installer;installer.config=base/std::to_string(trial);installer.transaction=installer.config/"transaction";
  fs::create_directories(installer.transaction);fs::create_directories(installer.config/"texture-packs");
  const auto target=installer.config/"texture-packs/test.rice";
  {std::ofstream f(target);f<<"original";}
  const auto current=tree_hash(target);
  dkr::updates::hash_reads.clear();
  if(trial==1){std::ofstream f(source/"1.png",std::ios::app);f<<"changed after verification";}
  if(trial==2)fail_file_sync=true;
  bool rejected=false;
  try{installer.put(source,"texture-packs/test.rice",{},proof);}catch(const Failure&){rejected=true;}
  fail_file_sync=false;
  if(trial==0){assert(!rejected);for(const auto&e:fs::recursive_directory_iterator(source))
    if(e.is_regular_file())assert(dkr::updates::hash_reads[e.path()]==0);
   assert(tree_hash(target)==proof&&tree_hash(installer.transaction/"backup")==current);}
  else {assert(rejected&&tree_hash(target)==current);}
 }
}
#endif
int main(int,char**argv){
 const fs::path base=argv[1],source=base/"source",target=base/"target";
 fs::create_directories(source/"nested/deeper");fs::create_directory(source/"empty");
 constexpr unsigned count=128;std::uint64_t total=0;
 for(unsigned i=0;i<count;++i){const auto folder=i%2?source:source/"nested/deeper";
  std::string bytes(100+i,'x');bytes[0]=char(i);std::ofstream f(folder/(std::to_string(i)+".png"),std::ios::binary);f.write(bytes.data(),bytes.size());total+=bytes.size();}
 const auto proof=tree_hash(source);file_syncs=directory_syncs=0;
#if FIXED
 std::uint64_t last=0;unsigned callbacks=0;
 CopyProgress copying{[&](auto done,auto size){assert(size==total&&done>=last&&done<=total);last=done;++callbacks;},0,total};
 copy_plain(source,target,{},true,true,&copying);
 assert(last==total&&callbacks>=count);
#else
 copy_plain(source,target);
#endif
 assert(file_syncs==count);const auto directories=directory_syncs;
 assert(tree_hash(target)==proof&&tree_size(target)==total&&fs::is_directory(target/"empty"));
#if FIXED
 std::uint64_t checked=0;unsigned checks=0;
 assert(tree_hash(target,{},[&](auto done,auto size){assert(size==total&&done>=checked&&done<=total);checked=done;++checks;})==proof);
 assert(checked==total&&checks>=count);
 assert(directories<=6);
#endif
 // A top-level individual file still needs its parent barrier.
 file_syncs=directory_syncs=0;copy_plain(source/"1.png",base/"single.png");
 assert(file_syncs==1&&directory_syncs==1);
 const auto existing=tree_hash(target);
 bool rejected=false;try{copy_plain(source,target);}catch(const Failure&){rejected=true;}
 assert(rejected&&tree_hash(target)==existing);
 fail_file_sync=true;rejected=false;
 try{copy_plain(source,base/"failed");}catch(const Failure&){rejected=true;}
 assert(rejected&&tree_hash(target)==existing);fail_file_sync=false;
 std::stop_source cancel;cancel.request_stop();rejected=false;
 try{copy_plain(source,base/"cancelled",cancel.get_token());}catch(const Failure&){rejected=true;}
 assert(rejected&&!fs::exists(base/"cancelled"));
 fs::create_symlink(source/"1.png",base/"redirect");rejected=false;
 try{copy_plain(base/"redirect",base/"redirect-copy");}catch(const Failure&){rejected=true;}
 assert(rejected&&!fs::exists(base/"redirect-copy"));
 {std::ofstream file(target/"1.png",std::ios::app);file<<"changed";}
 assert(tree_hash(target)!=proof);
#if FIXED
 publication_test(base/"publication",source);
#endif
 printf("RESULT files=%u directory_barriers=%u\n",count,directories);
}
'''
signatures=('void sync_directory(', 'void sync_file(', 'std::string path_text(',
            'bool path_exists(', 'void ordinary(', 'fs::path child(', 'void directories(',
            'std::uint64_t tree_size(', 'void copy_plain(', 'std::string tree_hash(')
with tempfile.TemporaryDirectory(prefix='dkrr-store-io-') as folder:
    t=Path(folder);results=[]
    include=game/'source/DKR-R/extern/n64-modern-runtime/thirdparty'
    for fixed,path in ((0,old),(1,new)):
        text=path.read_text();code='\n'.join(function(text,s) for s in signatures)
        put=function(text,'void InstallManager::Impl::put(') if fixed else ''
        source=t/f'fixture-{fixed}.cpp';source.write_text(fixture.replace('@FUNCTIONS@',code).replace('@PUT@',put))
        binary=t/f'test-{fixed}'
        subprocess.run(['clang++','-std=c++20',f'-DFIXED={fixed}','-O1','-g',
                        '-Wall','-Wextra','-Werror','-Wno-deprecated-declarations','-fsanitize=address,undefined',
                        '-I'+str(include),str(source),'-lcrypto','-Wl,--wrap=fsync','-o',str(binary)],check=True)
        output=subprocess.check_output([str(binary),str(t/f'files-{fixed}')],text=True,timeout=30,
                                      env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
        print(('original: ' if not fixed else 'corrected: ')+output.strip(),flush=True)
        results.append(int(re.search(r'directory_barriers=(\d+)',output)[1]))
    assert results[0]>128 and results[1]<=6
print('PASS actual Store I/O: every file still fsynced, grouped directory barriers, identical hashes and progress; actual put reuses verified digest without source rehash, detects changed copies before replacing old content, preserves backup; cancellation/sync/symlink/overwrite guards retained')
