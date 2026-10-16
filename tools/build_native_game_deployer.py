#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build one private app/asset/ROM update, guarded by a complete local predecessor."""
import argparse, hashlib, json, os, pathlib, subprocess
from build_native_game_bundle_updater import inventory, inventory_hash, TITLE
from check_update_baseline import baseline_output

ROM_HASHES = {77: '0cb115d8716dbbc2922fda38e533b9fe63bb9670', 80: '6d96743d46f8c0cd0edb0ec5600b003c89b93755'}

def plan(previous, game, expected):
    current = game/'private/native'/TITLE
    old, new = inventory(previous, previous=True), inventory(current)
    old_output = baseline_output(previous, expected)
    if (previous/'sce_sys/param.json').read_bytes()!=(current/'sce_sys/param.json').read_bytes():
        raise ValueError('Title metadata change requires a separately qualified migration')
    if baseline_output(current, inventory_hash(new)) != old_output:
        raise ValueError('Output declaration differs from the recorded predecessor')
    if sorted(json.loads((game/'private/roms/selected-revisions.json').read_text())['revisions']) != [77,80]:
        raise ValueError('Both owned US revisions are required')
    entries = []
    for name in sorted(old.keys() | new.keys(), key=lambda n: (n == 'eboot.bin', n == 'sce_sys/param.json', n)):
        entries.append(dict(path='/data/homebrew/'+TITLE+'/'+name, source=current/name if name in new else None,
                            old=old.get(name), new=new.get(name,old.get(name))))
    for rev, digest in ROM_HASHES.items():
        source = game/'private/roms'/f'dkr.us.v{rev}.z64'
        if source.is_symlink() or source.stat().st_size != 12*1024*1024 or hashlib.sha1(source.read_bytes()).hexdigest() != digest:
            raise ValueError('Owned canonical ROM verification failed')
        entries.insert(0, dict(path=f'/data/dkrr/roms/dkr.us.v{rev}.z64', source=source, old=None,
                              new=dict(size=source.stat().st_size,sha1=digest,mode=0o644)))
    return entries

def emit(entries, work):
    assembly = '.section .rodata\n'
    header = ('struct deploy_file { const char *path; const unsigned char *data; unsigned long long old_size,new_size; '
              'const char *old_sha1,*new_sha1; unsigned mode; int required; };\n')
    rows=[]; directories={'/data/homebrew','/data/homebrew/'+TITLE,'/data/dkrr','/data/dkrr/roms',
                         '/data/dkrr/renderer','/data/dkrr/preferences','/data/dkrr/diagnostics',
                         '/data/dkrr/profile-us-v77','/data/dkrr/profile-us-v80'}
    for i, entry in enumerate(entries):
        old,new=entry['old'],entry['new'];symbol='0';source=entry['source']
        if source is not None:
            if any(c in str(source) for c in '\r\n"\\'):
                raise ValueError('Unsupported embedded input path')
            if source.stat().st_size != new['size'] or hashlib.sha1(source.read_bytes()).hexdigest()!=new['sha1']:
                raise ValueError('Deployment source changed during planning')
            symbol=f'deploy_data_{i}'
            header+=f'extern const unsigned char {symbol}[];\n'
            assembly+=f'.balign 16\n.global {symbol}\n{symbol}:\n.incbin "{source}"\n'
        path=entry['path']
        if len((path+'.dkrr-deploy-tmp').encode())>=256:raise ValueError('Deployment path too long')
        directories.update(str(p) for p in pathlib.PurePosixPath(path).parents if str(p) not in ('/','/data'))
        rows.append('{'+json.dumps(path)+','+symbol+f',{old["size"] if old else 0},{new["size"]},'+
                    (json.dumps(old['sha1']) if old else '0')+','+json.dumps(new['sha1'])+f',{new["mode"]:04o},{int(old is not None)}'+'}')
    header+='static const struct deploy_file deploy_files[]={\n'+',\n'.join(rows)+'\n};\n'
    dirs=[]
    for path in sorted(directories,key=lambda n:(n.count('/'),n)):
        mode=0o1777 if path=='/data/dkrr/diagnostics' else 0o777 if path in {'/data/dkrr/renderer','/data/dkrr/preferences','/data/dkrr/profile-us-v77','/data/dkrr/profile-us-v80'} else 0o755
        dirs.append('{'+json.dumps(path)+f',0{mode:o}'+'}')
    header+='static const struct { const char *path; unsigned mode; } deploy_dirs[]={'+','.join(dirs)+'};\n'
    (work/'native_deploy_files.h').write_text(header);(work/'embedded.S').write_text(assembly)

def build(sdk, previous, game, output, expected):
    for path in (sdk,previous,game,output):
        if not path.is_absolute():raise ValueError('Use absolute build paths')
    if 'private' not in output.parts or output.exists() or output.is_symlink():
        raise ValueError('Use a new output under a private directory')
    entries=plan(previous,game,expected)
    work=output.parent/(output.stem+'-source');work.mkdir(parents=True,exist_ok=False,mode=0o700)
    emit(entries,work)
    root=pathlib.Path(__file__).resolve().parents[1]
    gpu=pathlib.Path(os.environ.get('DKR_GPU_WORK',str(root/'work/vulkan')))
    sha=(gpu/'PS5_PayloadSDK/crt/nid.c').read_text().replace('nid_encode(', 'dkrr_update_unused_nid_encode(')
    sha+='''\n#include <errno.h>
extern long read(int,void *,unsigned long);
int dkrr_sha1_fd(int fd,unsigned char digest[20]) {
 SHA1_CTX ctx; unsigned char data[8192]; long n; SHA1Init(&ctx);
 for(;;) { n=read(fd,data,sizeof(data)); if(n<0 && errno==EINTR)continue;
  if(n<0)return -1; if(!n)break; SHA1Update(&ctx,data,(unsigned)n); }
 SHA1Final(digest,&ctx); return 0;
}
'''
    (work/'sha1.c').write_text(sha)
    cc=str(sdk/'bin/prospero-clang')
    for name in ('sha1.c','embedded.S'):
        subprocess.run([cc,'-O2','-c',str(work/name),'-o',str(work/(name+'.o'))],check=True)
    subprocess.run([cc,'-std=c11','-O2','-Wall','-Wextra','-Werror','-I',str(work),str(root/'src/deploy_native_game.c'),
                    str(work/'sha1.c.o'),str(work/'embedded.S.o'),'-o',str(output)],check=True)
    subprocess.run([str(sdk/'bin/prospero-strip'),'--strip-debug',str(output)],check=True)
    output.chmod(0o600)
    (work/'plan.json').write_text(json.dumps({'previous_manifest_sha256':expected,'destinations':[e['path'] for e in entries]},indent=2)+'\n')
    return output

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('sdk','previous','game','output'):p.add_argument(name,type=pathlib.Path)
    p.add_argument('--expected-previous-manifest-sha256',required=True);a=p.parse_args()
    try:out=build(a.sdk,a.previous,a.game,a.output,a.expected_previous_manifest_sha256)
    except (ValueError,OSError) as e:p.error(str(e))
    print('Built private combined update:',out)
    print('Contains owned ROMs and game code. Never share this ELF. No console operation performed.')
if __name__=='__main__':main()
