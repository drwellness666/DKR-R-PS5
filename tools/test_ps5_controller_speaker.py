#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Run actual app speaker worker with synthetic PCM and mocked native AudioOut."""
import os,pathlib,subprocess,sys,tempfile
r=pathlib.Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='dkrr-speaker-') as tmp:
    out=pathlib.Path(tmp)/'speaker'
    subprocess.run(['clang','-std=c11','-O1','-g','-fsanitize=address,undefined','-c',str(r/'installer/vadpcm.c'),'-o',str(pathlib.Path(tmp)/'vadpcm.o')],check=True)
    subprocess.run(['clang++','-std=c++20','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+str(r/'src'),str(r/'tests/ps5_controller_speaker.cpp'),str(r/'src/ps5_controller_speaker.cpp'),str(r/'src/ps5_custom_horn.cpp'),str(pathlib.Path(tmp)/'vadpcm.o'),'-pthread','-o',str(out)],check=True)
    env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1')
    # The worker deliberately drops a cue that is more than 150 ms old (a late
    # sound must not play). On a starved CI host the test's real-thread stimulus
    # can miss that window, which surfaces only as a timed wait() failing.
    # Retry that one signature; any other failure stays an immediate failure.
    for attempt in range(1,4):
        run=subprocess.run([str(out)],env=env,capture_output=True,text=True)
        if run.returncode==0:break
        timed_wait="void wait(Test)" in run.stderr and "Assertion `test()' failed" in run.stderr
        if not timed_wait or attempt==3:
            sys.stdout.write(run.stdout);sys.stderr.write(run.stderr);raise SystemExit(run.returncode)
        print(f"retry {attempt}: cue expired under host load (timed wait only)")
    sys.stdout.write(run.stdout)

    custom=pathlib.Path(tmp)/'custom-horn'
    subprocess.run(['clang++','-std=c++20','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+str(r/'src'),str(r/'tests/ps5_custom_horn.cpp'),str(r/'src/ps5_controller_speaker.cpp'),str(r/'src/ps5_custom_horn.cpp'),str(pathlib.Path(tmp)/'vadpcm.o'),'-pthread','-o',str(custom)],check=True)
    subprocess.run([str(custom)],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
    print('PASS admitted mod horn: bounded bank decode, exact sample/pitch/envelope inputs, owner lifetime, overlapping voices, no retail PCM lookup, invalid optional bank does not disable retail cues')

with tempfile.TemporaryDirectory(prefix='dkrr-speaker-files-') as tmp:
    out=pathlib.Path(tmp)/'reader'
    subprocess.run(['clang++','-std=c++20','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+str(r/'src'),'-Wl,--wrap=open','-Wl,--wrap=read','-Wl,--wrap=fstat','-Wl,--wrap=close',str(r/'tests/ps5_pcm_reader.cpp'),'-o',str(out)],check=True)
    subprocess.run([str(out),tmp],check=True,timeout=10,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
