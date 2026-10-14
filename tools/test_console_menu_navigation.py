#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise real ImGui controller navigation through the prepared console field layout."""
import argparse, pathlib, subprocess, tempfile
root=pathlib.Path(__file__).resolve().parent.parent
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('game_work',type=pathlib.Path)
a=parser.parse_args();source=a.game_work.resolve()/'source'
text=(source/'DKR-R/runtime-recomp/src/game/runtime_settings_ui.inl').read_text()
start=text.index('struct SettingsColumns {');end=text.index('\n};',start)+3
layout=text[start:end]
if 'single(one_column || kConsoleMenuLayout)' not in layout:raise SystemExit('Prepared console field patch is missing')
imgui=source/'rt64-game/src/contrib/imgui'
saves=(source/'DKR-R/runtime-recomp/src/game/runtime_saves_ui.inl').read_text()
start=saves.index('struct SavesGrid {');end=saves.index('\n};',start)+3
saves_layout=saves[start:end]
if 'per_row = kConsoleMenuLayout ? 1' not in saves_layout:raise SystemExit('Prepared console save grid patch is missing')
# Adapt only the fixture's constructor/Finish call; all field geometry is upstream.
saves_fixture='static void PaddockGap(float n){ImGui::Dummy({0,n});}\n'+saves_layout+'''
struct SettingsColumns: SavesGrid {
 SettingsColumns(float w,bool):SavesGrid(w,1200.0f){}
 void Finish(){SavesGrid::Finish(1000.0f);}
};
'''
for name,definition in [('settings',layout),('save-editor',saves_fixture)]:
 with tempfile.TemporaryDirectory(prefix='dkrr-ui-nav-') as tmp:
  tmp=pathlib.Path(tmp);(tmp/'settings_columns.inc').write_text('constexpr bool kConsoleMenuLayout=true;\n'+definition)
  out=tmp/'navigation'
  subprocess.run(['clang++','-std=c++20','-O1','-Wall','-Wextra','-Werror','-Wno-nontrivial-memcall','-Wno-uninitialized-const-pointer','-I',str(tmp),'-I',str(imgui),str(root/'tests/console_menu_navigation.cpp'),*[str(imgui/file) for file in ['imgui.cpp','imgui_draw.cpp','imgui_widgets.cpp','imgui_tables.cpp']],'-o',str(out)],check=True)
  subprocess.run([str(out)],check=True)
  print('Checked actual console layout:',name)
