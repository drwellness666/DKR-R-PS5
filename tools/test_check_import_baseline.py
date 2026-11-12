#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""The import gate must fail for a lost stand-in, warn for a new import, and flag a new console-unresolved import."""
import json,pathlib,subprocess,sys,tempfile
root=pathlib.Path(__file__).resolve().parent.parent
TOOL=str(root/'tools/check_import_baseline.py')
with tempfile.TemporaryDirectory(prefix='dkrr-imports-') as temp:
    work=pathlib.Path(temp)
    (work/'a.c').write_text('#define _GNU_SOURCE\n#include <string.h>\n#include <unistd.h>\nint main(int c,char**v){char b[8];(void)c;return (int)readlink(v[0],b,8)+(strcasestr(v[0],"x")!=0)+(int)getpid();}\n')
    subprocess.run(['clang','-O0','-o',str(work/'a'),str(work/'a.c')],check=True)
    base={'schema':1,'missing_shimmed':['readlink'],'missing_accepted':{'sceFoo':'dormant'},'verified':['getpid','__libc_start_main']}
    path=work/'baseline.json';path.write_text(json.dumps(base))
    run=lambda *a:subprocess.run([sys.executable,TOOL,str(work/'a'),'--baseline',str(path),*a],capture_output=True,text=True)
    first=run();assert first.returncode==1 and 'FAIL: readlink is imported again' in first.stdout,first.stdout
    assert 'WARN: new import not yet checked on the console: strcasestr' in first.stdout,first.stdout
    base['missing_shimmed']=[];path.write_text(json.dumps(base))
    warn=run();assert warn.returncode==0 and 'WARN' in warn.stdout,warn.stdout
    strict=run('--strict');assert strict.returncode==1,'strict mode must fail on an unchecked import'
    accepted=run('--accept-verified');assert accepted.returncode==0 and 'readlink' in json.loads(path.read_text())['verified']
    clean=run('--strict');assert clean.returncode==0 and 'WARN' not in clean.stdout,clean.stdout
    # Console side: unresolved imports the baseline does not know.
    journal=work/'run.log';journal.write_text('DKRR_STEP t=1 sub=imports sceFoo\nDKRR_STEP t=2 sub=imports fsync\nDKRR_STEP t=3 sub=storage x\n')
    flagged=subprocess.run([sys.executable,TOOL,'--baseline',str(path),'--journal',str(journal)],capture_output=True,text=True)
    assert flagged.returncode==1 and 'NEW UNRESOLVED IMPORT on the console: fsync' in flagged.stdout and 'sceFoo' not in flagged.stdout,flagged.stdout
    journal.write_text('DKRR_STEP t=1 sub=imports sceFoo\n')
    assert subprocess.run([sys.executable,TOOL,'--baseline',str(path),'--journal',str(journal)]).returncode==0
# The recorded baseline itself must be coherent.
real=json.loads((root/'docs/IMPORTS_BASELINE.json').read_text())
sets=[set(real['verified']),set(real['missing_accepted']),set(real['missing_shimmed'])]
assert not (sets[0]&sets[1] or sets[0]&sets[2] or sets[1]&sets[2]),'a name must have exactly one classification'
assert {'readlink','symlink','link','fchown','mkstemp','strcasestr'}==sets[2]
text=(root/'src/ps5_missing_libc.c').read_text()
for name in sets[2]:assert f' {name}(' in text or f'*{name}(' in text,name+' is classified as shimmed but not defined'
print('PASS import gate: lost stand-in fails, new import warns/fails strict, console-unresolved import flagged, baseline coherent')
