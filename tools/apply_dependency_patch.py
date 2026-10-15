# SPDX-License-Identifier: GPL-3.0-or-later
"""Apply a pinned text patch in a private scratch repo, then verify changed bytes.

git apply run below an unrelated outer Git root can silently skip its paths.
Only affected ordinary files are staged here; the dependency/outer Git metadata
is never initialized, staged or changed.
"""
import hashlib,pathlib,re,shutil,subprocess,tempfile
def apply_dependency_patch(target,patch):
    target=target.resolve();patch=patch.resolve();text=patch.read_text();names=[]
    for a,b in re.findall(r'^diff --git a/([^\r\n ]+) b/([^\r\n ]+)$',text,re.M):
        if a!=b:raise ValueError('Dependency renames require separate review')
        path=pathlib.PurePosixPath(a)
        if path.is_absolute() or '..' in path.parts or '.git' in path.parts:raise ValueError('Unsafe dependency patch path')
        if a in names:raise ValueError('Duplicate dependency patch path')
        names.append(a)
    if not names:raise ValueError('Dependency patch contains no supported file paths')
    def value(path):
        if path.is_symlink():raise ValueError('Dependency patch input is a symlink')
        if not path.exists():return None
        if not path.is_file():raise ValueError('Dependency patch input is not an ordinary file')
        return path.read_bytes()
    original={name:value(target/name) for name in names}
    with tempfile.TemporaryDirectory(prefix='dkrr-dependency-patch-') as folder:
        scratch=pathlib.Path(folder)
        subprocess.run(['git','init','--quiet',str(scratch)],check=True)
        for name,data in original.items():
            if data is not None:
                file=scratch/name;file.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(target/name,file)
        command=['git','-C',str(scratch),'apply','--recount','--whitespace=nowarn','--ignore-space-change']
        subprocess.run(command+['--check',str(patch)],check=True)
        subprocess.run(command+[str(patch)],check=True)
        result={name:value(scratch/name) for name in names}
        if any(original[name]==result[name] for name in names):raise ValueError('Dependency patch reported success without changing every expected file')
        # Validate all affected originals again before copying any result.
        if any(value(target/name)!=original[name] for name in names):raise ValueError('Dependency changed during patch staging')
        for name,data in result.items():
            file=target/name
            if data is None:file.unlink()
            else:
                file.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(scratch/name,file)
        rows=[]
        for name,data in result.items():
            if value(target/name)!=data:raise ValueError('Dependency patch result readback mismatch')
            rows.append({'file':name,'before_sha256':hashlib.sha256(original[name]).hexdigest() if original[name] is not None else None,'after_sha256':hashlib.sha256(data).hexdigest() if data is not None else None})
        return {'patch_sha256':hashlib.sha256(patch.read_bytes()).hexdigest(),'files':rows}
