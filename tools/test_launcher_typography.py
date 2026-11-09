#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Measure the final launcher font loader (glyph centre and digit height) without a ROM.

Pass the composed final runtime_ui.cpp and runtime_ui_paddock.inl (for example
from the native-rebase tree) plus the DKR-R game source and ImGui directories.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('runtime_ui', type=Path)
parser.add_argument('paddock', type=Path)
parser.add_argument('game', type=Path, help='DKR-R runtime-recomp/src/game with generated/*_font.c')
parser.add_argument('imgui', type=Path)
args = parser.parse_args()

text = args.runtime_ui.read_text()
start = text.index('void LoadLauncherFonts() {')
end = text.index('\n}\n', start) + 3
with tempfile.TemporaryDirectory(prefix='dkrr-typography-') as directory:
    work = Path(directory)
    (work / 'launcher_fonts.inl').write_text(text[start:end])
    (work / 'runtime_ui_paddock.inl').write_text(args.paddock.read_text())
    objects = []
    for face in ('racing_banana', 'jumpman', 'selawik'):
        output = work / (face + '.o')
        subprocess.run(['clang', '-std=c11', '-O1', '-I', str(args.game / 'generated'), '-c',
                        str(args.game / f'generated/{face}_font.c'), '-o', str(output)], check=True)
        objects.append(str(output))
    executable = work / 'typography'
    subprocess.run(['clang++', '-D__PROSPERO__', '-std=c++20', '-O1', '-w',
                    '-I', str(work), '-I', str(args.game), '-I', str(args.imgui),
                    str(root / 'tests/launcher_typography.cpp'), *objects,
                    *[str(args.imgui / name) for name in ('imgui.cpp', 'imgui_draw.cpp', 'imgui_widgets.cpp', 'imgui_tables.cpp')],
                    '-o', str(executable)], check=True)
    subprocess.run([str(executable)], check=True, stderr=subprocess.STDOUT)
