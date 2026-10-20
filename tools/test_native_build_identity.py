#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Ensure C++ adapter headers participate in the actual native fingerprint."""
import pathlib,tempfile
from native_build_identity import identify
with tempfile.TemporaryDirectory(prefix='dkrr-native-identity-') as tmp:
 p=pathlib.Path(tmp);root=p/'kit';game=p/'game';sdl=p/'sdl';support=p/'gpu/PS5_Vulkan'
 for name in ('VULKAN_SOURCES.json','PORT_RELEASE.json','GAME_SOURCES.json','tools/compose_upstream_release.py','tools/prepare_camera_kernels.py','tools/StagePS5Release.cmake','tools/link_native_game.sh','tools/native_build_identity.py','tools/generate_native_import_check.py','tools/build_universal_installer.py','tools/StagePS5HDR.cmake','tools/build_hdr_shaders.py','tools/apply_ps5_hdr_driver.py','vendor/ps5-sdk-source.tar.gz','src/ps5_profile_paths.hpp'):
  f=root/name;f.parent.mkdir(parents=True,exist_ok=True);f.write_text('fixture '+name)
 def current():return identify(root,game,sdl,support,1)
 first=current();assert any(x['path']=='source/ps5_profile_paths.hpp' for x in first['inputs'])
 header=root/'src/ps5_profile_paths.hpp';header.write_text('changed shared profile routing')
 second=current();assert first['build_id']!=second['build_id']
 release=root/'PORT_RELEASE.json';release.write_text('different installer release sequence')
 assert current()['build_id']!=second['build_id']
 release.write_text('fixture PORT_RELEASE.json')
 (root/'src/explanation.md').write_text('documentation is not a native input')
 assert current()['build_id']==second['build_id']
 (root/'installer').mkdir();(root/'installer/files.c').write_text('native update filesystem implementation')
 assert current()['build_id']!=second['build_id']
 (root/'installer/files.c').unlink()
 header.unlink();assert current()['build_id']!=second['build_id']
 before=current()['build_id'];(root/'src/ps5_hdr.frag').write_text('authored HDR shader');assert current()['build_id']!=before
 before=current()['build_id'];(root/'tools/StagePS5HDR.cmake').write_text('changed HDR composition');assert current()['build_id']!=before
 for relative in ('licenses/COMPONENT-NOTICES.txt','licenses/COMPONENT-NOTICES.json',
                  'licenses/PLATFORM-NOTICE-SOURCES.json','THIRD_PARTY.md',
                  'homebrew/DKR-R-Game/ARTWORK.md','TITLE_ART_SOURCES.json',
                  'tools/prepare_title_art.sh','tools/prepare_upstream_title_art.py'):
  f=root/relative;f.parent.mkdir(parents=True,exist_ok=True)
  before=current()['build_id'];f.write_text('staged notice/art input');assert current()['build_id']!=before
  before=current()['build_id'];f.write_text('changed staged notice/art input');assert current()['build_id']!=before
  before=current()['build_id'];f.unlink();assert current()['build_id']!=before
 for f in (game/'game-ps5/assets/filters/scanlines-soft.png',game/'private/title-art-paired/icon0.png',
           support.parent/'native-title/libc.prx',support.parent/'native-title/ps5-native-tool',support/'sce_sys/param-radv.json'):
  f.parent.mkdir(parents=True,exist_ok=True)
  before=current()['build_id'];f.write_bytes(b'actual staged input');assert current()['build_id']!=before
  before=current()['build_id'];f.write_bytes(b'changed staged input');assert current()['build_id']!=before
  before=current()['build_id'];f.unlink();assert current()['build_id']!=before
print('PASS actual native identity: adapter, shader, notice, title-art, staged asset and linked runtime/tool changes affect the fingerprint; unrelated prose does not')
