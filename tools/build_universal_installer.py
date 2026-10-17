#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Prepare ROM-free installer/update candidates. Distribution review is separate."""
import argparse,hashlib,json,pathlib,re,subprocess,tarfile
from build_native_game_bundle_updater import inventory,inventory_hash
ROOT=pathlib.Path(__file__).resolve().parents[1]
PUBLIC_FILES=set(re.findall(r'^    "([^"]+)",$',(ROOT/'src/ps5_update_files.h').read_text(),re.M))
if len(PUBLIC_FILES)!=37:raise ValueError('Unexpected reviewed application allowlist')

def sha1(data):return hashlib.sha1(data).hexdigest()
def release_files(app,strict=False):
    files=inventory(app)
    selected={}
    for name,meta in files.items():
        # Generated original sounds are rebuilt from the user's ROMs on PS5.
        if name.startswith('assets/controller-audio/'):continue
        if name not in PUBLIC_FILES:raise ValueError('Unreviewed public application file: '+name)
        data=(app/name).read_bytes()
        if data[:4] in (bytes.fromhex('80371240'),bytes.fromhex('37804012'),bytes.fromhex('40123780')):
            raise ValueError('ROM header in application package')
        selected[name]=meta
    if not selected or len(selected)>100:raise ValueError('Unexpected release file count')
    if strict and set(selected)!=PUBLIC_FILES:raise ValueError('Incomplete reviewed public application assets')
    return selected

def emit(app,build,work,migrations,sequence=3):
    if type(sequence) is not int or not 1<=sequence<=2147483647:raise ValueError('Invalid release sequence')
    if not re.fullmatch('integrated-[a-f0-9]{24}',build):raise ValueError('Invalid native build identity')
    files=release_files(app)
    h='#include "dkrr_install.h"\nstruct release_file {const char *path;const unsigned char *start,*end;unsigned mode;const char *sha1;};\n'
    h+='static const char release_build_id[]='+json.dumps(build)+';\nstatic const char release_stage_id[]='+json.dumps(build+'-r'+str(sequence))+';\nstatic const unsigned release_sequence='+str(sequence)+';\n'
    assembly='.section .rodata\n';rows=[];blobs={}
    for i,(name,meta) in enumerate(sorted(files.items())):
        source=app/name
        if any(c in str(source) for c in '\n\r"\\'):raise ValueError('Unsupported application path')
        h+=f'extern const unsigned char release_{i}_start[],release_{i}_end[];\n'
        key=(meta['size'],meta['sha256'])
        if key in blobs:
            first=blobs[key]
            assembly+=f'.global release_{i}_start,release_{i}_end\n.set release_{i}_start,release_{first}_start\n.set release_{i}_end,release_{first}_end\n'
        else:
            blobs[key]=i
            assembly+=f'.balign 16\n.global release_{i}_start,release_{i}_end\nrelease_{i}_start:\n.incbin "{source}"\nrelease_{i}_end:\n'
        rows.append('{'+json.dumps(name)+f',release_{i}_start,release_{i}_end,0{meta["mode"]:o},'+json.dumps(meta['sha1'])+'}')
    h+='static const struct release_file release_files[]={'+','.join(rows)+'};\n'
    legacy=[]
    for app_dir,receipt in migrations:
        recorded=json.loads(receipt.read_text());old=inventory(app_dir,previous=True)
        if inventory_hash(old)!=recorded['app_manifest_sha256'] or old!=recorded['files']:
            raise ValueError('Migration application differs from its recorded receipt')
        bid=recorded['build_id']
        if not re.fullmatch('integrated-[a-f0-9]{24}',bid):raise ValueError('Invalid migration build')
        r=[]
        for name,v in sorted(old.items()):r.append('{'+json.dumps(name)+f',{v["size"]},'+json.dumps(v['sha1'])+f',0{v["mode"]:o}'+'}')
        legacy.append('{'+json.dumps(bid)+f',0,{len(r)},'+'{'+','.join(r)+'}'+'}')
    h+='static const struct dkrr_manifest legacy_manifests[]={'+','.join(legacy or ['{"",0,0,{{0}}}'])+'};\n'
    (work/'release_files.h').write_text(h);(work/'embedded.S').write_text(assembly)
    return files

def hash_source():
    with tarfile.open(ROOT/'vendor/ps5-sdk-source.tar.gz') as a:
        text=a.extractfile('crt/nid.c').read().decode().replace('nid_encode(','dkrr_unused_nid_encode(')
    return text+'''\n#include <errno.h>
extern long read(int,void*,unsigned long);
int dkrr_sha1_fd(int fd,unsigned char digest[20]) {
 SHA1_CTX c; unsigned char b[8192];long n;SHA1Init(&c);
 for(;;){n=read(fd,b,sizeof(b));if(n<0&&errno==EINTR)continue;if(n<0)return -1;if(!n)break;SHA1Update(&c,b,(unsigned)n);}
 SHA1Final(digest,&c);return 0;
}
void dkrr_sha1_bytes(const void*data,unsigned long n,unsigned char digest[20]) {
 SHA1_CTX c;SHA1Init(&c);SHA1Update(&c,data,(unsigned)n);SHA1Final(digest,&c);
}
'''

def build(sdk,app,receipt,output,migrations=()):
    for p in (sdk,app,receipt,output):
        if not p.is_absolute() or p.is_symlink():raise ValueError('Use ordinary absolute paths')
    if 'private' not in output.parts or output.exists():raise ValueError('Use a new private candidate output directory')
    record=json.loads(receipt.read_text());all_files=inventory(app)
    release_files(app,strict=True)
    if record['files']!=all_files or record['app_manifest_sha256']!=inventory_hash(all_files):raise ValueError('Game differs from its native receipt')
    output.mkdir(parents=True,mode=0o700);work=output/'source';work.mkdir(mode=0o700)
    sequence=json.loads((ROOT/'PORT_RELEASE.json').read_text()).get('installer_sequence',3)
    files=emit(app,record['build_id'],work,migrations,sequence);(work/'sha1.c').write_text(hash_source())
    cc=str(sdk/'bin/prospero-clang');objects=[]
    for source in (work/'embedded.S',work/'sha1.c',ROOT/'installer/files.c',ROOT/'installer/rom.c',ROOT/'installer/audio.c',ROOT/'installer/vadpcm.c',ROOT/'installer/register.c'):
        obj=work/(source.name+'.o');flags=['-std=c11','-O2','-Wall','-Wextra','-Werror'] if source.parent==ROOT/'installer' else ['-O2']
        subprocess.run([cc,*flags,'-I',str(ROOT/'installer'),'-c',str(source),'-o',str(obj)],check=True);objects.append(obj)
    built={}
    for edition in ('Installer','Update','Network-Update'):
        target=output/('DKRR-'+edition+'.elf')
        subprocess.run([cc,'-std=c11','-O2','-Wall','-Wextra','-Werror',f'-DDKRR_UPDATE_ONLY={int(edition!="Installer")}',f'-DDKRR_NETWORK_UPDATE={int(edition=="Network-Update")}','-I',str(ROOT/'installer'),'-I',str(work),str(ROOT/'installer/main.c'),*[str(o) for o in objects],'-o',str(target)],check=True)
        subprocess.run([str(sdk/'bin/prospero-strip'),'--strip-debug',str(target)],check=True);target.chmod(0o600)
        built[target.name]=dict(size=target.stat().st_size,sha256=hashlib.sha256(target.read_bytes()).hexdigest())
    # The installed game uses this data-only package directly. The legacy
    # network ELF above exists only to migrate earlier Beta 3.3 clients.
    from build_native_update_package import build as build_data_package
    native_package=output/'DKRR-Update.dkru'
    package=build_data_package(app,receipt,sequence,native_package)
    built[native_package.name]={k:package[k] for k in ('size','sha256')}
    source_hashes={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted((ROOT/'installer').glob('*')) if p.is_file()}
    unique={(f['size'],f['sha256']) for f in files.values()}
    metadata=dict(schema=1,build_id=record['build_id'],release_sequence=sequence,files=files,payloads=built,installer_sources=source_hashes,roms_embedded=False,original_pcm_embedded=False,
                  unique_embedded_blobs=len(unique),deduplicated_bytes=sum(f['size'] for f in files.values())-sum(size for size,_ in unique),
                  legacy_builds=[json.loads(r.read_text())['build_id'] for _,r in migrations],console_executed=False,distribution_review_complete=False,
                  scope='ROM-free package candidates. Runtime has compiled game functions; full corresponding-source/artwork/component review is required before distribution.')
    (output/'CANDIDATE.json').write_text(json.dumps(metadata,indent=2)+'\n');(output/'CANDIDATE.json').chmod(0o600)
    return metadata

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for n in ('sdk','app','receipt','output'):p.add_argument(n,type=pathlib.Path)
    p.add_argument('--migration',nargs=2,type=pathlib.Path,action='append',default=[]);a=p.parse_args()
    try:result=build(a.sdk,a.app,a.receipt,a.output,a.migration)
    except (ValueError,KeyError,OSError) as e:p.error(str(e))
    print(json.dumps({k:result[k] for k in ('build_id','payloads','roms_embedded','original_pcm_embedded','console_executed','distribution_review_complete')},indent=2))
if __name__=='__main__':main()
