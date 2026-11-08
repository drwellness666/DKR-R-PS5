#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Package digest with a mismatched Mbed TLS context ABI (regression for the 1.0 updater)."""
import argparse,hashlib,os,pathlib,subprocess,tempfile
from test_ps5_internet_update import dependencies
ROOT=pathlib.Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('game_work',nargs='?',type=pathlib.Path);p.add_argument('--cache',action='append',type=pathlib.Path,default=[]);a=p.parse_args()
with tempfile.TemporaryDirectory(prefix='dkrr-digest-abi-') as d:
    t=pathlib.Path(d);deps=dependencies(a.game_work,a.cache,t)
    (t/'legacy_mod_hash_config.h').write_text('#define MBEDTLS_SHA256_C\n')
    common=['-O1','-g','-fsanitize=address,undefined','-I'+str(t),'-I'+str(deps/'mbedtls/include')]
    objects=[]
    # Provider: default configuration, SHA-224 enabled (larger context, as linked on PS5).
    for src in (deps/'mbedtls/library/sha256.c',deps/'mbedtls/library/platform_util.c'):
        obj=t/(src.name+'.o');subprocess.run(['clang','-std=c11',*common,'-c',str(src),'-o',str(obj)],check=True);objects.append(str(obj))
    # Consumer: the small legacy configuration, exactly as 1.0 compiled it.
    for src in (deps/'monocypher/src/monocypher.c',deps/'monocypher/src/optional/monocypher-ed25519.c'):
        obj=t/(src.name+'.o');subprocess.run(['clang','-std=c11',*common,'-I'+str(deps/'monocypher/src'),'-c',str(src),'-o',str(obj)],check=True);objects.append(str(obj))
    subprocess.run(['clang++','-std=c++20','-Wall','-Wextra','-Werror',*common,'-DMBEDTLS_CONFIG_FILE="legacy_mod_hash_config.h"','-I'+str(ROOT/'src'),'-I'+str(deps/'monocypher/src'),'-I'+str(deps/'monocypher/src/optional'),
        str(ROOT/'src/ps5_update_policy.cpp'),str(ROOT/'tests/ps5_update_digest_abi.cpp'),*objects,'-o',str(t/'abi')],check=True)
    env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1')
    for size in (20,65536,65537,3*65536+100):
        data=bytearray(b'DKRRPK2\n'+bytes((i*131+7)%251+1 for i in range(size-8)))  # no zero bytes anywhere
        f=t/'package.bin';f.write_bytes(data)
        subprocess.run([str(t/'abi'),str(f),hashlib.sha256(data).hexdigest()],env=env,check=True,stdout=subprocess.DEVNULL)
print('PASS update digest: correct SHA-256 with a larger linked Mbed TLS context; no SHA-224 truncation from stale buffer bytes')
