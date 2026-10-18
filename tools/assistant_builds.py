#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Private completed-build history. A record alone never enables installation."""
import hashlib, json, os, pathlib, re, secrets, stat
from build_native_game_bundle_updater import inventory, inventory_hash
from build_native_game_deployer import ROM_HASHES
from analyze_session import elf_layout
from report_diagnostic_session import read_regular
from build_universal_installer import PUBLIC_FILES

PAYLOADS = ('DKRR-Installer.elf', 'DKRR-Update.elf', 'DKRR-Update.dkru')

def sha(path):
    digest=hashlib.sha256()
    with os.fdopen(os.open(path, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK),'rb') as f:
        info=os.fstat(f.fileno())
        if not stat.S_ISREG(info.st_mode) or not 0 <= info.st_size <= 512*1024*1024:
            raise ValueError('Invalid build file')
        while chunk:=f.read(1024*1024):digest.update(chunk)
    return digest.hexdigest()

def ordinary(path):
    if not path.is_absolute():raise ValueError('Build path must be absolute')
    for p in (path,*path.parents):
        if p.is_symlink():raise ValueError('Build/history symlink refused')
    return path

def source_identity(root):
    digest=hashlib.sha256()
    paths=[]
    for folder in ('src','installer','patches','tools','vendor','homebrew','licenses'):
        base=root/folder
        if base.is_dir():paths.extend(p for p in base.rglob('*') if p.is_file() and '__pycache__' not in p.parts)
    paths.extend(p for p in root.iterdir() if p.is_file() and (p.suffix in {'.sh','.json'} or p.name in {'setup.py','CMakeLists.txt','LICENSE','THIRD_PARTY.md'}))
    for p in sorted(paths):
        if p.is_symlink():raise ValueError('Source symlink refused')
        digest.update(str(p.relative_to(root)).encode()+b'\0'+bytes.fromhex(sha(p)))
    return digest.hexdigest()

def inspect(entry):
    if not isinstance(entry,dict) or set(entry)!={'game','source_sha256','payload_sha256','package_receipt_sha256','receipt_sha256'}:
        raise ValueError('Invalid completed-build record')
    if any(not isinstance(entry[key],str) or not re.fullmatch('[a-f0-9]{64}',entry[key]) for key in ('source_sha256','package_receipt_sha256','receipt_sha256')):
        raise ValueError('Invalid completed-build digest')
    if not isinstance(entry['payload_sha256'],dict) or set(entry['payload_sha256'])!=set(PAYLOADS) or any(not isinstance(v,str) or not re.fullmatch('[a-f0-9]{64}',v) for v in entry['payload_sha256'].values()):
        raise ValueError('Invalid universal payload inventory')
    game=ordinary(pathlib.Path(entry['game']))
    receipt=ordinary(game/'private/CANDIDATE.json')
    raw=read_regular(receipt,1024*1024)
    if hashlib.sha256(raw).hexdigest()!=entry['receipt_sha256']:
        raise ValueError('Completed build receipt changed')
    spec=json.loads(raw);app=game/'private/native/PPSA94678'
    if not isinstance(spec,dict) or spec.get('title_id')!='PPSA94678' or spec.get('schema')!=1 or spec.get('app_relative_to_game_work')!='private/native/PPSA94678' or spec.get('symbols_relative_to_game_work')!='native-link/llvm-pie.elf':
        raise ValueError('Invalid build receipt identity')
    build=spec.get('build_id','')
    if not re.fullmatch('integrated-[a-f0-9]{24}',build):raise ValueError('Invalid build identity')
    files=inventory(ordinary(app))
    if files!=spec.get('files') or inventory_hash(files)!=spec.get('app_manifest_sha256') or files['eboot.bin']['sha256']!=spec.get('eboot_sha256'):
        raise ValueError('Application differs from its build receipt')
    symbols=ordinary(game/'native-link/llvm-pie.elf')
    if symbols.stat().st_size>512*1024*1024 or sha(symbols)!=spec.get('symbol_elf_sha256'):
        raise ValueError('Matching symbols changed or missing')
    raw=read_regular(symbols,512*1024*1024);elf_layout(raw)
    if build.encode() not in raw or build.encode() not in (app/'eboot.bin').read_bytes():
        raise ValueError('Executable/symbol build identity mismatch')
    roms={}
    for rev,digest in ROM_HASHES.items():
        p=ordinary(game/'private/roms'/f'dkr.us.v{rev}.z64')
        raw=read_regular(p,12*1024*1024)
        if len(raw)!=12*1024*1024 or hashlib.sha1(raw).hexdigest()!=digest:
            raise ValueError('Recovered owned ROM changed or missing')
        roms[str(rev)]=p
    package_dir=ordinary(game/'private/universal')
    package_raw=read_regular(ordinary(package_dir/'CANDIDATE.json'),1024*1024)
    if hashlib.sha256(package_raw).hexdigest()!=entry['package_receipt_sha256']:
        raise ValueError('Universal package receipt changed')
    package=json.loads(package_raw)
    public={name:meta for name,meta in files.items() if not name.startswith('assets/controller-audio/')}
    if not isinstance(package,dict) or package.get('schema')!=1 or package.get('build_id')!=build or set(public)!=PUBLIC_FILES or package.get('files')!=public or package.get('roms_embedded') is not False or package.get('original_pcm_embedded') is not False:
        raise ValueError('Universal package differs from the recorded game')
    if not isinstance(package.get('payloads'),dict) or set(package['payloads'])!=set(PAYLOADS):
        raise ValueError('Incomplete universal package')
    payloads={}
    for name in PAYLOADS:
        p=ordinary(package_dir/name);digest=sha(p);meta=package['payloads'][name]
        if not isinstance(meta,dict) or digest!=entry['payload_sha256'][name] or meta.get('sha256')!=digest or meta.get('size')!=p.stat().st_size:
            raise ValueError('Universal payload changed')
        with p.open('rb') as f:magic=f.read(8)
        if (name.endswith('.elf') and magic[:4]!=b'\x7fELF') or (name.endswith('.dkru') and magic!=b'DKRRPK2\n'):
            raise ValueError('Invalid universal payload format')
        payloads[name]=p
    return dict(entry=entry,game=game,spec=spec,roms=roms,payloads=payloads)

class History:
    def __init__(self,path):self.path=pathlib.Path(path).absolute()
    def read(self):
        ordinary(self.path)
        if not self.path.exists():return []
        if not stat.S_ISREG(self.path.stat().st_mode) or self.path.stat().st_size>65536:raise ValueError('Invalid private build history')
        value=json.loads(read_regular(self.path,65536))
        if not isinstance(value,dict) or value.get('schema')!=1 or not isinstance(value.get('builds'),list) or len(value['builds'])>32 or any(not isinstance(row,dict) for row in value['builds']):
            raise ValueError('Invalid private build history')
        return value['builds']
    def remember(self,entry):
        ordinary(self.path)
        rows=self.read(); rows=[entry]+[row for row in rows if row.get('game')!=entry['game']]
        self.path.parent.mkdir(parents=True,exist_ok=True,mode=0o700)
        self.path.parent.chmod(0o700)
        temporary=self.path.with_name('.history-'+secrets.token_hex(8))
        try:
            with os.fdopen(os.open(temporary,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600),'w') as f:
                json.dump(dict(schema=1,builds=rows[:32]),f);f.write('\n');f.flush();os.fsync(f.fileno())
            os.replace(temporary,self.path)
        finally:
            if temporary.exists():temporary.unlink()

def completed_entry(root,game):
    return dict(game=str(game),source_sha256=source_identity(root),
                payload_sha256={name:sha(game/'private/universal'/name) for name in PAYLOADS},
                package_receipt_sha256=sha(game/'private/universal/CANDIDATE.json'),
                receipt_sha256=sha(game/'private/CANDIDATE.json'))
