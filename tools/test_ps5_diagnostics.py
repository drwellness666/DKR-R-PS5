#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Actual diagnostic C tests and contained child faults; no console connection."""
import os,pathlib,signal,subprocess,tempfile
root=pathlib.Path(__file__).resolve().parent.parent
with tempfile.TemporaryDirectory(prefix='dkrr-diagnostics-') as temp:
    work=pathlib.Path(temp);data=work/'data';common=['clang','-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-I',str(root/'src'),'-DDKR_DIAG_DATA_ROOT="'+str(data)+'"',str(root/'src/ps5_diagnostics.c'),str(root/'tests/ps5_diagnostics.c')]
    subprocess.run(common+['-fsanitize=address,undefined','-o',str(work/'checked')],check=True)
    env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1')
    subprocess.run([str(work/'checked')],env=env,check=True)
    subprocess.run(['clang','-std=c11','-O1','-fsanitize=address,undefined','-I',str(root/'src'),'-c',str(root/'src/ps5_diagnostics.c'),'-o',str(work/'diag.o')],check=True)
    subprocess.run(['clang++','-std=c++20','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I',str(root/'src'),str(root/'tests/diagnostics_ui.cpp'),str(work/'diag.o'),'-o',str(work/'ui')],check=True)
    subprocess.run([str(work/'ui')],env=env,check=True)
    subprocess.run(common+['-o',str(work/'fault')],check=True)
    subprocess.run([str(work/'fault'),'missing'],check=True)
    data.mkdir();(data/'diagnostics').symlink_to(work,target_is_directory=True)
    subprocess.run([str(work/'fault'),'missing'],check=True);assert not list(work.glob('run-*.log'))
    (data/'diagnostics').unlink();subprocess.run([str(work/'checked'),'journal'],env=env,check=True)
    logs=list((data/'diagnostics').glob('run-*.log'));assert len(logs)==2
    assert sum(p.read_text().count('DKRR_CAPTURE ') for p in logs)==1
    for mode,expected in [('preserve',0),('fault',-signal.SIGSEGV),('abort',-signal.SIGABRT)]:
        before=set((data/'diagnostics').iterdir());result=subprocess.run([str(work/'fault'),mode],capture_output=True)
        assert result.returncode==expected,(mode,result.returncode,result.stderr)
        after=set((data/'diagnostics').iterdir())-before;assert len(after)==1
        text=next(iter(after)).read_text();assert 'build=integrated-test' in text
        if mode=='preserve':assert 'DKRR_CRASH' not in text and 'DKRR_SESSION_END' in text
        else:
            assert 'DKRR_CRASH' in text and 'stage=0x0000000000000004' in text
            assert 'DKRR_SESSION_END' not in text
            if mode=='fault':assert 'address=0x00000000000000d0' in text
    print('PASS actual journal, symlink/missing refusal, repeated init, preserved handler, SIGSEGV/SIGABRT default termination and crash records')
