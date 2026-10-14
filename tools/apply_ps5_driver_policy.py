#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Apply the exact pinned driver policy, verifying every changed source byte."""
import argparse,hashlib,json,pathlib
from apply_dependency_patch import apply_dependency_patch
root=pathlib.Path(__file__).resolve().parent.parent
def apply(mesa):
 spec=json.loads((root/'patches/ps5-driver-policy.json').read_text())
 patch=root/'patches/0006-ps5-driver-policy.patch'
 if hashlib.sha256(patch.read_bytes()).hexdigest()!=spec['patch_sha256']:raise ValueError('Driver policy patch hash mismatch')
 old=True;new=True
 for name,row in spec['files'].items():
  p=mesa/name
  if p.is_symlink() or not p.is_file():raise ValueError('Missing ordinary pinned driver source: '+name)
  digest=hashlib.sha256(p.read_bytes()).hexdigest();old&=digest==row['original_sha256'];new&=digest==row['patched_sha256']
 if new:return {'unchanged':True,'patch_sha256':spec['patch_sha256']}
 if not old:raise ValueError('Unknown/mixed PS5 driver source; use a fresh pinned tree')
 receipt=apply_dependency_patch(mesa,patch)
 for name,row in spec['files'].items():
  if hashlib.sha256((mesa/name).read_bytes()).hexdigest()!=row['patched_sha256']:raise ValueError('Driver policy read-back mismatch: '+name)
 return receipt
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('mesa',type=pathlib.Path);a=p.parse_args()
 if not a.mesa.is_absolute():p.error('Use an absolute dedicated PS5_Mesa source directory')
 try:receipt=apply(a.mesa)
 except (ValueError,OSError) as e:p.error(str(e))
 print('PASS exact PS5 driver policy:',json.dumps(receipt,sort_keys=True))
