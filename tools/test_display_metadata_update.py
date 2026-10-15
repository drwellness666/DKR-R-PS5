#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Actual combined updater for the single qualified display metadata transition."""
import hashlib,json,os,pathlib,subprocess,tarfile,tempfile
from build_preview_update_set import metadata_transition
root=pathlib.Path(__file__).resolve().parent.parent
with tempfile.TemporaryDirectory(prefix='dkrr-display-metadata-') as folder:
    work=pathlib.Path(folder);before=work/'before';after=work/'after'
    for p in (before,after):(p/'sce_sys').mkdir(parents=True)
    old={'titleId':'PPSA94678','attribute3':0,'nested':{'preserved':'yes'}};new=dict(old,attribute3=0x40040)
    def write(value,path):path.write_text(json.dumps(value,indent=2)+'\n')
    write(old,before/'sce_sys/param.json');write(new,after/'sce_sys/param.json')
    assert metadata_transition(before,after,True)
    def refused(allowed):
        try:metadata_transition(before,after,allowed)
        except ValueError:return
        raise AssertionError('Unqualified metadata accepted')
    refused(False)
    for value in (dict(new,titleId='PPSA00000'),dict(new,attribute3=0x80000),dict(new,extra=1),dict(new,nested={'preserved':'no'}),dict(new,attribute3=True)):
        write(value,after/'sce_sys/param.json');refused(True)
    (after/'sce_sys/param.json').write_text('{"titleId":"PPSA94678","attribute3":0,"attribute3":262208}')
    refused(True);write(new,after/'sce_sys/param.json')
    data=(before/'sce_sys/param.json').read_bytes(),(after/'sce_sys/param.json').read_bytes()
    app=work/'app';metas=[app/'sce_sys',work/'registered',work/'appmeta'];code=(b'old code',b'new code')
    with tarfile.open(root/'vendor/ps5-sdk-source.tar.gz') as tar:sha=tar.extractfile('crt/nid.c').read().decode()
    sha=sha.replace('nid_encode(','dkrr_unused_nid_encode(')+'''\n#include <unistd.h>
#include <errno.h>
int dkrr_sha1_fd(int fd,unsigned char digest[20]){SHA1_CTX c;unsigned char b[8192];ssize_t n;SHA1Init(&c);for(;;){n=read(fd,b,sizeof(b));if(n<0&&errno==EINTR)continue;if(n<0)return -1;if(!n)break;SHA1Update(&c,b,(unsigned)n);}SHA1Final(digest,&c);return 0;}\n'''
    (work/'sha.c').write_text(sha)
    subprocess.run(['clang','-D_GNU_SOURCE','-O1','-fsanitize=address,undefined','-c',str(work/'sha.c'),'-o',str(work/'sha.o')],check=True)
    def array(name,values):return 'static const unsigned char '+name+'[]={'+','.join(map(str,values))+'};\n'
    for rollback in (False,True):
        build=work/('back' if rollback else 'forward');build.mkdir();oldbytes,newbytes=data[::-1] if rollback else data;oldcode,newcode=code[::-1] if rollback else code
        header='struct update_file {const char*path;const unsigned char*start;unsigned long long old_size,new_size;const char*old_sha1,*new_sha1;unsigned mode;};\n'+array('metadata',newbytes)+array('code',newcode)
        def update_row(path,a,b,symbol):return '{'+json.dumps(str(path))+f',{symbol},{len(a)},{len(b)},"{hashlib.sha1(a).hexdigest()}","{hashlib.sha1(b).hexdigest()}",0644'+'}'
        rows=[update_row(app/'eboot.bin',oldcode,newcode,'code')]+[update_row(m/'param.json',oldbytes,newbytes,'metadata') for m in metas]
        header+='static const struct update_file update_files[]={'+','.join(rows)+'};\n';(build/'native_update_files.h').write_text(header)
        art=b'paired-art';header='struct poster_file {const char*path,*backup;unsigned long long size;const char*sha1;};\n'
        def guard(path,backup=False):return '{'+json.dumps(str(path))+','+(json.dumps(str(path)+'.dkrr-retired-launch-poster') if backup else '0')+f',{len(art)},"{hashlib.sha1(art).hexdigest()}"'+'}'
        header+='static const struct poster_file poster_guards[]={'+','.join(guard(m/n) for m in metas for n in ('icon0.png','pic0.dds'))+'};\n'
        header+='static const struct poster_file poster_files[]={'+','.join(guard(m/'pic1.dds',True) for m in metas)+'};\n';(build/'native_poster_retirement.h').write_text(header)
        subprocess.run(['clang','-std=c11','-D_GNU_SOURCE','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-DDKR_KEEP_POSTER=1','-DDKR_RESTORE_POSTER='+str(int(rollback)),'-DDKR_UPDATE_VERSION=41','-I',str(build),str(root/'src/update_game_preview.c'),str(work/'sha.o'),'-o',str(build/'test')],check=True)
    def reset():
        app.mkdir(exist_ok=True)
        (app/'eboot.bin').write_bytes(code[0])
        for m in metas:
            m.mkdir(exist_ok=True)
            for p in m.iterdir():p.unlink()
            (m/'param.json').write_bytes(data[0])
            for n in ('icon0.png','pic0.dds','pic1.dds'):(m/n).write_bytes(b'paired-art')
    def snapshot():return {str(p):p.read_bytes() for base in (app,metas[1],metas[2]) for p in base.rglob('*') if p.is_file()}
    def run(back=False,ok=True):
        p=subprocess.run([str(work/('back' if back else 'forward')/'test')],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'));assert (p.returncode==0)==ok,p.stdout+p.stderr
    reset();original=snapshot();run();run();assert all((m/'param.json').read_bytes()==data[1] for m in metas);run(True);run(True);assert snapshot()==original
    reset();(metas[1]/'param.json').write_bytes(data[1]);run();run(True);assert snapshot()==original
    for bad in (metas[2]/'param.json',metas[2]/'param.json.dkrr-v41-update'):
        reset();bad.write_bytes(b'foreign');start=snapshot();run(ok=False);assert snapshot()==start
    reset();(metas[1]/'param.json').unlink();(metas[1]/'param.json').symlink_to(metas[0]/'param.json');start=snapshot();run(ok=False);assert snapshot()==start
    print('PASS metadata contract and actual combined 3-copy update: repeat/resume, exact restoration, full refusal before writes on foreign/symlink/temp metadata')
