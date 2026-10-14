#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Package only a checksum-complete, ROM-free source export as a release asset."""
import argparse,hashlib,pathlib,subprocess,sys,zipfile

def package(root,output):
 root=root.resolve();output=output.absolute()
 if output.exists() or root==output or root in output.parents:raise ValueError('Use a new ZIP outside the source tree')
 subprocess.run([sys.executable,str(root/'tools/check_public_source.py'),str(root)],check=True)
 sums=root/'SHA256SUMS';expected={}
 for line in sums.read_text().splitlines():
  digest,name=line.split('  ',1);path=pathlib.PurePosixPath(name)
  if len(digest)!=64 or path.is_absolute() or '..' in path.parts or name in expected:raise ValueError('Invalid source checksum inventory')
  expected[name]=digest
 actual={p.relative_to(root).as_posix():p for p in root.rglob('*') if p.is_file() and '.git' not in p.relative_to(root).parts and '__pycache__' not in p.relative_to(root).parts}
 checksum_files={'SHA256SUMS','vendor/SHA256SUMS'}
 if set(actual)!=set(expected)|checksum_files:raise ValueError('Source tree differs from the reviewed export inventory')
 for name,digest in expected.items():
  p=actual[name]
  if p.is_symlink() or hashlib.sha256(p.read_bytes()).hexdigest()!=digest:raise ValueError('Source checksum mismatch: '+name)
 output.parent.mkdir(parents=True,exist_ok=True)
 with zipfile.ZipFile(output,'x',compression=zipfile.ZIP_DEFLATED,compresslevel=6) as archive:
  for name,p in sorted(actual.items()):archive.write(p,'DKR-R-PS5/'+name)
 return {'file':output.name,'sha256':hashlib.sha256(output.read_bytes()).hexdigest(),'size':output.stat().st_size,'source_files':len(actual),'scope':'ROM-free source beta only; no console or private build output'}
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('source',type=pathlib.Path);p.add_argument('output',type=pathlib.Path);a=p.parse_args()
 try:result=package(a.source,a.output)
 except ValueError as e:p.error(str(e))
 print(result)
