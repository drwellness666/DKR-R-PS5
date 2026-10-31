#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise the actual staged RTC implementations with native-like restrictions."""
import argparse, os, shutil, subprocess, tempfile
from pathlib import Path
from stage_ps5_network import stage
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('source', type=Path)
p.add_argument('--cmake', default='cmake')
a = p.parse_args()
r = Path(__file__).resolve().parents[1]
source = a.source.resolve()
with tempfile.TemporaryDirectory(prefix='dkrr-rtc-wakeup-') as temporary:
    t = Path(temporary)
    stage(source, t/'staged')
    juice = t/'juice'
    shutil.copytree(source/'deps/libjuice', juice, ignore=shutil.ignore_patterns('.git'))
    shutil.copyfile(t/'staged/deps/libjuice/src/conn_poll.c', juice/'src/conn_poll.c')
    flags = ['-O1', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-I'+str(r/'src')]
    subprocess.run([a.cmake, '-S', str(juice), '-B', str(t/'build'), '-DCMAKE_C_COMPILER=clang',
                    '-DBUILD_SHARED_LIBS=OFF', '-DNO_TESTS=ON', '-DCMAKE_BUILD_TYPE=Debug',
                    '-DCMAKE_C_FLAGS='+' '.join(flags+['-DDKRR_TEST_NATIVE_NET','-include',str(r/'src/ps5_net_alias.h')])], check=True, stdout=subprocess.DEVNULL)
    subprocess.run([a.cmake, '--build', str(t/'build'), '--target', 'juice-static', '-j3'], check=True, stdout=subprocess.DEVNULL)
    objects=[]
    for src in ('src/ps5_net.c','tests/ps5_scenet_host.c'):
        obj=t/(Path(src).stem+'.o')
        subprocess.run(['clang','-std=c11','-D_GNU_SOURCE','-Wno-macro-redefined',*flags,'-c',str(r/src),'-o',str(obj)],check=True)
        objects.append(str(obj))
    binary=t/'test'
    includes=[source/'include',source/'include/rtc',source/'src/impl',source/'deps/plog/include',juice/'include']
    common=['clang++','-std=c++17','-DRTC_STATIC','-DRTC_ENABLE_MEDIA=0',*flags,*['-I'+str(x) for x in includes]]
    wake=t/'wakeup.o'
    subprocess.run([*common,'-DDKRR_TEST_NATIVE_NET','-include',str(r/'src/ps5_net_alias.h'),'-c',str(t/'staged/src/impl/pollinterrupter.cpp'),'-o',str(wake)],check=True)
    subprocess.run([*common,str(r/'tests/ps5_rtc_nonblocking.cpp'),str(wake),*objects,str(t/'build/libjuice-static.a'),'-pthread','-o',str(binary)],check=True)
    subprocess.run([str(binary)], env=dict(os.environ, ASAN_OPTIONS='detect_leaks=1', UBSAN_OPTIONS='halt_on_error=1'), check=True, timeout=30)
