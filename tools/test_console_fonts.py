#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Verify and bake DKR-R's unchanged upstream font loader without a ROM."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tarfile
import tempfile

root = Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('game_work', type=Path, nargs='?')
parser.add_argument('--cache', type=Path, action='append', default=[])
args = parser.parse_args()


def launcher_function(text):
    start = text.index('void LoadLauncherFonts() {')
    return text[start:text.index('\nclass ControlFontScope', start)]


with tempfile.TemporaryDirectory(prefix='dkrr-upstream-fonts-') as directory:
    work = Path(directory)
    game_work = args.game_work
    if game_work is None:
        game_work = work / 'game'
        command = ['python3', str(root / 'tools/prepare_controller_test_sources.py'), str(game_work)]
        for cache in args.cache:
            command += ['--cache', str(cache)]
        subprocess.run(command, check=True)

    source = game_work.resolve() / 'source'
    game = source / 'DKR-R/runtime-recomp/src/game'
    imgui = source / 'rt64-game/src/contrib/imgui'
    spec = next(row for row in json.loads((root / 'GAME_SOURCES.json').read_text())['sources']
                if row['name'] == 'DKR-R')
    caches = [game_work, game_work / 'downloads', *args.cache, root / 'vendor']
    archive = next((cache / spec['file'] for cache in caches if (cache / spec['file']).is_file()), None)
    if archive is None or hashlib.sha256(archive.read_bytes()).hexdigest() != spec['sha256']:
        raise SystemExit('Missing exact pinned upstream archive for font comparison')
    with tarfile.open(archive) as upstream:
        members = {member.name.split('/', 1)[1]: member for member in upstream.getmembers()
                   if member.isfile() and '/' in member.name}

        def original(name):
            # The 1.0.7 release capsule canonically stores text with LF;
            # this does not alter a single byte of the embedded font arrays.
            return upstream.extractfile(members['runtime-recomp/src/game/' + name]).read().replace(b'\r\n', b'\n')

        actual_loader = launcher_function((game / 'runtime_ui.cpp').read_text())
        if actual_loader != launcher_function(original('runtime_ui.cpp').decode()):
            raise SystemExit('Production font loader differs from upstream DKR-R')
        console_layout_flag = '''
// Console pages use one vertical flow so D-pad movement follows the reading order.
#if defined(__PROSPERO__)
constexpr bool kConsoleMenuLayout = true;
#else
constexpr bool kConsoleMenuLayout = false;
#endif
'''
        paddock = (game / 'runtime_ui_paddock.inl').read_text()
        if paddock.replace(console_layout_flag, '') != original('runtime_ui_paddock.inl').decode():
            raise SystemExit('Typography or font metrics differ from upstream DKR-R')
        for face in ('racing_banana', 'jumpman', 'selawik'):
            for suffix in ('c', 'h'):
                name = f'generated/{face}_font.{suffix}'
                if (game / name).read_bytes().replace(b'\r\n', b'\n') != original(name):
                    raise SystemExit('Font bytes differ from upstream: ' + name)

    (work / 'upstream_launcher_fonts.inl').write_text(actual_loader)
    objects = []
    for face in ('racing_banana', 'jumpman', 'selawik'):
        output = work / (face + '.o')
        subprocess.run(['clang', '-std=c11', '-O1', '-I', str(game / 'generated'), '-c',
                        str(game / f'generated/{face}_font.c'), '-o', str(output)], check=True)
        objects.append(str(output))
    executable = work / 'fonts'
    subprocess.run(['clang++', '-D__PROSPERO__', '-std=c++20', '-O1', '-Wno-nontrivial-memcall',
                    '-I', str(work), '-I', str(game), '-I', str(imgui),
                    str(root / 'tests/console_fonts.cpp'), *objects,
                    *[str(imgui / name) for name in ('imgui.cpp', 'imgui_draw.cpp', 'imgui_widgets.cpp', 'imgui_tables.cpp')],
                    '-o', str(executable)], check=True)
    subprocess.run([str(executable)], check=True)
    print('PASS exact upstream font function, font data, typography metrics and fallback policy')
