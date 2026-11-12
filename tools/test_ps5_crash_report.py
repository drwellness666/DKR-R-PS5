#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""A real contained fault must come back as function and line; the FTP tool must stay read-only."""
import os,pathlib,subprocess,sys,tempfile
root=pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0,str(root/'tools'))
from ps5_crash_report import parse_journal,report
from pull_console_debug import ReadOnlyFTP
with tempfile.TemporaryDirectory(prefix='dkrr-crash-report-') as temp:
    work=pathlib.Path(temp);data=work/'data'
    binary=work/'fault'
    subprocess.run(['clang','-std=c11','-O0','-g','-fno-omit-frame-pointer','-Wall','-Wextra','-Werror','-I',str(root/'src'),
                    '-DDKR_DIAG_DATA_ROOT="'+str(data)+'"',str(root/'src/ps5_diagnostics.c'),str(root/'tests/ps5_diagnostics.c'),'-o',str(binary)],check=True)
    data.mkdir()
    subprocess.run([str(binary),'fault'],capture_output=True)
    journal=next((data/'diagnostics').glob('run-*.log'))
    symbols=work/'symbols'
    subprocess.run([sys.executable,str(root/'tools/archive_symbols.py'),str(binary),'integrated-test',str(symbols)],check=False,capture_output=True)
    # The host test program has no _start symbol of its own that we control;
    # the archive tool insists on one, as the shipped title records &_start.
    (symbols/'integrated-test').mkdir(parents=True,exist_ok=True);(symbols/'integrated-test/llvm-pie.elf').write_bytes(binary.read_bytes())
    text,facts=report(journal.read_text(),symbols,entry_symbol='main')
    print(text)
    assert facts['crash'] and facts['crash']['signal']==11 and facts['crash']['address']==0xd0
    assert not facts['ended'],'a crashed run has no session end'
    assert 'at main (' in text and 'tests/ps5_diagnostics.c' in text,'RIP must resolve to the faulting function and file'
    assert 'last step before the end: [test] before-fault second line' in text
    assert 'NO session end' in text
    # An unresolved import: the platform leaves it at address 0 and the first call jumps there.
    existing=set((data/'diagnostics').glob('run-*.log'))
    subprocess.run([str(binary),'nullcall'],capture_output=True)
    nulljournal=next(iter(set((data/'diagnostics').glob('run-*.log'))-existing))
    nulltext,nullfacts=report(nulljournal.read_text(),symbols,entry_symbol='main')
    print(nulltext)
    assert nullfacts['crash']['rip']==0 and nullfacts['nullcall'] is not None
    assert 'NULL CALL' in nulltext and 'called from main (' in nulltext,'the call site must be named'
    assert 'UNRESOLVED IMPORT (by the call instruction): dkrr_missing_import' in nulltext,'the import must be named'
    # No symbols archived: still a usable report, no invented names.
    plain,_=report(journal.read_text(),work/'nothing',entry_symbol='main')
    assert 'symbols : none archived' in plain and 'at main' not in plain
    # A clean run is reported as clean.
    clean=parse_journal('DKRR_SESSION v=1 build=x pid=1 entry=0x10\nDKRR_STEP t=5 sub=a b\nDKRR_SESSION_END\n')
    assert clean['ended'] and clean['crash'] is None and clean['steps']==[(5,'a','b')]
# The console tool may only read.
ftp=ReadOnlyFTP()
for verb in ('STOR a','DELE a','RNFR a','MKD a','SITE CHMOD 777 a','APPE a','RMD a'):
    try:ftp.putline(verb)
    except RuntimeError as error:assert 'read-only' in str(error)
    else:raise AssertionError(verb+' was not refused')
print('PASS crash journal -> function:line with a real fault, honest report without symbols, read-only console tool')
