#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build the pinned resolver without a background event thread."""
import argparse, hashlib, json, pathlib, subprocess
p=argparse.ArgumentParser();p.add_argument('game',type=pathlib.Path);p.add_argument('sdk',type=pathlib.Path);p.add_argument('source',type=pathlib.Path);p.add_argument('--cmake',default='cmake');a=p.parse_args()
game,sdk,source=(x.resolve() for x in (a.game,a.sdk,a.source));stage=game/'native-dns';stage.mkdir(exist_ok=True)
link=stage/'source'
if link.is_symlink():
    if link.resolve()!=source:raise SystemExit('Unexpected DNS source link')
elif link.exists():raise SystemExit('Unexpected DNS source directory')
else:link.symlink_to(source,target_is_directory=True)
if '1.34.9' not in (source/'include/ares_version.h').read_text():raise SystemExit('Unexpected c-ares version')
args=[a.cmake,'-S',str(source),'-B',str(stage/'build'),'-G','Unix Makefiles','-DCMAKE_TOOLCHAIN_FILE='+str(sdk/'toolchain/prospero.cmake'),'-DCMAKE_BUILD_TYPE=Release','-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY','-DCARES_STATIC=ON','-DCARES_SHARED=OFF','-DCARES_INSTALL=OFF','-DCARES_BUILD_TOOLS=OFF','-DCARES_BUILD_TESTS=OFF','-DCARES_THREADS=OFF','-DHAVE_PIPE2=0','-DHAVE_FCNTL_O_NONBLOCK=0','-DHAVE_IOCTL_FIONBIO=1','-DHAVE_IF_NAMETOINDEX=0','-DHAVE_IF_INDEXTONAME=0','-DHAVE_GETRANDOM=0','-DHAVE_ARC4RANDOM_BUF=1']
args.append('-DHAVE_MEMMEM=0') # Header declaration exists; no native SDK export.
# c-ares recomputes HAVE_FCNTL_O_NONBLOCK from HAVE_O_NONBLOCK during
# configuration. Disable that prerequisite too, then check the actual output.
args.append('-DHAVE_O_NONBLOCK=0')
# The renderer's arc4random_buf is a clock-seeded graphics PRNG. DNS query
# identifiers/cookies use the same checked OS entropy source as TLS and ICE.
args.extend(['-DHAVE_GETSERVBYNAME_R=0','-DHAVE_GETSERVBYPORT_R=0'])
bridge=pathlib.Path(__file__).resolve().parents[1]/'src/ps5_net_alias.h'
args.extend(['-DCMAKE_PROJECT_INCLUDE='+str(bridge.parent.parent/'tools/NativeSocketTarget.cmake'),'-DDKRR_NATIVE_NET_HEADER='+str(bridge)])
args.append('-DCMAKE_C_FLAGS=-Darc4random_buf=dkrr_dns_random -Dgetservbyname=dkrr_dns_service_by_name -Dgetservbyport=dkrr_dns_service_by_port')
subprocess.run(args,check=True)
config=(stage/'build/ares_config.h').read_text()
if '#define HAVE_FCNTL_O_NONBLOCK 1' in config or '#define HAVE_IOCTL_FIONBIO 1' not in config:
 raise SystemExit('c-ares did not select checked ioctl nonblocking I/O')
subprocess.run([a.cmake,'--build',str(stage/'build'),'-j3'],check=True)
archive=stage/'build/lib/libcares.a'
(stage/'DNS-RECEIPT.json').write_text(json.dumps(dict(version='1.34.9',source_sha256='9364b70a12eb59a06830fb488d9357f90100dff0d98b23ea3593a183d43e6f2f',archive_sha256=hashlib.sha256(archive.read_bytes()).hexdigest(),configuration='system DNS; caller-owned bounded polling; no event thread'),indent=2)+'\n')
