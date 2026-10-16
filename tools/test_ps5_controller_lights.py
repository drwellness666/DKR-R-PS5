#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise actual native light policy and the pinned SDL light-bar transport."""
import argparse,hashlib,os,pathlib,subprocess,tarfile,tempfile
root=pathlib.Path(__file__).resolve().parent.parent
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('game_work',type=pathlib.Path,nargs='?')
p.add_argument('sdl_work',type=pathlib.Path,nargs='?')
p.add_argument('--cache',type=pathlib.Path,action='append',default=[])
a=p.parse_args()
prepared=None
if (a.game_work is None)!=(a.sdl_work is None):p.error('Supply both prepared work paths or neither')
if a.game_work is None:
    prepared=tempfile.TemporaryDirectory(prefix='dkrr-light-inputs-')
    folder=pathlib.Path(prepared.name);a.game_work=folder/'game';a.sdl_work=folder/'sdl'
    command=['python3',str(root/'tools/prepare_controller_test_sources.py'),str(a.game_work)]
    for cache in a.cache:command+=['--cache',str(cache)]
    subprocess.run(command,check=True)
    archive=root/'vendor/ps5-SDL.tar.gz'
    expected=[line.split('  ')[0] for line in (root/'vendor/SHA256SUMS').read_text().splitlines()
              if line.endswith('  ps5-SDL.tar.gz')]
    if len(expected)!=1 or hashlib.sha256(archive.read_bytes()).hexdigest()!=expected[0]:
        raise SystemExit('Pinned SDL source checksum mismatch')
    import json
    files=json.loads((root/'patches/sdl-patch-manifest.json').read_text())['files']
    with tarfile.open(archive) as tar:
        for name in files:
            entries=[m for m in tar if m.isfile() and (m.name==name or m.name.endswith('/'+name))]
            if len(entries)!=1:raise SystemExit('Missing/duplicate pinned SDL source')
            out=a.sdl_work/'src/ps5-SDL'/name;out.parent.mkdir(parents=True,exist_ok=True)
            out.write_bytes(tar.extractfile(entries[0]).read())
    subprocess.run(['python3',str(root/'tools/apply_sdl_patch.py'),str(a.sdl_work/'src/ps5-SDL')],check=True)

game=a.game_work/'source/DKR-R/runtime-recomp/src/game'
text=(a.sdl_work/'src/ps5-SDL/src/joystick/ps5/SDL_ps5joystick.c').read_text()
functions=[text[text.index('typedef struct PS5_ItemEffect'):text.index('_Static_assert(sizeof(PS5_ItemEffect)')]]
for name in ('PS5_JoystickSetLED','PS5_JoystickSendEffect'):
    start=text.index(name+'(');start=text.rfind('static int',0,start)
    begin=text.index('{',start);depth=1;end=begin+1
    while depth:
        depth+=(text[end]=='{')-(text[end]=='}');end+=1
    functions.append(text[start:end])

with tempfile.TemporaryDirectory(prefix='dkrr-controller-lights-') as folder:
    folder=pathlib.Path(folder);(folder/'ps5_led.inc').write_text('\n'.join(functions))
    binary=folder/'lights'
    subprocess.run(['clang++','-std=c++20','-O1','-g','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined','-I',str(root/'src'),'-I',str(game),
                    '-I',str(folder),str(root/'tests/ps5_controller_lights.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'),check=True)
if prepared is not None:prepared.cleanup()
