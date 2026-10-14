#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Validate native launcher formats using only the Python standard library."""
import pathlib, struct, sys
root=pathlib.Path(__file__).resolve().parent.parent
if len(sys.argv)!=2:raise SystemExit('Usage: validate_title_art.py ABSOLUTE_BUILD_TITLE_ART_DIRECTORY')
folder=pathlib.Path(sys.argv[1])
icon=(folder/'icon0.png').read_bytes()
if icon[:8]!=b'\x89PNG\r\n\x1a\n' or icon[12:16]!=b'IHDR' or struct.unpack_from('>II',icon,16)!=(512,512):
    raise SystemExit('Icon must be a 512x512 PNG')
for name in ('pic0.dds','pic1.dds'):
    data=(folder/name).read_bytes()
    if len(data)!=8294548 or data[:4]!=b'DDS ' or data[84:88]!=b'DX10':
        raise SystemExit(name+': expected exact single-level 3840x2160 BC7 DX10 DDS size')
    if struct.unpack_from('<I',data,4)[0]!=124 or struct.unpack_from('<II',data,12)!=(2160,3840):
        raise SystemExit(name+': invalid header or dimensions')
    if struct.unpack_from('<I',data,28)[0]!=1 or struct.unpack_from('<IIIII',data,128)!=(98,3,0,1,1):
        raise SystemExit(name+': require BC7_UNORM, 2D, one surface, one mip, straight alpha')
if (folder/'pic0.dds').read_bytes()!=(folder/'pic1.dds').read_bytes():
    raise SystemExit('Selection and launch artwork must use the same verified image')
print('PASS: native 512px icon and identical paired 4K BC7 selection/launch backgrounds')
