#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise actual CRT overlay geometry with pinned ImGui, without a ROM/GPU."""
import argparse,pathlib,subprocess,tempfile
from compiler_flags import imgui_warnings
r=pathlib.Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('game_work',nargs='?',type=pathlib.Path);p.add_argument('--cache',action='append',type=pathlib.Path,default=[]);a=p.parse_args()
with tempfile.TemporaryDirectory(prefix='dkrr-crt-geometry-') as folder:
 t=pathlib.Path(folder)
 if a.game_work is None:
  a.game_work=t/'inputs';command=['python3',str(r/'tools/prepare_controller_test_sources.py'),str(a.game_work)]
  for cache in a.cache:command+=['--cache',str(cache)]
  subprocess.run(command,check=True)
 source=a.game_work/'source/DKR-R/runtime-recomp/src/game/runtime_crt_overlay.cpp'
 text=source.read_text()
 # Native one-quad repetition relies on the Vulkan descriptor's wrap sampler.
 for expected in ('sampler_desc.addressU = RenderTextureAddressMode::WRAP;',
                  'sampler_desc.addressV = RenderTextureAddressMode::WRAP;',
                  'sampler->vk, texture->imageView,'):
  if text.count(expected)!=1:raise RuntimeError('CRT repeat sampler dependency changed')
 body=text[text.index('    const ImVec2 display = ImGui::GetIO().DisplaySize;',text.index('bool dkr::runtime::crt::draw')):text.index('\nvoid dkr::runtime::crt::release')]
 (t/'crt_draw.inc').write_text(body)
 imgui=a.game_work/'source/rt64-game/src/contrib/imgui'
 for native in (False,True):
  target=t/('native' if native else 'desktop')
  command=['clang++','-std=c++20','-O1','-g','-Wall','-Wextra','-Werror',*imgui_warnings(),'-fsanitize=address,undefined','-fno-omit-frame-pointer','-I'+str(t),'-I'+str(imgui)]
  if native:command+=['-D__PROSPERO__']
  subprocess.run(command+[str(r/'tests/ps5_crt_overlay.cpp'),*[str(imgui/n) for n in ('imgui.cpp','imgui_draw.cpp','imgui_widgets.cpp','imgui_tables.cpp')],'-o',str(target)],check=True)
  subprocess.run([str(target)],check=True,timeout=30)
