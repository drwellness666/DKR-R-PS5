#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,os,subprocess,tempfile
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('game',type=Path);a=p.parse_args();r=Path(__file__).resolve().parents[1];g=a.game.resolve();domain=g/'game-ps5/native-domain/src/game'
with tempfile.TemporaryDirectory(prefix='dkrr-native-netplay-') as temporary:
 t=Path(temporary);objects=[];flags=['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-I'+str(r/'src')]
 for source in ('src/ps5_net.c','tests/ps5_scenet_host.c'):
  dest=t/(Path(source).stem+'.o');subprocess.run(['clang','-std=c11','-D_GNU_SOURCE','-Wno-macro-redefined',*flags,'-c',str(r/source),'-o',str(dest)],check=True);objects.append(str(dest))
 sources=[domain/'netplay'/name for name in ('datagram_socket.cpp','lan_discovery.cpp','network_interfaces.cpp')]
 out=t/'test'
 subprocess.run(['clang++','-std=c++20','-DDKRR_TEST_NATIVE_NET',*flags,'-I'+str(domain),str(r/'tests/ps5_netplay_sockets.cpp'),*[str(p) for p in sources],*objects,'-pthread','-o',str(out)],check=True)
 subprocess.run([str(out)],check=True,timeout=20,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=1',UBSAN_OPTIONS='halt_on_error=1'))
