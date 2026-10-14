#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Explicit, bounded PC capture of an existing kernel-log TCP service."""
import argparse,hashlib,json,pathlib,socket,time
def capture(host,port,output,seconds,build):
    if not output.is_absolute() or output.exists() or 'private' not in output.parts:raise ValueError('Use a new absolute private session directory')
    if not 1<=seconds<=3600 or not 1<=port<=65535:raise ValueError('Invalid capture limit/port')
    # Connect first; a refused connection leaves no misleading empty session.
    connection=socket.create_connection((host,port),timeout=5)
    output.mkdir(parents=True,mode=0o700);connection.settimeout(1)
    record={'schema':1,'build_id':build,'started_utc':time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime()),'port':port,'bytes':0,'end_reason':'duration','crash_inferred_from_disconnect':False}
    deadline=time.monotonic()+seconds
    try:
        with connection, (output/'kernel.log').open('xb') as log:
            while time.monotonic()<deadline:
                try:chunk=connection.recv(32768)
                except socket.timeout:continue
                except OSError as e:record['end_reason']='connection-error';record['error_type']=type(e).__name__;break
                if not chunk:record['end_reason']='connection-closed';break
                log.write(chunk);log.flush();record['bytes']+=len(chunk)
                if record['bytes']>=64*1024*1024:record['end_reason']='size-limit';break
    except KeyboardInterrupt:record['end_reason']='user-stop'
    finally:
        file=output/'kernel.log';record['sha256']=hashlib.sha256(file.read_bytes()).hexdigest() if file.exists() else None
        (output/'SESSION.json').write_text(json.dumps(record,indent=2)+'\n')
    return record
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--host');p.add_argument('--port',type=int,default=3232);p.add_argument('--seconds',type=int,default=900)
    p.add_argument('--output',type=pathlib.Path);p.add_argument('--build-id',default='unspecified');p.add_argument('--capture',action='store_true');a=p.parse_args()
    if not a.capture:print('Offline: no connection. Use --capture --host YOUR_PS5_IP --output ABSOLUTE/private/NEW_SESSION --build-id ID when testing.');raise SystemExit(0)
    if not a.host or a.output is None:p.error('Explicit host and output required')
    try:r=capture(a.host,a.port,a.output,a.seconds,a.build_id)
    except (ValueError,OSError) as e:p.error(str(e))
    print('Session saved:',a.output,'bytes=',r['bytes'],'end=',r['end_reason']);print('Connection loss alone is not evidence of a kernel panic.')
