#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Compile exact final launcher-first fragments with external-owner test doubles.

These are host logic checks, not Vulkan/display or PS5 gameplay validation.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('work', type=Path, nargs='?', help='optional full native update work; CI uses the hash-checked PS5 patch')
args = parser.parse_args()
work = args.work.resolve() if args.work else ROOT / 'work'
work.mkdir(exist_ok=True)
final = work / 'native-rebase/runtime-recomp/src/game'


def block(text, marker):
    start = text.index(marker)
    brace = text.index('{', start)
    at, depth = brace + 1, 1
    while depth:
        depth += (text[at] == '{') - (text[at] == '}')
        at += 1
    return text[start:at]


main = (ROOT / 'src/ps5_game_main.cpp').read_text()
if args.work:
    game = (final / 'game_main.cpp').read_text()
    ui = (final / 'runtime_ui.cpp').read_text()
    play = (final / 'runtime_play_ui.inl').read_text()
else:
    # Existing ROM-free CI tests use the reviewed patch when the full native
    # composition (which needs owned ROM inputs) is unavailable. Never execute
    # the old '-' side; include complete added blocks and unchanged context.
    raw = (ROOT / 'patches/dkrr-109-native-final.patch').read_bytes()
    manifest = json.loads((ROOT / 'patches/dkrr-109-final-manifest.json').read_text())
    if hashlib.sha256(raw).hexdigest() != manifest['patch_sha256']:
        raise SystemExit('Changed final native patch')
    records = list(re.finditer(r'^--- a/([^\n]+)\n', raw.decode(), re.M))
    pieces = {}
    text = raw.decode()
    for index, record in enumerate(records):
        section = text[record.end():records[index + 1].start() if index + 1 < len(records) else len(text)]
        pieces[record[1]] = '\n'.join(line[1:] for line in section.splitlines()
                                      if line.startswith((' ', '+')) and not line.startswith('+++'))
    game = pieces['runtime-recomp/src/game/game_main.cpp']
    ui = pieces['runtime-recomp/src/game/runtime_ui.cpp']
    play = pieces['runtime-recomp/src/game/runtime_play_ui.inl']
argv = re.search(r'        char name\[\].*?\n        char \*args\[\].*?;\n', main, re.S)
if argv is None or '--rom' in argv[0] or 'dkrr_game_main(3,args)' not in main:
    raise SystemExit('PS5 no longer uses the launcher-first argument path')
profile_marker = '    {\n        // The PS5 title opens the launcher first'
profile = block(game, profile_marker)
assert profile.count('std::filesystem::create_directories(profile_directory)') == 1
profile = profile.replace('std::filesystem::create_directories(profile_directory)',
                          'fixture_create_directory(profile_directory)')
handoff = block(ui, '        if(dkrr::updates::close_requested()) {')
return_start = play.index('    DrawPlaySaveSummary(width,live);')
return_dialog = block(play[return_start:], '    if(live) {')
# Static regression checks cover the separate renderer handoff and preference
# gates that cannot run with host owner doubles.
assert 'AutomaticStart automatic_start(false,' in ui
assert '#if !defined(__PROSPERO__)\n    DrawSkipLauncherSetting(width);\n#endif' in play
if args.work:
    assert game.index('runtime_thread.join();') < game.index('[boot][stop] game stopped; returning to launcher')
    assert ui.index('native_frontend.close()', ui.index('while (running)')) < ui.index('SDL_DestroyRenderer(renderer);', ui.index('while (running)'))

with tempfile.TemporaryDirectory(prefix='launcher-first-', dir=work) as folder:
    tmp = Path(folder)
    for name, text in (('argv', argv[0]), ('profile', profile), ('update', handoff), ('return', return_dialog)):
        (tmp / f'launcher_{name}.inc').write_text(text)
    exe = tmp / 'test'
    subprocess.run(['clang++', '-std=c++20', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-I' + str(tmp), '-I' + str(ROOT / 'src'),
                    str(ROOT / 'tests/ps5_launcher_first.cpp'), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True, timeout=20,
                   env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=1', 'UBSAN_OPTIONS': 'halt_on_error=1'})
