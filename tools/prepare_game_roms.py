#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Validate owned US dumps and create private canonical copies; supports one revision."""
import argparse, hashlib, json
from pathlib import Path
from check_rom import canonical
KNOWN={77:'0cb115d8716dbbc2922fda38e533b9fe63bb9670',80:'6d96743d46f8c0cd0edb0ec5600b003c89b93755'}
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('destination',type=Path);p.add_argument('v77',type=Path,nargs='?');p.add_argument('v80',type=Path,nargs='?');p.add_argument('--rom',type=Path,action='append',default=[]);a=p.parse_args()
 if not a.destination.is_absolute():p.error('Use an absolute private destination')
 if a.rom and (a.v77 or a.v80):p.error('Use --rom or the legacy v77/v80 positions, not both')
 inputs=[(None,x) for x in a.rom] if a.rom else [(77,a.v77),(80,a.v80)]
 inputs=[x for x in inputs if x[1] is not None]
 if not inputs:p.error('Supply at least one own supported ROM using --rom')
 selected={}
 # Validate every input before creating any output.
 for expected,original in inputs:
  if original.stat().st_size!=12*1024*1024:raise SystemExit('Expected a clean 12 MiB own cartridge dump')
  data,order=canonical(original.read_bytes());sha=hashlib.sha1(data).hexdigest();revision=next((rev for rev,wanted in KNOWN.items() if sha==wanted),None)
  if revision is None or (expected and revision!=expected):raise SystemExit('Dump does not match the requested supported US revision')
  if revision in selected:raise SystemExit('Duplicate ROM revision supplied')
  selected[revision]=(data,order)
 a.destination.mkdir(parents=True,exist_ok=True,mode=0o700)
 for revision,(data,order) in sorted(selected.items()):
  target=a.destination/f'dkr.us.v{revision}.z64'
  if target.exists() or target.is_symlink():
   if target.is_symlink() or not target.is_file() or target.read_bytes()!=data:raise SystemExit('Refusing to replace a different existing canonical file')
  else:
   with target.open('xb') as f:f.write(data)
   target.chmod(0o600)
  print(f'Private canonical v{revision} verified ({order}); original unchanged')
 manifest=a.destination/'selected-revisions.json'
 if manifest.is_symlink():raise SystemExit('Refusing a linked ROM selection manifest')
 manifest.write_text(json.dumps({'revisions':sorted(selected),'canonical_sha1':{str(rev):KNOWN[rev] for rev in selected}},indent=2)+'\n');manifest.chmod(0o600)
if __name__=='__main__':main()
