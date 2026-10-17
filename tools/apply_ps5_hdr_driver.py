#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Apply HDR10 scanout after the pinned PS5 driver policy, with exact hashes."""
import argparse,hashlib,json,pathlib,subprocess
root=pathlib.Path(__file__).resolve().parents[1]
def apply(mesa):
 m=json.loads((root/'patches/ps5-hdr-driver.json').read_text());patch=root/'patches/0009-ps5-driver-hdr10.patch'
 if hashlib.sha256(patch.read_bytes()).hexdigest()!=m['patch_sha256']:raise ValueError('HDR driver patch differs')
 states=[]
 for n,v in m['files'].items():
  p=mesa/n
  if p.is_symlink() or not p.is_file():raise ValueError('Missing ordinary HDR driver input')
  h=hashlib.sha256(p.read_bytes()).hexdigest();states.append((h==v['original_sha256'],h==v['patched_sha256']))
 if all(x[1] for x in states):return
 if not all(x[0] for x in states):raise ValueError('HDR driver input differs from the exact pinned SDR policy')
 cmd=['patch','--batch','--fuzz=0','-p1','-d',str(mesa),'-i',str(patch)]
 subprocess.run(cmd+['--dry-run'],check=True);subprocess.run(cmd,check=True)
 for n,v in m['files'].items():
  if hashlib.sha256((mesa/n).read_bytes()).hexdigest()!=v['patched_sha256']:raise ValueError('HDR driver readback differs')
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('mesa',type=pathlib.Path);a=p.parse_args()
 if not a.mesa.is_absolute():p.error('Use an absolute Mesa work directory')
 try:apply(a.mesa)
 except (ValueError,OSError) as e:p.error(str(e))
 print('PASS exact HDR10 driver patch')
