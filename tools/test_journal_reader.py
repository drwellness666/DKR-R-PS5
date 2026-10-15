#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Run the actual bounded reader against temporary own files; no console."""
import os,pathlib,subprocess,tempfile
root=pathlib.Path(__file__).resolve().parent.parent
env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1')
with tempfile.TemporaryDirectory(prefix='dkrr-journal-reader-') as tmp:
 p=pathlib.Path(tmp);name='run-12345-000006-7.log';file=p/name
 header=b'DKRR_SESSION v=1 build=integrated-host-test pid=7\n';file.write_bytes(header+b'own-note\n')
 binary=p/'reader'
 subprocess.run(['clang','-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
                 '-DDKR_JOURNAL_DIRECTORY="'+tmp+'"','-DDKR_JOURNAL_NAME="'+name+'"',
                 '-DDKR_JOURNAL_UID='+str(os.geteuid()),'-DDKR_BUILD_ID="integrated-host-test"',
                 str(root/'src/read_own_journal.c'),'-o',str(binary)],check=True)
 def run():return subprocess.run([str(binary)],capture_output=True,env=env)
 r=run();assert r.returncode==0 and header+b'own-note\n' in r.stdout and b'DKRR JOURNAL COPY PASS' in r.stdout
 file.write_bytes(header+b'x'*(66048-len(header)));r=run();assert r.returncode==0
 file.write_bytes(header+b'x'*(66049-len(header)));r=run();assert r.returncode==6 and b'DKRR JOURNAL COPY BEGIN' not in r.stdout
 file.write_bytes(b'foreign-build\n');r=run();assert r.returncode==8 and b'foreign-build' not in r.stdout
 file.unlink();file.symlink_to(root/'src/read_own_journal.c');r=run();assert r.returncode==5
 file.unlink();file.mkdir();r=run();assert r.returncode==6
print('PASS actual exact-build journal reader: own bytes, full bounded crash suffix, oversized/foreign/symlink/directory refusal')
