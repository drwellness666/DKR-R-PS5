#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Compile one bounded own-journal reader, never connect or deploy automatically."""
import argparse,hashlib,json,pathlib,re,subprocess
root=pathlib.Path(__file__).resolve().parent.parent
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('sdk',type=pathlib.Path);p.add_argument('candidate',type=pathlib.Path)
p.add_argument('journal_name');p.add_argument('output',type=pathlib.Path);a=p.parse_args()
if not all(x.is_absolute() for x in (a.sdk,a.candidate,a.output)):p.error('Use absolute paths')
if a.output.exists() or a.output.is_symlink() or 'private' not in a.output.parts or a.output.suffix!='.elf':p.error('Use a new private ELF output')
if not re.fullmatch(r'run-[0-9]{1,20}-[0-9]{6}-[0-9]{1,10}\.log',a.journal_name):p.error('Use the exact own journal basename from the diagnostics directory')
spec=json.loads(a.candidate.read_text());build=spec.get('build_id','')
if spec.get('title_id')!='PPSA94678' or not re.fullmatch(r'[a-z0-9-]{1,64}',build):p.error('Invalid own candidate identity/build')
a.output.parent.mkdir(parents=True,exist_ok=True,mode=0o700)
subprocess.run([str(a.sdk/'bin/prospero-clang'),'-std=c11','-O2','-Wall','-Wextra','-Werror',
               '-DDKR_JOURNAL_NAME="'+a.journal_name+'"','-DDKR_BUILD_ID="'+build+'"',
               str(root/'src/read_own_journal.c'),'-o',str(a.output)],check=True)
record={'schema':1,'build_id':build,'journal_name':a.journal_name,'game_uid':1,'file':a.output.name,
        'size':a.output.stat().st_size,'sha256':hashlib.sha256(a.output.read_bytes()).hexdigest(),
        'console_executed':False,'scope':'Only read the explicitly chosen own journal; drop only own reader identity if needed; no console files/processes modified'}
a.output.with_suffix('.json').write_text(json.dumps(record,indent=2)+'\n')
print('Prepared exact-build journal reader:',a.output,'SHA256=',record['sha256'],'; no console connection')
