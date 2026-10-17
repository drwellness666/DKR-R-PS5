#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise the exact native update parser, crypto and worker with host service doubles."""
import argparse,hashlib,json,os,pathlib,re,struct,subprocess,tarfile,tempfile,urllib.request
ROOT=pathlib.Path(__file__).resolve().parents[1]

def dependencies(work,cache,temporary):
    if work is not None:return work/'source/DKR-R/extern'
    result=temporary/'deps';result.mkdir()
    metadata=json.loads((ROOT/'GAME_SOURCES.json').read_text())
    for name in ('monocypher','mbedtls'):
        row=next(x for x in metadata['sources'] if x.get('name')==name);archive=None
        for folder in cache:
            p=folder/row['file']
            if p.is_file() and hashlib.sha256(p.read_bytes()).hexdigest()==row['sha256']:archive=p;break
        if archive is None:
            archive=temporary/row['file']
            with urllib.request.urlopen(row['url'],timeout=30) as source:archive.write_bytes(source.read())
            if hashlib.sha256(archive.read_bytes()).hexdigest()!=row['sha256']:raise ValueError('Dependency hash mismatch')
        target=result/name;target.mkdir()
        with tarfile.open(archive) as source:
            for member in source:
                parts=member.name.split('/',1)
                if len(parts)==2 and parts[1]:member.name=parts[1];source.extract(member,target,filter='data')
    return result

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('game_work',nargs='?',type=pathlib.Path);p.add_argument('--cache',action='append',type=pathlib.Path,default=[]);p.add_argument('--release-payload',type=pathlib.Path);a=p.parse_args()
    with tempfile.TemporaryDirectory(prefix='dkrr-network-update-') as d:
        t=pathlib.Path(d);deps=dependencies(a.game_work,a.cache,t);key=t/'test-key.pem'
        with key.open('wb') as output:subprocess.run(['openssl','genpkey','-algorithm','ED25519'],stdout=output,check=True)
        public=subprocess.check_output(['openssl','pkey','-in',str(key),'-pubout','-outform','DER'])[12:];assert len(public)==32;(t/'public.bin').write_bytes(public)
        (t/'fixture_key.h').write_text('static const unsigned char dkrr_update_public_key[32]={'+','.join(str(n) for n in public)+'};\n')
        data=bytearray(range(256));data[:7]=b'\x7fELF\x02\x01\x01';struct.pack_into('<H',data,18,62);payload=t/'payload.elf';payload.write_bytes(data)
        def signed(sequence):
            body=f'DKRR-UPDATE-1\nPPSA94678\n{sequence}\nps5-beta.3.4\n256\n{hashlib.sha256(data).hexdigest()}\nDKRR-Network-Update.elf\n'.encode();message=t/'body';message.write_bytes(body)
            sig=subprocess.check_output(['openssl','pkeyutl','-sign','-rawin','-inkey',str(key),'-in',str(message)])
            return body+sig.hex().encode()+b'\n'
        manifest=t/'channel.txt';manifest.write_bytes(signed(7));(t/'current.txt').write_bytes(signed(6))
        flags=['-O1','-g','-fsanitize=address,undefined','-DMBEDTLS_CONFIG_FILE="legacy_mod_hash_config.h"','-I'+str(ROOT/'src'),'-I'+str(t),'-I'+str(deps/'monocypher/src'),'-I'+str(deps/'monocypher/src/optional'),'-I'+str(deps/'mbedtls/include')]
        (t/'legacy_mod_hash_config.h').write_text('#define MBEDTLS_SHA256_C\n')
        objects=[]
        for src in (deps/'monocypher/src/monocypher.c',deps/'monocypher/src/optional/monocypher-ed25519.c',deps/'mbedtls/library/sha256.c',deps/'mbedtls/library/platform_util.c'):
            obj=t/(src.name+'.o');subprocess.run(['clang','-std=c11',*flags,'-c',str(src),'-o',str(obj)],check=True);objects.append(str(obj))
        cpp=['clang++','-std=c++20',*flags,'-Wall','-Wextra','-Werror']
        subprocess.run(cpp+[str(ROOT/'src/ps5_update_policy.cpp'),str(ROOT/'tests/ps5_update_policy.cpp'),*objects,'-o',str(t/'policy')],check=True)
        env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1')
        subprocess.run([str(t/'policy'),str(manifest),str(t/'public.bin'),str(payload)],env=env,check=True)
        release_channel=ROOT/'updates/channel.txt'
        if release_channel.exists():
            trusted=bytes(int(n,16) for n in re.findall(r'0x([0-9a-f]{2})',(ROOT/'src/ps5_update_channel.h').read_text()));assert len(trusted)==32
            (t/'release-public.bin').write_bytes(trusted)
            subprocess.run([str(t/'policy'),str(release_channel),str(t/'release-public.bin'),str(a.release_payload or '-'),'--release' if a.release_payload else '--channel'],env=env,check=True)
        payload.write_bytes(data)
        data_root=t/'data/dkrr';(data_root/'preferences').mkdir(parents=True)
        subprocess.run(cpp+['-pthread','-DDKRR_RELEASE_SEQUENCE=6','-DDKRR_UPDATE_KEY_HEADER="fixture_key.h"','-DDKRR_UPDATE_ROOT="'+str(data_root)+'"',str(ROOT/'src/ps5_update_policy.cpp'),str(ROOT/'src/ps5_internet_update.cpp'),str(ROOT/'tests/ps5_update_worker.cpp'),*objects,'-o',str(t/'worker')],check=True)
        for mode in ('good','offline','current','signature','digest','oversize','receiver','cancel','redirect','bad-redirect','short-send','no-ack','auto'):
            subprocess.run([str(t/'worker'),mode,str(manifest),str(t/'current.txt'),str(payload)],env=env,check=True,timeout=20)
        assert not list(data_root.rglob('*.elf'))
        print('PASS actual native update worker: 13 service scenarios, shared opt-in auto-check, no retained download, no execution before verification')
if __name__=='__main__':main()
