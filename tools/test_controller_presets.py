#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Test actual patched inputs and controls/settings functions without ROMs."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('game_work', type=Path, nargs='?')
parser.add_argument('--cache', type=Path, action='append', default=[])
args = parser.parse_args()
prepared = None
if args.game_work is None:
    prepared = tempfile.TemporaryDirectory(prefix='dkrr-controller-test-inputs-')
    args.game_work = Path(prepared.name) / 'pinned'
    command = ['python3', str(root / 'tools/prepare_controller_test_sources.py'), str(args.game_work)]
    for cache in args.cache:
        command.extend(['--cache', str(cache)])
    subprocess.run(command, check=True)
source = args.game_work.resolve() / 'source'
game = source / 'DKR-R/runtime-recomp/src/game'
controls = (game / 'runtime_controls_ui.inl').read_text()
ui = (game / 'runtime_ui.cpp').read_text()

def function(text, name):
    start = text.index(name)
    start = text.rfind('\n', 0, start) + 1
    # These reviewed functions have no braces in string literals/comments.
    begin = text.index('{', start)
    depth = 1
    end = begin + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end] + '\n'

snapshot = controls[controls.index('struct ControlsSnapshot {'):controls.index('\nstruct ControlsPageState')]
snapshot += function(controls, 'TakeControlsSnapshot()')
snapshot += function(controls, 'RestoreControlsSnapshot(')
saved_start = ui.index('        const auto controller_assignments =')
saved_end = ui.index('        // This must be the final record.', saved_start)
sdl = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'sdl2'], text=True))
# Clang renamed diagnostics in later releases. Probe only the known ImGui
# compatibility suppressions; keep -Werror for all other diagnostics.
imgui_warnings = []
for warning in ['-Wno-nontrivial-memcall', '-Wno-nontrivial-memaccess',
                '-Wno-uninitialized-const-pointer', '-Wno-uninitialized-const-reference']:
    probe = subprocess.run(['clang++', '-x', 'c++', '-fsyntax-only', '-Werror', warning, '-'],
                           input='', text=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    if probe.returncode == 0:
        imgui_warnings.append(warning)
env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy',
           ASAN_OPTIONS='detect_leaks=0', UBSAN_OPTIONS='halt_on_error=1')
with tempfile.TemporaryDirectory(prefix='dkrr-controller-presets-') as temp:
    temp = Path(temp)
    (temp / 'controls_snapshot.inc').write_text(snapshot)
    (temp / 'binding_settings.inc').write_text(function(ui, 'LoadPlayerBindingSetting('))
    (temp / 'save_bindings.inc').write_text(ui[saved_start:saved_end])
    out = temp / 'controller-presets'
    subprocess.run(['clang++', '-std=c++20', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-DDKR_RUNTIME_HAS_RT64=1',
                    '-I', str(game), '-I', str(temp), '-I', str(root / 'src'), str(root / 'tests/controller_presets.cpp'),
                    str(game / 'runtime_input.cpp'), *sdl, '-pthread', '-o', str(out)], check=True)
    subprocess.run([str(out), str(temp / 'shared-profile-fixture')], env=env, check=True)
    # Same pinned authored UI source, including Play selector/sidebar and the
    # one-column Settings/Save grids. No additional preparation or ROM needed.
    for name in ('test_console_play_navigation.py','test_console_menu_navigation.py'):
        subprocess.run(['python3',str(root/'tools'/name),str(args.game_work)],env=env,check=True)
    (temp / 'controller_capture.inc').write_text(function(ui, 'bool HandleInputCaptureEvent(SDL_Event* event) {'))
    out = temp / 'controller-capture'
    subprocess.run(['clang++', '-std=c++20', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-DDKR_RUNTIME_HAS_RT64=1',
                    '-I', str(game), '-I', str(temp), str(root / 'tests/controller_capture.cpp'),
                    str(game / 'runtime_input.cpp'), *sdl, '-pthread', '-o', str(out)], check=True)
    subprocess.run([str(out)], env=env, check=True)
    (temp / 'controls_state.inc').write_text(
        controls[controls.index('struct ControlsSnapshot {'):controls.index('// SDL\'s binding names')])
    (temp / 'controller_capture_begin.inc').write_text(function(controls, 'BeginControllerCapture('))
    (temp / 'friendly_binding.inc').write_text(function(controls, 'FriendlyBinding('))
    (temp / 'controls_preset.inc').write_text(function(controls, 'DrawControlsPreset('))
    (temp / 'controls_bindings.inc').write_text(function(controls, 'DrawControlsBindings('))
    imgui = source / 'rt64-game/src/contrib/imgui'
    out = temp / 'controller-navigation'
    subprocess.run(['clang++', '-std=c++20', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    *imgui_warnings,
                    '-fsanitize=address,undefined', '-DDKR_RUNTIME_HAS_RT64=1',
                    '-I', str(game), '-I', str(temp), '-I', str(imgui),
                    str(root / 'tests/controller_controls_navigation.cpp'), str(game / 'runtime_input.cpp'),
                    *[str(imgui / file) for file in ['imgui.cpp','imgui_draw.cpp','imgui_widgets.cpp','imgui_tables.cpp']],
                    *sdl, '-pthread', '-o', str(out)], check=True)
    subprocess.run([str(out)], env=env, check=True)
