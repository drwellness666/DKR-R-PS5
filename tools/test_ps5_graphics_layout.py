#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise the real HDR widgets and changing diagnostic disclosure with ImGui."""
import argparse
import os
import re
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('game_work', type=Path)
a = p.parse_args()
d = a.game_work.resolve() / 'source/DKR-R/runtime-recomp/src/game'
imgui = a.game_work.resolve() / 'source/rt64-game/src/contrib/imgui'

def function(text, name):
    start = text.index(name)
    start = text.rfind('\n', 0, start) + 1
    at = start + re.search(r'\)\s*\{', text[start:]).end() - 1
    depth, end = 1, at + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end] + '\n'

with tempfile.TemporaryDirectory(prefix='dkrr-graphics-layout-') as temporary:
    work = Path(temporary)
    settings = (d / 'runtime_settings_ui.inl').read_text()
    pieces = [settings[:settings.index('// ------------------------------------------------------------ drop-down')]]
    pieces += [function(settings, 'bool SettingsCheck(')]
    pieces += [settings[settings.index('struct SettingsRangeValue'):settings.index('// ------------------------------------------------------------ cards')]]
    pieces += [function(settings, 'bool SettingsDisclosure(')]
    (work / 'settings.inc').write_text('\n'.join(pieces))
    (work / 'details.inc').write_text(function((d / 'runtime_messages_ui.inl').read_text(), 'void DrawTechnicalDetails('))
    source = (d / 'runtime_ui.cpp').read_text()
    start = source.index('void LoadLauncherFonts() {')
    (work / 'upstream_launcher_fonts.inl').write_text(source[start:source.index('\nclass ControlFontScope', start)])
    objects = []
    for face in ('racing_banana', 'jumpman', 'selawik'):
        output = work / (face + '.o')
        subprocess.run(['clang', '-std=c11', '-O1', '-I', str(d / 'generated'), '-c',
                        str(d / f'generated/{face}_font.c'), '-o', str(output)], check=True)
        objects.append(str(output))
    executable = work / 'test'
    subprocess.run(['clang++', '-D__PROSPERO__', '-std=c++20', '-O1', '-g', '-Wall', '-Wextra',
                    '-Wno-nontrivial-memcall', '-fsanitize=address,undefined',
                    '-I'+str(work), '-I'+str(root/'src'), '-I'+str(d), '-I'+str(imgui),
                    str(root/'tests/ps5_graphics_layout.cpp'), *objects,
                    *[str(imgui/name) for name in ('imgui.cpp', 'imgui_draw.cpp', 'imgui_widgets.cpp', 'imgui_tables.cpp')],
                    '-o', str(executable)], check=True)
    subprocess.run([str(executable)], check=True,
                   env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0', UBSAN_OPTIONS='halt_on_error=1'))
