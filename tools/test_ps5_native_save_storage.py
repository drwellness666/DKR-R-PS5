#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise the patched production save transactions; model only path root and metadata."""
import argparse,os,pathlib,subprocess,tempfile,tarfile
root=pathlib.Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('game_work',type=pathlib.Path,nargs='?');p.add_argument('--cache',action='append',type=pathlib.Path,default=[]);a=p.parse_args()
with tempfile.TemporaryDirectory(prefix='dkrr-native-save-') as tmp:
    t=pathlib.Path(tmp)
    if a.game_work is None:
        a.game_work=t/'inputs'
        command=['python3',str(root/'tools/prepare_controller_test_sources.py'),str(a.game_work)]
        for cache in a.cache:command+=['--cache',str(cache)]
        subprocess.run(command,check=True)
    text=(a.game_work/'source/DKR-R/runtime-recomp/src/game/save_storage.hpp').read_text()
    # Explicit test seams: host files live in tmp; native nlink=0/error and I/O
    # failures are modeled. All admission/lease/validation/commit code is real.
    for old,new in [('std::filesystem::hard_link_count(target, ec)','fixture_link_count(target, ec)'),('dkrr::ps5_saves::transaction_path(target)','fixture_transaction_path(target)')]:
        if text.count(old)!=1:raise SystemExit('Save test seam changed: '+old)
        text=text.replace(old,new)
    (t/'save_storage.hpp').write_text(text);out=t/'storage'
    backend=(a.game_work/'source/DKR-R/runtime-recomp/src/game/shared_save_backend.inl').read_text()
    def function(signature):
        start=backend.index(signature);begin=backend.index('{',start);end=begin+1;depth=1
        while depth:
            depth+=(backend[end]=='{')-(backend[end]=='}');end+=1
        return backend[start:end]+'\n'
    (t/'shared_prepare.inc').write_text(''.join(function(s) for s in (
        'bool read_json(', 'bool write_json(', 'bool prepare(')))
    with tarfile.open(root/'vendor/N64ModernRuntime.tar.gz') as archive:
        (t/'json.hpp').write_bytes(archive.extractfile('thirdparty/json/json.hpp').read())
    subprocess.run(['clang++','-D__PROSPERO__','-std=c++20','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Wl,--wrap=fsync','-I'+str(root/'src'),'-I'+str(t),str(root/'tests/ps5_native_save_storage.cpp'),'-o',str(out)],check=True)
    subprocess.run([str(out),str(t/'files')],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
