#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Reproduce native second-start record refusal and exercise the real fix."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('game',type=Path)
args=parser.parse_args()
root=Path(__file__).resolve().parents[1]
domain=args.game.resolve()/'game-ps5/native-domain/src/game'
old=args.game.resolve()/'source/DKR-R/runtime-recomp/src/game/record_store.inl'

fixture=r'''
#include <cassert>
#include <filesystem>
#include <fstream>
#include <future>
#include <string>
#include "ps5_native_save_policy.hpp"
namespace fs=std::filesystem;
static fs::path fixture_root;
static bool hide_links=false;
static std::uintmax_t fixture_link_count(const fs::path&p,std::error_code&ec){
 if(hide_links){ec.clear();return 0;}return fs::hard_link_count(p,ec);
}
static bool fixture_transaction_path(const fs::path&p){
 auto relative=p.lexically_relative(fixture_root);
 if(relative.empty()||relative.native().starts_with(".."))return false;
 return dkrr::ps5_saves::transaction_path(fs::path("/data/dkrr")/relative);
}
#include "save_storage.hpp"
#include "custom_records.hpp"
namespace storage=dkr::runtime::saves::storage;
static bool ReadFileBounded(const fs::path&p,std::size_t max,storage::Bytes&out){
 std::error_code error;auto n=fs::file_size(p,error);
 return !error&&n<=max&&storage::read_exact(p,n,out);
}
#include "record_store.inl"
namespace records=dkr::runtime::saves::records;
int main(int,char**argv){
 fixture_root=argv[1];const auto path=fixture_root/"profile-us-v77/saves/custom-records-v1.bin";
 fs::create_directories(path.parent_path());std::string error;
 records::Record r;r.key=std::string(64,'a');r.vehicle=1;r.lap=123;r.course=456;
 records::Map initial;initial[{r.key,r.vehicle}]=r;
 assert(records::persist(path,records::encode(initial),error));
 storage::Bytes before;assert(ReadFileBounded(path,1024,before));
 hide_links=true;
 assert(!storage::writable_path(path,error)); // Generic Inspect still refuses unknown metadata.
 const bool admitted=records::begin(path,true,error);
 if(!FIXED){assert(!admitted&&error=="Save storage metadata does not permit this operation.");return 0;}
 assert(admitted);records::Map restored;assert(records::decode(records::seed(),restored));
 assert(restored.size()==1&&restored.begin()->second.course==456);
 assert(records::finish(error));
 assert(records::begin(path,true,error)); // Repeated native second start.
 storage::Bytes after;assert(ReadFileBounded(path,1024,after)&&after==before);
 r.course=400;records::Map next;next[{r.key,r.vehicle}]=r;
 records::submit(records::encode(next));assert(records::finish(error));
 assert(records::begin(path,true,error));assert(records::decode(records::seed(),restored));
 assert(restored.begin()->second.course==400);assert(records::finish(error));
 assert(records::begin(path,false,error));assert(records::decode(records::seed(),restored)&&restored.empty());
 assert(records::finish(error));
 const auto foreign=fixture_root/"foreign.bin";{std::ofstream f(foreign);f<<"keep";}
 assert(!records::begin(foreign,true,error)&&fs::file_size(foreign)==4);
 hide_links=false;const auto alias=fixture_root/"known-alias.bin";fs::create_hard_link(path,alias);
 assert(!records::begin(path,true,error));fs::remove(alias);
 hide_links=true;{std::ofstream f(path,std::ios::binary|std::ios::trunc);f<<"invalid-records";}
 assert(!records::begin(path,true,error));assert(error.starts_with("Custom-course records need recovery"));
 assert(fs::file_size(path)==15); // Invalid original retained.
}
'''
with tempfile.TemporaryDirectory(prefix='dkrr-record-start-') as folder:
    temp=Path(folder)
    storage=(domain/'save_storage.hpp').read_text()
    for before,after in [('std::filesystem::hard_link_count(target, ec)','fixture_link_count(target, ec)'),
                         ('dkrr::ps5_saves::transaction_path(target)','fixture_transaction_path(target)')]:
        assert storage.count(before)==1
        storage=storage.replace(before,after)
    (temp/'save_storage.hpp').write_text(storage)
    (temp/'custom_records.hpp').write_bytes((domain/'custom_records.hpp').read_bytes())
    (temp/'test.cpp').write_text(fixture)
    for fixed,path in ((0,old),(1,domain/'record_store.inl')):
        (temp/'record_store.inl').write_bytes(path.read_bytes())
        binary=temp/f'test-{fixed}'
        subprocess.run(['clang++','-std=c++20','-D__PROSPERO__',f'-DFIXED={fixed}',
                        '-O1','-g','-pthread','-Wall','-Wextra','-Werror',
                        '-fsanitize=address,undefined','-I'+str(root/'src'),'-I'+str(temp),
                        str(temp/'test.cpp'),'-o',str(binary)],check=True)
        subprocess.run([str(binary),str(temp/f'files-{fixed}')],check=True,timeout=20,
                       env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
        print('PASS', 'actual native fix: repeated record restore, async merge, no-restore, unknown metadata scoping, alias refusal and invalid-file preservation'
              if fixed else 'actual RC40 second-start failure reproduced with existing custom records and native nlink=0',flush=True)
main=(domain/'game_main.cpp').read_text()
start=main.index('std::fprintf(stderr,"[legacy][launch] %s\\n",error.what());')
end=main.index('if (selecting_save_owner)',start)
assert 'return 4;' not in main[start:end]
assert 'dkr::runtime::ui::run_startup_screen(' in main[end:end+2200]
print('PASS production native startup refusal reaches existing recovery menu instead of old preview exit')
