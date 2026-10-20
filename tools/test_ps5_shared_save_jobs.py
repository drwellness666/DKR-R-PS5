#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise the actual 1.0.7 save-job boundary with a thread-start fault."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
root=Path(__file__).resolve().parents[1]
source=Path(sys.argv[1])/'source/DKR-R/runtime-recomp/src/game/shared_save_jobs.inl'
text=source.read_text()
assert text.count('std::async(')==1
with tempfile.TemporaryDirectory(prefix='dkrr-shared-save-jobs-') as folder:
    work=Path(folder)
    (work/'shared_save_jobs.inc').write_text(text.replace('std::async(', 'fixture_async('))
    out=work/'test'
    subprocess.run(['clang++','-std=c++20','-O1','-g','-pthread','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined','-I'+str(work),str(root/'tests/ps5_shared_save_jobs.cpp'),'-o',str(out)],check=True)
    subprocess.run([str(out)],check=True,timeout=20,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
print('PASS actual shared-save job: thread-start refusal/retry, duplicate job refusal, successful UI completion, worker exception and rejected save; no console claim')
