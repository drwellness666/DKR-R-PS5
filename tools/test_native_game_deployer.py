#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Run the combined updater against synthetic files with sanitizers and I/O faults."""
import hashlib,json,os,pathlib,shutil,subprocess,tarfile,tempfile
from unittest.mock import patch
from build_native_game_deployer import emit, plan, ROM_HASHES
from build_native_game_bundle_updater import CORE,inventory,inventory_hash
ROOT=pathlib.Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='dkrr-deployment-') as temp:
    work=pathlib.Path(temp);old=work/'old';game=work/'game';new=game/'private/native/PPSA94678'
    for app in (old,new):
        for name in CORE:
            p=app/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(b'unchanged-'+name.encode())
        (app/'eboot.bin').write_bytes(bytes.fromhex('4f153d1d')+b'old-code')
        (app/'sce_sys/param.json').write_text(json.dumps(dict(titleId='PPSA94678',attribute3=0x40040)))
    (old/'assets').mkdir();(old/'assets/retained.txt').write_text('preserve old asset')
    (new/'eboot.bin').write_bytes(bytes.fromhex('4f153d1d')+b'new-code')
    (new/'assets/controller-audio/us-v77').mkdir(parents=True)
    (new/'assets/controller-audio/us-v77/0.pcm').write_bytes(b'new synthetic sound')
    romdir=game/'private/roms';romdir.mkdir();(romdir/'selected-revisions.json').write_text('{"revisions":[77,80]}')
    rom=b'synthetic-owned-data'.ljust(12*1024*1024,b'0')
    for rev in (77,80):(romdir/f'dkr.us.v{rev}.z64').write_bytes(rom)
    manifest=inventory_hash(inventory(old,previous=True))
    with patch.dict(ROM_HASHES,{77:hashlib.sha1(rom).hexdigest(),80:hashlib.sha1(rom).hexdigest()}):
        entries=plan(old,game,manifest)
        assert entries[-1]['path'].endswith('/eboot.bin')
        try:plan(old,game,'0'*64);raise AssertionError('Unknown predecessor accepted')
        except ValueError:pass
        metadata=new/'sce_sys/param.json';before=metadata.read_bytes();metadata.write_text(json.dumps(dict(titleId='PPSA94678',attribute3=0)))
        try:plan(old,game,manifest);raise AssertionError('Output declaration changed')
        except ValueError:pass
        metadata.write_bytes(before)
    # Rewrite destination roots in the generated header; actual C remains intact.
    emit(entries,work)
    data=work/'data';data.mkdir();header=work/'native_deploy_files.h'
    header.write_text(header.read_text().replace('/data/',str(data)+'/'))
    with tarfile.open(ROOT/'vendor/ps5-sdk-source.tar.gz') as a:
        sha=a.extractfile('crt/nid.c').read().decode().replace('nid_encode(','dkrr_update_unused_nid_encode(')
    sha+='''\n#include <unistd.h>
#include <errno.h>
int dkrr_sha1_fd(int fd,unsigned char digest[20]) {
 SHA1_CTX c; unsigned char b[8192]; ssize_t n; SHA1Init(&c);
 for(;;){n=read(fd,b,sizeof(b));if(n<0 && errno==EINTR)continue;if(n<0)return -1;if(!n)break;SHA1Update(&c,b,(unsigned)n);}
 SHA1Final(digest,&c);return 0;
}
'''
    (work/'sha1.c').write_text(sha)
    (work/'fault.c').write_text('''#include <errno.h>
#include <stdlib.h>
#include <string.h>
int __real_rename(const char *,const char *);
int __wrap_rename(const char *a,const char *b) {
 if(getenv("DKRR_TEST_FAIL_COMMIT") && strstr(b,"eboot.bin")){errno=EIO;return -1;}
 return __real_rename(a,b);
}
''')
    flags=['clang','-std=c11','-D_GNU_SOURCE','-O1','-g','-fsanitize=address,undefined']
    subprocess.run(flags+['-c',str(work/'sha1.c'),'-o',str(work/'sha1.o')],check=True)
    subprocess.run(['clang','-c',str(work/'embedded.S'),'-o',str(work/'embedded.o')],check=True)
    binary=work/'deployer'
    subprocess.run(flags+['-Wall','-Wextra','-Werror','-I',str(work),str(ROOT/'src/deploy_native_game.c'),str(work/'sha1.o'),str(work/'embedded.o'),str(work/'fault.c'),'-Wl,--wrap=rename','-o',str(binary)],check=True)
    dest=data/'homebrew/PPSA94678'
    def reset():
        for child in data.iterdir():
            if child.is_dir() and not child.is_symlink():shutil.rmtree(child)
            else:child.unlink()
        dest.parent.mkdir(parents=True);shutil.copytree(old,dest)
        saves=data/'dkrr/profile-us-v77';saves.mkdir(parents=True);(saves/'save.bin').write_bytes(b'untouched save')
        (data/'dkrr/preferences').mkdir();(data/'dkrr/preferences/shared.json').write_bytes(b'untouched settings')
    def snapshot():
        return {str(p.relative_to(data)):p.read_bytes() for p in data.rglob('*') if p.is_file() and not p.is_symlink()}
    def run(ok,fault=False):
        env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1')
        if fault:env['DKRR_TEST_FAIL_COMMIT']='1'
        r=subprocess.run([str(binary)],env=env,capture_output=True,text=True)
        assert (r.returncode==0)==ok,(r.returncode,r.stdout,r.stderr)
        assert ('DKR-R UPDATE PASS.' in r.stdout)==ok
    reset();run(True);assert (dest/'eboot.bin').read_bytes()==(new/'eboot.bin').read_bytes()
    assert (dest/'assets/controller-audio/us-v77/0.pcm').read_bytes()==b'new synthetic sound'
    assert (dest/'assets/retained.txt').read_bytes()==b'preserve old asset'
    assert (data/'dkrr/profile-us-v77/save.bin').read_bytes()==b'untouched save'
    assert (data/'dkrr/preferences/shared.json').read_bytes()==b'untouched settings'
    before=snapshot();run(True);assert snapshot()==before
    assert (dest/'eboot.bin').stat().st_mode&0o777==0o755
    for path in ('sce_module/libc.prx','assets/controller-audio/us-v77/0.pcm'):
        reset();p=dest/path;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(b'foreign bytes');before=snapshot();run(False);assert snapshot()==before
    reset();(dest/'sce_module/libc.prx').unlink();before=snapshot();run(False);assert snapshot()==before
    reset();outside=work/'unrelated';outside.write_bytes(b'keep');(dest/'sce_sys/icon0.png').unlink();(dest/'sce_sys/icon0.png').symlink_to(outside);before=snapshot();run(False);assert snapshot()==before and outside.read_bytes()==b'keep'
    reset();(dest/'assets').rename(work/'displaced');(dest/'assets').symlink_to(work/'displaced',target_is_directory=True);before=snapshot();run(False);assert snapshot()==before
    reset();p=dest/'eboot.bin.dkrr-deploy-tmp';p.write_bytes(b'unrelated temp');before=snapshot();run(False);assert snapshot()==before
    reset();run(False,True);assert (dest/'eboot.bin').read_bytes()==(old/'eboot.bin').read_bytes()
    # Added assets and ROMs already committed, failure before executable rename:
    # rerunning the identical payload completes a known partial old/new bundle.
    assert (dest/'assets/controller-audio/us-v77/0.pcm').exists();run(True)
    assert (dest/'eboot.bin').read_bytes()==(new/'eboot.bin').read_bytes()
print('PASS combined update: real C under ASan/UBSan, exact predecessor, added assets/ROMs, preserved saves/settings, repeat/resume, injected commit failure, symlink/collision refusal')
