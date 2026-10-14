#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Verify a private update pair; send only on explicit --apply/--rollback."""
import argparse,hashlib,ipaddress,json,pathlib,socket,sys
MARKERS={'update':'DKRR PREVIEW UPDATE PASS: files staged; launch manually',
         'rollback':'DKRR PREVIEW ROLLBACK PASS: files staged; launch manually'}
def load_package(folder,mode=None):
 if folder.is_symlink() or not folder.is_dir():raise ValueError('Use an ordinary package directory')
 path=folder/'READY.json'
 if path.is_symlink() or not path.is_file():raise ValueError('Missing ordinary READY.json')
 spec=json.loads(path.read_text())
 if spec.get('schema')!=1 or spec.get('title_id')!='PPSA94678':raise ValueError('Unexpected package schema/title')
 payloads=spec.get('payloads',{})
 if set(payloads)!=set(MARKERS):raise ValueError('Package must contain exactly update and rollback entries')
 data={}
 for name in MARKERS:
  row=payloads[name]
  if row.get('file')!=name+'.elf':raise ValueError('Unexpected payload filename')
  file=folder/row['file']
  if file.is_symlink() or not file.is_file():raise ValueError('Missing ordinary payload: '+name)
  value=file.read_bytes()
  if len(value)!=row.get('size') or hashlib.sha256(value).hexdigest()!=row.get('sha256') or value[:4]!=b'\x7fELF':raise ValueError('Payload verification failed: '+name)
  if mode is None or mode==name:data[name]=value
 return spec,data

def send_checked(host,port,data,mode,connect=socket.create_connection,emit=print):
 address=ipaddress.ip_address(host)
 if not address.is_private or address.is_unspecified or address.is_multicast:raise ValueError('Use your console private LAN IP, not discovery/public endpoints')
 if mode not in MARKERS or not 1<=port<=65535:raise ValueError('Invalid operation/port')
 collected=bytearray()
 with connect((host,port),timeout=10) as sock:
  sock.settimeout(180);sock.sendall(data);sock.shutdown(socket.SHUT_WR);sock.settimeout(70)
  while True:
   part=sock.recv(4096)
   if not part:break
   collected.extend(part)
   if len(collected)>262144:raise RuntimeError('Receipt exceeded the bounded own-payload log size')
   text=collected.decode('utf-8',errors='replace')
   if 'DKRR PREVIEW STOP:' in text:
    emit(text);raise RuntimeError('Console payload stopped; do not launch or automatically retry')
   if MARKERS[mode]+'\n' in text.replace('\r\n','\n'):
    emit(text)
    return text
 text=collected.decode('utf-8',errors='replace');emit(text)
 raise RuntimeError('No authoritative completion marker received; transfer alone is not execution proof. Do not automatically resend.')

def main(argv=None):
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('package',type=pathlib.Path)
 p.add_argument('--host',help='Console private LAN IP; only needed for explicit send');p.add_argument('--port',type=int,default=9021)
 g=p.add_mutually_exclusive_group();g.add_argument('--apply',action='store_true');g.add_argument('--rollback',action='store_true')
 a=p.parse_args(argv);mode='update' if a.apply else 'rollback' if a.rollback else None
 try:
  _,payloads=load_package(a.package,mode)
  if mode is None:
   print('PASS: private update and rollback sizes/hashes/ELF headers verified. No console connection.');return 0
  if not a.host:p.error('--host is required when sending')
  print('Close the own DKR-R title before sending; this tool does not close applications or restart the console.')
  send_checked(a.host,a.port,payloads[mode],mode)
  print('Confirmed own file staging. Launch manually; picture/audio/saves remain a separate console test.');return 0
 except (ValueError,OSError,RuntimeError,KeyboardInterrupt) as e:
  print('STOP:',str(e) or 'Receiving interrupted; the payload may still be running.',file=sys.stderr)
  return 1
if __name__=='__main__':raise SystemExit(main())
