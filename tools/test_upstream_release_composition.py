#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Check actual composed-release provenance and reject malformed source layers."""
from pathlib import Path
import argparse
import difflib
import hashlib
import json
import tempfile
import subprocess
import os
from compose_upstream_release import compose
root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('game_work', type=Path)
args = parser.parse_args()
game = args.game_work / 'source/DKR-R'
record = json.loads((game/'ps5-release-layers.json').read_text())
pin = json.loads((root/'GAME_SOURCES.json').read_text())['release_layers']['sha256']
assert record['manifest_sha256'] == pin and record['records_verified'] == 2549
assert len(record['files']) == 3213
assert record['upstream_version'] == json.loads((root/'GAME_SOURCES.json').read_text())['upstream_version']
assert record['capsule_version'] == '1.0.7'
hotfix = json.loads((root/'GAME_SOURCES.json').read_text())['startup_memory_hotfix']
assert hashlib.sha256((game/hotfix['path']).read_bytes()).hexdigest() == hotfix['sha256']
assert 'patches/startup-memory/RuntimeMemory.cmake' in (game/'runtime-recomp/cmake/PerformancePatches.cmake').read_text()
patches = json.loads((root/'patches/dkrr-patch-manifest.json').read_text())['files']
for name, row in record['files'].items():
    expected = patches[name]['patched_sha256'] if name in patches else row['sha256']
    assert hashlib.sha256((game/name).read_bytes()).hexdigest() == expected, name
sha = lambda raw: hashlib.sha256(raw).hexdigest()
with tempfile.TemporaryDirectory(prefix='dkrr-release-layers-') as folder:
    base = Path(folder)
    data = base/'patches/release-prep/v1.0.7/source-capsule-r2'
    data.mkdir(parents=True)
    original = b'alpha\nbeta\n'
    changed = b'alpha\ngamma\n'
    source = base/'runtime-recomp/src/game/sample.cpp'
    source.parent.mkdir(parents=True)
    source.write_bytes(original)
    patch = ''.join(difflib.unified_diff(original.decode().splitlines(True), changed.decode().splitlines(True), fromfile='before', tofile='after')).encode()
    (data/'change.patch').write_bytes(patch)
    row = dict(base='runtime-recomp/src/game/sample.cpp', base_sha256=sha(original), patch='change.patch', sha256=sha(changed))
    document = dict(schema=1, version='1.0.7', roles={'CAMERA':{'src/game/sample.cpp':row}})
    def save():
        raw=json.dumps(document).encode();(data/'manifest.json').write_bytes(raw);return sha(raw)
    def rejects(action):
        try:action()
        except (ValueError,FileNotFoundError):return
        raise AssertionError('Invalid source composition accepted')
    expected=save()
    rejects(lambda:compose(base,'0'*64))
    row['base_sha256']='0'*64;rejects(lambda:compose(base,save()))
    row['base_sha256']=sha(original);row['sha256']='0'*64;rejects(lambda:compose(base,save()))
    assert source.read_bytes()==original
    row['sha256']=sha(changed);row['patch']='../escape';rejects(lambda:compose(base,save()))
    row['patch']='change.patch'
    source.unlink();outside=base/'retained.cpp';outside.write_bytes(original);source.symlink_to(outside)
    rejects(lambda:compose(base,save()));assert outside.read_bytes()==original
    source.unlink();source.write_bytes(original)
    result=compose(base,save());assert source.read_bytes()==changed and result['records_verified']==1
# Exercise the actual native regular-file admission branch. Only filesystem
# metadata calls are modeled; paths, link policy and read-only scope stay real.
core=(game/'runtime-recomp/src/game/updates/update_core.cpp').read_text()
a=core.index('void regular(');b=core.index('\nbool path_exists(',a)
body=core[a:b]
assert body.count('std::filesystem::symlink_status(p, ec)')==1
assert body.count('std::filesystem::hard_link_count(p, ec)')==1
body=body.replace('std::filesystem::symlink_status(p, ec)','fixture_status(p, ec)').replace('std::filesystem::hard_link_count(p, ec)','fixture_links(p, ec)')
prelude=r'''#include <cassert>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <system_error>
static unsigned links=0;static bool symbolic=false,metadata_error=false;
std::filesystem::file_status fixture_status(const std::filesystem::path&,std::error_code& ec){ec.clear();return std::filesystem::file_status(symbolic?std::filesystem::file_type::symlink:std::filesystem::file_type::regular);}
unsigned fixture_links(const std::filesystem::path&,std::error_code& ec){ec=metadata_error?std::make_error_code(std::errc::io_error):std::error_code{};return links;}
[[noreturn]]void fail(const std::string& error){throw std::runtime_error(error);}
'''
checks=r'''
void rejected(const char* path,bool reading){bool refused=false;try{regular(path,reading);}catch(const std::exception&){refused=true;}assert(refused);}
int main(){
 regular("/data/dkrr/profile-us-v77/mod-store/catalogue.json");
 rejected("/data/foreign/file",false);regular("/data/foreign/file",true);
 rejected("/data/dkrr/profile-us-v77/mod-store/../saves/file",false);
 rejected("/data/dkrr/profile-us-v80/mod-store/file",false);
 for(unsigned count:{1U,2U}){links=count;if(count==1){regular("/data/foreign/file",false);regular("/data/foreign/file",true);}else{rejected("/data/foreign/file",false);rejected("/data/foreign/file",true);}}
 links=0;symbolic=true;rejected("/data/dkrr/profile-us-v77/mod-store/file",true);
 symbolic=false;metadata_error=true;rejected("/data/dkrr/profile-us-v77/mod-store/file",true);
}
'''
with tempfile.TemporaryDirectory(prefix='dkrr-store-metadata-') as folder:
    test=Path(folder)/'test.cpp';test.write_text(prelude+body+checks);binary=Path(folder)/'test'
    subprocess.run(['clang++','-D__PROSPERO__','-std=c++20','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(test),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
print('PASS exact 1.0.9 provenance with frozen 1.0.7 layers and pinned startup-memory hotfix, wrong pins/base/result, path traversal and source-link refusal; failed validation leaves source unchanged')
