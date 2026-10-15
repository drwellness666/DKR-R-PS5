#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Ensure C++ adapter headers participate in the actual native fingerprint."""
import pathlib,tempfile
from native_build_identity import identify
with tempfile.TemporaryDirectory(prefix='dkrr-native-identity-') as tmp:
 p=pathlib.Path(tmp);root=p/'kit';game=p/'game';sdl=p/'sdl';support=p/'gpu/PS5_Vulkan'
 for name in ('VULKAN_SOURCES.json','tools/link_native_game.sh','tools/native_build_identity.py','tools/generate_native_import_check.py','src/ps5_profile_paths.hpp'):
  f=root/name;f.parent.mkdir(parents=True,exist_ok=True);f.write_text('fixture '+name)
 def current():return identify(root,game,sdl,support,1)
 first=current();assert any(x['path']=='source/ps5_profile_paths.hpp' for x in first['inputs'])
 header=root/'src/ps5_profile_paths.hpp';header.write_text('changed shared profile routing')
 second=current();assert first['build_id']!=second['build_id']
 (root/'src/explanation.md').write_text('documentation is not a native input')
 assert current()['build_id']==second['build_id']
 header.unlink();assert current()['build_id']!=second['build_id']
print('PASS actual native identity: C++ header edits/additions/removal change the build ID; unrelated prose does not')
