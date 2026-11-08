#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Keep native packages installable by the released 1.0 in-game updater.

1.0 compiled its package digest against a smaller Mbed TLS context than the
linked library. Its SHA-256 finish step reads the SHA-224 flag from the first
four bytes of the last 64 KiB read block. The digest is correct only when those
package bytes are zero. Later updaters do not depend on this (see
tests/ps5_update_digest_abi.cpp). Trailing newlines in a JSON notice move the
package layout until the condition holds; the JSON content stays identical."""
import argparse,pathlib,sys
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parent))
from build_universal_installer import PUBLIC_FILES
BLOCK=65536;HEADER=96;ROW=192
ADJUSTABLE='assets/licenses/Title-Artwork-Sources.json'

def probe_offset(size):return (size-1)//BLOCK*BLOCK

def compatible_bytes(data):
    o=probe_offset(len(data));return data[o:o+4]==b'\0\0\0\0'

def _bytes_at(app,sizes,offset,count):
    position=HEADER;out=b''
    if offset<HEADER:return None  # header contains non-zero magic; never selected for real packages
    for name in sorted(PUBLIC_FILES):
        position+=ROW
        if offset<position:return None
        end=position+sizes[name]
        if offset<end:
            if offset+count>end:return None
            with (app/name).open('rb') as f:f.seek(offset-position);return f.read(count)
        position=end
    return None

def check(app,extra=0):
    sizes={n:(app/n).stat().st_size for n in PUBLIC_FILES};sizes[ADJUSTABLE]+=extra
    total=HEADER+sum(ROW+s for s in sizes.values())
    o=probe_offset(total)
    if extra and _owner(sizes,o)==ADJUSTABLE:return False
    return _bytes_at(app,sizes,o,4)==b'\0\0\0\0'

def _owner(sizes,offset):
    position=HEADER
    for name in sorted(PUBLIC_FILES):
        position+=ROW
        if position<=offset<position+sizes[name]:return name
        position+=sizes[name]
    return None

def align(app):
    target=app/ADJUSTABLE;text=target.read_bytes()
    if text.rstrip(b'\n')+b'\n'!=text:raise ValueError('Unexpected notice ending')
    for extra in range(0,4*BLOCK):
        if check(app,extra):
            if extra:target.write_bytes(text+b'\n'*extra)
            return extra
    raise ValueError('No 1.0-compatible package layout found')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('app',type=pathlib.Path);a=p.parse_args()
    extra=align(a.app.resolve())
    print(f'PASS 1.0 updater package layout: {extra} trailing newline(s) added to {ADJUSTABLE}')
