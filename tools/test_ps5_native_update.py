#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Real in-process transaction, signed packages and power cuts, without ROMs."""
import argparse,hashlib,json,os,pathlib,shutil,struct,subprocess,tempfile
from build_native_update_package import build
from build_universal_installer import PUBLIC_FILES,hash_source
from build_native_game_bundle_updater import inventory,inventory_hash
from test_ps5_internet_update import dependencies
ROOT=pathlib.Path(__file__).resolve().parents[1]

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('game_work',nargs='?',type=pathlib.Path);parser.add_argument('--cache',action='append',type=pathlib.Path,default=[]);a=parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='dkrr-native-update-') as folder:
        t=pathlib.Path(folder);deps=dependencies(a.game_work,a.cache,t);app=t/'data/homebrew/PPSA94678';root=t/'data/dkrr';prefs=root/'preferences';client=prefs/f'update-client-{os.getuid()}';lease=root/'app.lock'
        client.mkdir(parents=True,mode=0o700);key=t/'key.pem'
        with key.open('wb') as out:subprocess.run(['openssl','genpkey','-algorithm','ED25519'],stdout=out,check=True)
        (t/'public.bin').write_bytes(subprocess.check_output(['openssl','pkey','-in',str(key),'-pubout','-outform','DER'])[12:])
        def fixture(where,sequence,code):
            for name in PUBLIC_FILES:
                p=where/name;p.parent.mkdir(parents=True,exist_ok=True)
                p.write_bytes((b'\x4f\x15\x3d\x1d'+code) if name=='eboot.bin' else b'unchanged ABI' if name=='sce_module/libc.prx' else json.dumps({'titleId':'PPSA94678'}).encode() if name=='sce_sys/param.json' else code+name.encode())
                p.chmod(0o755 if name in ('eboot.bin','sce_module/libc.prx') else 0o644)
            for rev in (77,80):
                for cue in range(32):
                    p=where/f'assets/controller-audio/us-v{rev}/{cue}.pcm';p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(b'private synthetic PCM'+str((rev,cue)).encode())
            files=inventory(where);record=dict(files=files,app_manifest_sha256=inventory_hash(files),build_id='integrated-'+code.decode()*24)
            receipt=t/f'receipt-{sequence}.json';receipt.write_text(json.dumps(record))
            manifest='DKRR-INSTALL-2\n'+record['build_id']+f'\n{sequence}\n'+''.join(f'{m["sha1"]} {m["size"]} {m["mode"]:o} {name}\n' for name,m in sorted(files.items()))+'END\n'
            return receipt,manifest
        old=t/'old';old_receipt,old_manifest=fixture(old,7,b'a');(old/'dkrr-install.manifest').write_text(old_manifest)
        new=t/'next';new_receipt,new_manifest=fixture(new,8,b'b')
        package=t/'DKRR-Update.dkru';build(new,new_receipt,8,package)
        original_package=package.read_bytes()
        def sign():
            data=package.read_bytes();body=f'DKRR-UPDATE-2\nPPSA94678\n8\nps5-beta.3.5\n{len(data)}\n{hashlib.sha256(data).hexdigest()}\nDKRR-Update.dkru\n'.encode();(t/'body').write_bytes(body)
            signature=subprocess.check_output(['openssl','pkeyutl','-sign','-rawin','-inkey',str(key),'-in',str(t/'body')]);(t/'channel').write_bytes(body+signature.hex().encode()+b'\n')
        sign()
        flags=['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-DMBEDTLS_CONFIG_FILE="legacy_mod_hash_config.h"','-I'+str(ROOT/'src'),'-I'+str(t),'-I'+str(deps/'monocypher/src'),'-I'+str(deps/'monocypher/src/optional'),'-I'+str(deps/'mbedtls/include')]
        (t/'legacy_mod_hash_config.h').write_text('#define MBEDTLS_SHA256_C\n');(t/'hash.c').write_text(hash_source());objects=[]
        for source in (deps/'monocypher/src/monocypher.c',deps/'monocypher/src/optional/monocypher-ed25519.c',deps/'mbedtls/library/sha256.c',deps/'mbedtls/library/platform_util.c',ROOT/'installer/files.c',t/'hash.c'):
            obj=t/(source.name+'.o');subprocess.run(['clang','-std=c11','-D_GNU_SOURCE',*flags,'-c',str(source),'-o',str(obj)],check=True);objects.append(str(obj))
        binary=t/'native-test'
        subprocess.run(['clang++','-std=c++20',*flags,'-Wall','-Wextra','-Werror','-DDKRR_RELEASE_SEQUENCE=7','-DDKRR_NATIVE_TEST_HOOK','-DDKRR_UPDATE_ROOT="'+str(root)+'"','-DDKRR_NATIVE_APP="'+str(app)+'"','-Wl,--wrap=lstat','-Wl,--wrap=fstat','-Wl,--wrap=rename','-Wl,--wrap=write',str(ROOT/'src/ps5_update_policy.cpp'),str(ROOT/'src/ps5_native_update.cpp'),str(ROOT/'tests/ps5_native_update.cpp'),*objects,'-o',str(binary)],check=True)
        env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1');runs=[]
        def run(action,expected=0,cut=None,index=0,zero=False,**failure):
            extra={} if cut is None else dict(CUT_EVENT=cut,CUT_INDEX=str(index))
            if zero:extra['ZERO_LINKS']='1'
            result=subprocess.run([str(binary),action,str(t/'channel'),str(t/'public.bin'),str(package),str(lease),str(expected)],env=dict(env,**extra,**failure),capture_output=True,text=True,timeout=20)
            if result.returncode!=(77 if cut else 0):raise AssertionError(result.stdout+result.stderr)
            runs.append(dict(action=action,expected=expected,cut=cut,index=index,zero_links=zero));return result
        def reset():
            if app.exists():shutil.rmtree(app)
            transaction=client/'native-install'
            if transaction.exists():shutil.rmtree(transaction)
            shutil.copytree(old,app);package.write_bytes(original_package);sign()
            (prefs/'controls.json').write_bytes(b'KEEP controls')
            (root/'profile-us-v77/saves').mkdir(parents=True,exist_ok=True);(root/'profile-us-v77/saves/save.bin').write_bytes(b'KEEP progress')
            (root/'mods').mkdir(exist_ok=True);(root/'mods/textures.pack').write_bytes(b'KEEP textures')
        def unchanged():
            assert all((app/p.relative_to(old)).read_bytes()==p.read_bytes() for p in old.rglob('*') if p.is_file())
            assert (prefs/'controls.json').read_bytes()==b'KEEP controls' and (root/'profile-us-v77/saves/save.bin').read_bytes()==b'KEEP progress' and (root/'mods/textures.pack').read_bytes()==b'KEEP textures'
        # Complete update: directory inode and already mapped executable survive;
        # full next manifest includes retained private sounds, no embedded ROM.
        reset();inode=app.stat().st_ino;run('prepare');unchanged();run('apply');assert app.stat().st_ino==inode
        assert (app/'dkrr-install.manifest').read_text()==new_manifest
        assert all((app/p.relative_to(new)).read_bytes()==p.read_bytes() for p in new.rglob('*') if p.is_file())
        run('recover');assert not (client/'native-install').exists()
        # Every individual file replacement, including executable and manifest,
        # is interrupted in a real subprocess and restored on the next launch.
        for i in range(len(PUBLIC_FILES)+1):
            reset();run('prepare');run('apply',cut='replaced',index=i);run('recover',1);unchanged();run('recover')
        reset();run('prepare');run('apply',cut='committed');run('recover');assert (app/'dkrr-install.manifest').read_text()==new_manifest
        # Crash during recovery itself is recoverable without consuming backups.
        reset();run('prepare');run('apply',cut='replaced',index=8);run('recover',1,cut='restored',index=4);run('recover',1);unchanged()
        reset();run('prepare',cut='prepared');run('recover');unchanged()
        reset();run('prepare',zero=True);run('apply',zero=True);run('recover',zero=True)
        reset();run('prepare',-1,FAIL_WRITE='1');unchanged();assert not (client/'native-install').exists()
        for i in (0,8,len(PUBLIC_FILES)):
            reset();run('prepare');run('apply',-1,FAIL_RENAME=str(i));unchanged();run('recover')
        reset();(app/'assets/ui/Icons').chmod(0o555);run('prepare',-1);unchanged();(app/'assets/ui/Icons').chmod(0o755)
        # Corrupt bytes fail before staging, even with an otherwise valid offer.
        reset();data=bytearray(original_package);data[-1]^=1;package.write_bytes(data);run('prepare',-1);unchanged()
        # A validly signed but malformed file table is rejected, including path
        # traversal, duplicate/order changes, size overflow and runtime ABI changes.
        first=96
        for offset,value in ((first,b'../escape\0'),(first+160,struct.pack('<Q',2**63)),(first+168,struct.pack('<I',0o777))):
            reset();data=bytearray(original_package);data[offset:offset+len(value)]=value;package.write_bytes(data);sign();run('prepare',-1);unchanged();assert not (client/'native-install').exists()
        reset();data=original_package+b'unknown';package.write_bytes(data);sign();run('prepare',-1);unchanged()
        # Owner data is retained; damaged install, hardlinks and symlinked app
        # directories cannot become overwrite targets.
        reset();(app/'eboot.bin').write_bytes(b'changed');run('prepare',-1);assert (app/'eboot.bin').read_bytes()==b'changed'
        reset();outside=t/'outside';outside.mkdir();(outside/'sentinel').write_bytes(b'KEEP');shutil.rmtree(app/'sce_sys');(app/'sce_sys').symlink_to(outside,target_is_directory=True);run('prepare',-1);assert (outside/'sentinel').read_bytes()==b'KEEP'
        reset();os.link(app/'eboot.bin',t/'hardlink');run('prepare',-1);(t/'hardlink').unlink();unchanged()
        # A damaged backup or unknown recovery entry is preserved, not erased.
        reset();run('prepare');run('apply',cut='replaced',index=1);(client/'native-install/old/eboot.bin').write_bytes(b'bad backup');before=(app/'eboot.bin').read_bytes();run('recover',-1);assert (app/'eboot.bin').read_bytes()==before
        reset();run('prepare');(client/'native-install/unknown').write_bytes(b'KEEP');run('recover',-1);assert (client/'native-install/unknown').read_bytes()==b'KEEP'
        print(f'PASS in-process native updater: {len(runs)} subprocess operations; every commit interruption, repeated recovery, signed data parsing, zero-link metadata, mapped old executable, stable app directory, retained saves/settings/textures/PCM, no loader')

if __name__=='__main__':main()
