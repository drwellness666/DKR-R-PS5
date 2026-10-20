#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Compile the actual native renderer rate getter, with no GPU/ROM/console."""
import argparse,os,pathlib,subprocess,tempfile
root=pathlib.Path(__file__).resolve().parent.parent
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('game_work',type=pathlib.Path,nargs='?')
p.add_argument('--cache',type=pathlib.Path,action='append',default=[])
a=p.parse_args();prepared=None
if a.game_work is None:
    prepared=tempfile.TemporaryDirectory(prefix='dkrr-rate-inputs-');a.game_work=pathlib.Path(prepared.name)/'game'
    command=['python3',str(root/'tools/prepare_controller_test_sources.py'),str(a.game_work)]
    for cache in a.cache:command+=['--cache',str(cache)]
    subprocess.run(command,check=True)
game=a.game_work/'source/DKR-R/runtime-recomp/src/game'
source=(game/'rt64_renderer.cpp').read_text()
start=source.index('int DetectDisplayRate() {');end=source.index('\nvoid ApplyConfig(',start)
with tempfile.TemporaryDirectory(prefix='dkrr-native-display-rate-') as folder:
    folder=pathlib.Path(folder)
    # Also exercise the actual lower-level RT64 window getter. The runtime's
    # target alone cannot detect a second 60-Hz clamp in the present queue.
    import tarfile
    rt=folder/'rt64';rt.mkdir()
    import json
    manifest=json.loads((root/'patches/rt64-patch-manifest.json').read_text())
    with tarfile.open(root/'vendor/RT64.tar.gz') as archive:
        for name in manifest['files']:
            member=next(m for m in archive if m.isfile() and (m.name==name or m.name.endswith('/'+name)))
            dest=rt/name;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(archive.extractfile(member).read())
    subprocess.run(['python3',str(root/'tools/apply_rt64_patch.py'),str(rt)],check=True)
    window=(rt/'src/hle/rt64_application_window.cpp').read_text()
    getter=window.index('    uint32_t ApplicationWindow::getRefreshRate() const {')
    getter_end=window.index('\n    }',getter)+6
    (folder/'native_window_rate.inc').write_text(window[getter:getter_end])
    (folder/'detect_display_rate.inc').write_text(source[start:end]);out=folder/'test'
    ui=(game/'runtime_ui.cpp').read_text()
    default_start=ui.index('#if defined(DKR_IOS) || defined(__PROSPERO__)\nRefreshRate g_modern_refresh_mode')
    default_end=ui.index('#if defined(DKR_IOS)\nResolution g_modern_resolution',default_start)
    (folder/'native_rate_defaults.inc').write_text(ui[default_start:default_end])
    block_start=source.index('    if (dkr::runtime::enhancements::modern_presentation_enabled()) {',source.index('void ApplyConfig('))
    block_end=source.index('    application.userConfig.displayBuffering',block_start)
    (folder/'apply_native_rate.inc').write_text(source[block_start:block_end])
    subprocess.run(['clang++','-D__PROSPERO__','-std=c++20','-O1','-g','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined','-include','initializer_list','-I',str(root/'src'),
                    '-I',str(folder),'-I',str(game),str(root/'tests/native_display_rate.cpp'),'-o',str(out)],check=True)
    subprocess.run([str(out)],env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'),check=True)
if prepared is not None:prepared.cleanup()
