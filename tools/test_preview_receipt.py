#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import hashlib,json,pathlib,tempfile
from run_preview_update import load_package,send_checked,MARKERS
class Socket:
 def __init__(self,chunks):self.chunks=iter(chunks);self.sent=None
 def __enter__(self):return self
 def __exit__(self,*args):return False
 def settimeout(self,n):pass
 def sendall(self,data):self.sent=data
 def shutdown(self,how):pass
 def recv(self,n):return next(self.chunks,b'')
with tempfile.TemporaryDirectory(prefix='dkrr-preview-receipt-') as directory:
 folder=pathlib.Path(directory);rows={}
 for mode in MARKERS:
  data=b'\x7fELFsynthetic-'+mode.encode();(folder/(mode+'.elf')).write_bytes(data)
  rows[mode]=dict(file=mode+'.elf',size=len(data),sha256=hashlib.sha256(data).hexdigest())
 (folder/'READY.json').write_text(json.dumps(dict(schema=1,title_id='PPSA94678',payloads=rows)))
 _,payloads=load_package(folder)
 for mode in MARKERS:
  bytes=(MARKERS[mode]+'\n').encode();sock=Socket([bytes[:18],bytes[18:]])
  output=send_checked('127.0.0.1',9021,payloads[mode],mode,connect=lambda *a,**k:sock,emit=lambda s:None)
  assert MARKERS[mode] in output and sock.sent==payloads[mode]
 for chunks in ([b'Transfer complete.\n'],[MARKERS['rollback'].encode()+b'\n'],[b'DKRR PREVIEW STOP: refused\n'],[b'x'*262145]):
  try:send_checked('127.0.0.1',9021,payloads['update'],'update',connect=lambda *a,**k:Socket(chunks),emit=lambda s:None)
  except RuntimeError:pass
  else:raise AssertionError('Non-authoritative/failure receipt accepted')
 (folder/'update.elf').write_bytes(b'\x7fELFtampered')
 try:load_package(folder)
 except ValueError:pass
 else:raise AssertionError('Tampered private binary accepted')
 try:send_checked('8.8.8.8',9021,b'fixture','update',connect=lambda *a,**k:(_ for _ in ()).throw(AssertionError('Connection should not happen')))
 except ValueError:pass
 print('PASS actual receipt/package logic: pre-send hashes, split markers, no transfer-only/wrong-mode/failure/oversize acceptance, no public connection')
