#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Apply exact native PS5 deltas after the official 1.0.9 source composition."""
import argparse,hashlib,json,re
from pathlib import Path
from compose_upstream_release import delta
INCLUDE=re.compile(r'^#include "([^"\n]+)"',re.M)
def sha(data):return hashlib.sha256(data).hexdigest()
def select_source(logical,rows,source,build):
    leaf=Path(logical).name
    if leaf=='runtime_online_ui.inl':return build/'generated/online-directory/runtime_online_ui.inl'
    if leaf=='runtime_graphics_ui.inl':return build/'generated/upstream-sync/graphics.inl'
    paths={Path(row['path']) for row in rows if Path(row['path']).name==leaf}
    if len(paths)>1:
        preferred={Path(row['path']) for row in rows if row['target']=='DKRPortGame' and Path(row['path']).name==leaf}
        if len(preferred)==1:paths=preferred
    if len(paths)>1:raise ValueError('Ambiguous final source: '+logical)
    return next(iter(paths),source/'runtime-recomp/ps5-release/native-base'/Path(logical).relative_to('runtime-recomp'))
def canonical(text,logical,source,build,adapters):
    base=source/'runtime-recomp/ps5-release/native-base'/Path(logical).relative_to('runtime-recomp')
    names={m[1] for m in INCLUDE.finditer(base.read_text())} if base.is_file() else set()
    restore={}
    def one(m):
        path=Path(m[1]);key=next((n for n in names if path.is_absolute() and path.as_posix().endswith('/'+n)),None)
        if key:restore[key]=m[1];return '#include "'+key+'"'
        return m[0]
    text=INCLUDE.sub(one,text)
    for path,token in ((build,'@DKRR_BUILD@'),(source,'@DKRR_SOURCE@'),(adapters,'@DKRR_ADAPTERS@')):text=text.replace(str(path),token)
    return text,restore
def expand(text,source,build,adapters):
    for token,path in (('@DKRR_BUILD@',build),('@DKRR_SOURCE@',source),('@DKRR_ADAPTERS@',adapters)):text=text.replace(token,str(path))
    return text

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source',type=Path,required=True);p.add_argument('--build',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    source,build,output=a.source.resolve(),a.build.resolve(),a.output.resolve();root=Path(__file__).resolve().parents[1];rows=json.loads((build/'ps5-release-graph.json').read_text())
    manifest=json.loads((root/'patches/dkrr-109-final-manifest.json').read_text());patch=(root/'patches/dkrr-109-native-final.patch').read_bytes()
    if sha(patch)!=manifest['patch_sha256']:raise ValueError('Changed final native patch')
    # Split only the exact unified file records, then verify every before/after.
    parts={}
    for match in re.finditer(rb'^--- a/([^\n]+)\n',patch,re.M):parts[match[1].decode()]=match.start()
    keys=list(parts);changes=[];prepared={}
    for index,key in enumerate(keys):
        raw=patch[parts[key]:parts[keys[index+1]] if index+1<len(keys) else len(patch)]
        path=select_source(key,rows,source,build);before,restore=canonical(path.read_text(),key,source,build,root/'src')
        row=manifest['files'][key]
        if sha(before.encode())!=row['original_sha256']:raise ValueError('Changed final release input: '+key)
        after=delta(before.encode(),raw)
        if sha(after)!=row['patched_sha256']:raise ValueError('Changed final native output: '+key)
        prepared[key]=(path,expand(after.decode(),source,build,root/'src'),restore)
    headers={str(Path(k).relative_to('runtime-recomp/src/game')):output/k for k in prepared if Path(k).suffix in ('.h','.hpp','.inl')}
    header_aliases={str(original):output/key for key,(original,text,restore) in prepared.items() if Path(key).suffix in ('.h','.hpp','.inl')}
    for key,(original,text,restore) in prepared.items():
        def includes(m):
            name=m[1]
            if name in header_aliases:return '#include \"'+str(header_aliases[name])+'\"'
            # The current vehicle layer extends the library-card and import
            # contracts. Preserve its effective header search when relocating UI.
            if name=='runtime_mod_library_ui.inl':return '#include "'+str(build/'generated/custom-vehicles/native/runtime_mod_library_ui.inl')+'"'
            if Path(name).name=='linux_keyring.hpp':return '#include "'+str(root/'src/ps5_online_keyring.hpp')+'"'
            chosen=next((value for relative,value in headers.items() if name==relative or name.endswith('/'+relative)),None)
            if chosen:return '#include "'+str(chosen)+'"'
            return '#include "'+restore.get(name,name)+'"'
        text=INCLUDE.sub(includes,text);target=output/key;target.parent.mkdir(parents=True,exist_ok=True)
        if not target.exists() or target.read_text()!=text:target.write_text(text)
        changes.append(dict(old=str(original),new=str(target),logical=key,sha256=sha(text.encode())))
    (output/'swaps.json').write_text(json.dumps(changes,indent=2)+'\n')
    print('PASS exact final PS5 composition:',len(changes),'source adaptations')
if __name__=='__main__':main()
