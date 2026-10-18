#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build a deterministic, ROM-free data package for the in-game installer."""
import argparse,hashlib,json,pathlib,re,struct
from build_universal_installer import PUBLIC_FILES,release_files
from build_native_game_bundle_updater import inventory,inventory_hash
from verify_component_notices import verify_app

def build(app,receipt,sequence,output):
    if not 1<=sequence<=2147483647 or output.exists() or output.is_symlink():raise ValueError('Use a new package path and valid sequence')
    record=json.loads(receipt.read_text());files=inventory(app)
    if record['files']!=files or record['app_manifest_sha256']!=inventory_hash(files):raise ValueError('Native application differs from its receipt')
    reviewed=release_files(app,strict=True);identity=record['build_id'].encode()
    if not re.fullmatch(rb'integrated-[a-f0-9]{24}',identity):raise ValueError('Invalid native identity')
    output.parent.mkdir(parents=True,exist_ok=True)
    try:
        with output.open('xb') as stream:
            stream.write(b'DKRRPK2\n'+struct.pack('<II',sequence,len(reviewed))+identity.ljust(80,b'\0'))
            for name in sorted(PUBLIC_FILES):
                meta=reviewed[name];data=(app/name).read_bytes();path=name.encode('ascii')
                if len(path)>=160 or len(data)!=meta['size'] or hashlib.sha256(data).hexdigest()!=meta['sha256']:raise ValueError('Package input changed')
                stream.write(path.ljust(160,b'\0')+struct.pack('<QI',len(data),meta['mode'])+bytes.fromhex(meta['sha1']))
                stream.write(data)
        if not 64<=output.stat().st_size<=128*1024*1024:raise ValueError('Unexpected native package size')
    except BaseException:
        output.unlink(missing_ok=True);raise
    return dict(size=output.stat().st_size,sha256=hashlib.sha256(output.read_bytes()).hexdigest(),files=len(reviewed),roms_embedded=False,original_pcm_embedded=False)

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for n in ('app','receipt','output'):p.add_argument(n,type=pathlib.Path)
    p.add_argument('--sequence',required=True,type=int);a=p.parse_args()
    try:
        # The low-level writer also serves synthetic transaction fixtures.
        # The standalone release command must enforce the same notice check
        # as the universal packager before producing an update download.
        verify_app(pathlib.Path(__file__).resolve().parents[1],a.app)
        result=build(a.app,a.receipt,a.sequence,a.output)
    except (OSError,ValueError,KeyError) as error:p.error(str(error))
    print(json.dumps(result,indent=2))
