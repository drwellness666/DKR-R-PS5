#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Stage verified public sources in the WSL filesystem, excluding all private work."""
import hashlib, os, pathlib, shutil, sys, tempfile

def stage(source,base):
    source=pathlib.Path(source).resolve();base=pathlib.Path(base).absolute()
    manifest=source/'SHA256SUMS'
    if manifest.is_symlink() or manifest.stat().st_size>1024*1024: raise ValueError('Invalid source inventory')
    data=manifest.read_bytes(); rows={}
    for line in data.decode().splitlines():
        digest,name=line.split('  ',1);p=pathlib.PurePosixPath(name)
        if len(digest)!=64 or any(c not in '0123456789abcdef' for c in digest) or p.is_absolute() or '\\' in name or '..' in p.parts or not p.parts or p.parts[0] in {'work','private','.git','bin','evidence'} or name in rows: raise ValueError('Unsafe or duplicate source inventory entry')
        rows[name]=digest
    if not {'setup.py','Start-Linux.sh','tools/start_build_assistant.sh','tools/setup_ui.html'}.issubset(rows): raise ValueError('Incomplete source release')
    for name,digest in rows.items():
        p=source/name
        if any((source/pathlib.Path(*p.relative_to(source).parts[:i])).is_symlink() for i in range(1,len(p.relative_to(source).parts)+1)) or not p.is_file() or hashlib.sha256(p.read_bytes()).hexdigest()!=digest: raise ValueError('Source checksum mismatch: '+name)
    base.mkdir(parents=True,exist_ok=True,mode=0o700)
    if base.is_symlink(): raise ValueError('Build destination cannot be a symlink')
    dest=base/('source-'+hashlib.sha256(data).hexdigest()[:16])
    if dest.exists():
        if dest.is_symlink() or (dest/'SHA256SUMS').read_bytes()!=data: raise ValueError('Existing source staging differs')
        for name,digest in rows.items():
            p=dest/name
            if any((dest/pathlib.Path(*p.relative_to(dest).parts[:i])).is_symlink() for i in range(1,len(p.relative_to(dest).parts)+1)) or not p.is_file() or hashlib.sha256(p.read_bytes()).hexdigest()!=digest: raise ValueError('Staged source changed: '+name)
        return dest
    temp=pathlib.Path(tempfile.mkdtemp(prefix='.staging-',dir=base))
    try:
        for name in [*rows,'SHA256SUMS','vendor/SHA256SUMS']:
            src=source/name;out=temp/name;out.parent.mkdir(parents=True,exist_ok=True)
            if src.is_symlink() or not src.is_file(): raise ValueError('Invalid source file: '+name)
            shutil.copyfile(src,out);out.chmod(0o755 if out.suffix=='.sh' else 0o644)
        temp.rename(dest)
    finally:
        if temp.exists():shutil.rmtree(temp)
    return dest
if __name__=='__main__':
    try:print(stage(sys.argv[1],pathlib.Path.home()/'.local/share/dkrr-ps5'))
    except (OSError,ValueError,IndexError) as e:raise SystemExit(str(e))
