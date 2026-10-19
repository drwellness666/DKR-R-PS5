#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise the native storage picker and its actual ImGui modal without ROMs."""
import argparse,pathlib,subprocess,tempfile
from compiler_flags import imgui_warnings
r=pathlib.Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('game_work',nargs='?',type=pathlib.Path);p.add_argument('--cache',action='append',type=pathlib.Path,default=[]);a=p.parse_args()
with tempfile.TemporaryDirectory(prefix='dkrr-picker-') as folder:
 t=pathlib.Path(folder)
 flags=['clang++','-std=c++20','-O1','-g','-pthread','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-I'+str(r/'src')]
 subprocess.run(flags+[str(r/'tests/ps5_file_picker.cpp'),str(r/'src/ps5_file_picker.cpp'),'-Wl,--wrap=readdir,--wrap=closedir,--wrap=lstat','-o',str(t/'core')],check=True)
 subprocess.run([str(t/'core'),str(t/'storage')],check=True,timeout=30)
 if a.game_work is None:
  a.game_work=t/'inputs';cmd=['python3',str(r/'tools/prepare_controller_test_sources.py'),str(a.game_work)]
  for cache in a.cache:cmd+=['--cache',str(cache)]
  subprocess.run(cmd,check=True)
 source=a.game_work/'source/DKR-R/runtime-recomp/src/game/runtime_ui.cpp'
 text=source.read_text()
 helper=text[text.index('bool ChooseFileForAction('):text.index('bool SelectRomWithDialog(')]
 jobs=text[text.index('std::mutex g_dialog_job_mutex;'):text.index('// A folder picker, for the dialog-job thread')]
 # Deterministic host fault at std::jthread construction; all production
 # queue logic stays unchanged, and successful starts still use std::jthread.
 if jobs.count('std::jthread') != 2:raise RuntimeError('Dialog thread test seam changed')
 action_jobs=jobs.replace('std::jthread','FixtureDialogThread')
 (t/'file_actions.inc').write_text('bool StartDialogJob(std::function<std::function<void()>()> job);\n'+helper+action_jobs)
 platform=(source.parent/'runtime_platform.cpp').read_text()
 begin=platform.index('bool dkr::runtime::platform::export_controller_mappings(')
 end=platform.index('\ndkr::runtime::controllers::DesiredAssignments',begin)
 (t/'file_exports.inc').write_text(platform[begin:end])
 subprocess.run(flags+['-DDKR_RUNTIME_HAS_RT64=1','-D__PROSPERO__','-I'+str(t),str(r/'tests/ps5_file_exports.cpp'),'-o',str(t/'exports')],check=True)
 subprocess.run([str(t/'exports'),str(t/'export-storage')],check=True,timeout=30)
 nfd=a.game_work/'source/rt64-game/src/contrib/nativefiledialog-extended/src/include'
 subprocess.run(flags+['-D__PROSPERO__','-I'+str(t),'-I'+str(nfd),str(r/'tests/ps5_file_actions.cpp'),'-o',str(t/'actions')],check=True)
 subprocess.run([str(t/'actions')],check=True,timeout=30)
 subprocess.run(['clang','-std=c11','-D_POSIX_C_SOURCE=200809L','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+str(nfd),'-c',str(r/'src/ps5_nfd.c'),'-o',str(t/'nfd.o')],check=True)
 toggle=text[text.index('void dkr::runtime::ui::toggle_overlay() {'):text.index('bool dkr::runtime::ui::overlay_visible() {')]
 (t/'overlay_toggle.inc').write_text(toggle)
 imgui=a.game_work/'source/rt64-game/src/contrib/imgui' 
 subprocess.run(flags+imgui_warnings()+['-I'+str(t),'-I'+str(nfd),'-I'+str(imgui),str(r/'tests/ps5_file_picker_ui.cpp'),str(r/'src/ps5_file_picker.cpp'),str(t/'nfd.o'),*[str(imgui/n) for n in ('imgui.cpp','imgui_draw.cpp','imgui_widgets.cpp','imgui_tables.cpp')],'-o',str(t/'ui')],check=True)
 subprocess.run([str(t/'ui')],check=True,timeout=30)
