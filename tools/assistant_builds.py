#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Private completed-build history. A record alone never enables installation."""
import hashlib, json, os, pathlib, re, secrets, stat
from build_native_game_bundle_updater import inventory, inventory_hash
from build_native_game_deployer import ROM_HASHES
from analyze_session import elf_layout

def sha(path):
    digest=hashlib.sha256()
    with path.open('rb') as f:
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
    for folder in ('src','patches','tools','vendor','homebrew'):
        base=root/folder
        if base.is_dir():paths.extend(p for p in base.rglob('*') if p.is_file() and '__pycache__' not in p.parts)
    paths.extend(p for p in root.iterdir() if p.is_file() and (p.suffix in {'.sh','.json'} or p.name in {'setup.py','CMakeLists.txt'}))
    for p in sorted(paths):
        if p.is_symlink():raise ValueError('Source symlink refused')
        digest.update(str(p.relative_to(root)).encode()+b'\0'+bytes.fromhex(sha(p)))
    return digest.hexdigest()

def inspect(entry):
    if not isinstance(entry,dict) or set(entry)!={'game','source_sha256','installer_sha256','receipt_sha256'}:
        raise ValueError('Invalid completed-build record')
    if any(not re.fullmatch('[a-f0-9]{64}',entry[key]) for key in ('source_sha256','installer_sha256','receipt_sha256')):
        raise ValueError('Invalid completed-build digest')
    game=ordinary(pathlib.Path(entry['game']))
    receipt=ordinary(game/'private/CANDIDATE.json'); installer=ordinary(game/'private/dkrr-install.elf')
    if receipt.stat().st_size>1024*1024 or sha(receipt)!=entry['receipt_sha256'] or sha(installer)!=entry['installer_sha256']:
        raise ValueError('Completed build or installer changed')
    with installer.open('rb') as f:
        if f.read(4)!=b'\x7fELF':raise ValueError('Installer is not an ELF')
    spec=json.loads(receipt.read_text());app=game/'private/native/PPSA94678'
    if spec.get('title_id')!='PPSA94678' or spec.get('schema')!=1 or spec.get('app_relative_to_game_work')!='private/native/PPSA94678' or spec.get('symbols_relative_to_game_work')!='native-link/llvm-pie.elf':
        raise ValueError('Invalid build receipt identity')
    build=spec.get('build_id','')
    if not re.fullmatch('integrated-[a-f0-9]{24}',build):raise ValueError('Invalid build identity')
    files=inventory(ordinary(app))
    if files!=spec.get('files') or inventory_hash(files)!=spec.get('app_manifest_sha256') or files['eboot.bin']['sha256']!=spec.get('eboot_sha256'):
        raise ValueError('Application differs from its build receipt')
    symbols=ordinary(game/'native-link/llvm-pie.elf')
    if symbols.stat().st_size>512*1024*1024 or sha(symbols)!=spec.get('symbol_elf_sha256'):
        raise ValueError('Matching symbols changed or missing')
    raw=symbols.read_bytes();elf_layout(raw)
    if build.encode() not in raw or build.encode() not in (app/'eboot.bin').read_bytes():
        raise ValueError('Executable/symbol build identity mismatch')
    roms={}
    for rev,digest in ROM_HASHES.items():
        p=ordinary(game/'private/roms'/f'dkr.us.v{rev}.z64')
        if not p.is_file() or p.stat().st_size!=12*1024*1024 or hashlib.sha1(p.read_bytes()).hexdigest()!=digest:
            raise ValueError('Recovered owned ROM changed or missing')
        roms[str(rev)]=p
    return dict(entry=entry,game=game,spec=spec,roms=roms)

class History:
    def __init__(self,path):self.path=pathlib.Path(path).absolute()
    def read(self):
        ordinary(self.path)
        if not self.path.exists():return []
        if not stat.S_ISREG(self.path.stat().st_mode) or self.path.stat().st_size>65536:raise ValueError('Invalid private build history')
        value=json.loads(self.path.read_text())
        if not isinstance(value,dict) or value.get('schema')!=1 or not isinstance(value.get('builds'),list) or len(value['builds'])>32:
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
                installer_sha256=sha(game/'private/dkrr-install.elf'),receipt_sha256=sha(game/'private/CANDIDATE.json'))
