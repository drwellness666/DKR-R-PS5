#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Explicit, bounded PC capture of an existing kernel-log TCP service."""
import argparse,hashlib,json,os,pathlib,re,socket,time
MAX_LOG_BYTES=64*1024*1024
MAX_RECEIVE_RECORDS=65536
def private_file(path):
    return os.fdopen(os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600),'wb')
def capture(host,port,output,seconds,build):
    if not output.is_absolute() or output.exists() or 'private' not in output.parts:raise ValueError('Use a new absolute private session directory')
    if not 1<=seconds<=3600 or not 1<=port<=65535:raise ValueError('Invalid capture limit/port')
    if not isinstance(build,str) or not re.fullmatch(r'[a-z0-9-]{1,64}',build):raise ValueError('Invalid build label')
    # Connect first; a refused connection leaves no misleading empty session.
    connection=socket.create_connection((host,port),timeout=5)
    with connection:
        output.mkdir(parents=True,mode=0o700);connection.settimeout(1)
        record={'schema':2,'build_id':build,'build_label_is_proof':False,'started_utc':time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime()),'port':port,'bytes':0,'end_reason':'duration','crash_inferred_from_disconnect':False,
                'receive_records':0,'receive_index_truncated':False,
                'receive_time_basis':'PC monotonic nanoseconds since capture start; network arrival, not PS5 execution or display timing'}
        start=time.monotonic_ns();deadline=start+int(seconds*1e9)
        digest=hashlib.sha256();timing_digest=hashlib.sha256()
        try:
            with private_file(output/'kernel.log') as log, private_file(output/'RECEIVE.jsonl') as timing:
                while time.monotonic_ns()<deadline:
                    try:chunk=connection.recv(min(32768,MAX_LOG_BYTES-record['bytes']))
                    except socket.timeout:continue
                    except OSError as e:record['end_reason']='connection-error';record['error_type']=type(e).__name__;break
                    if not chunk:record['end_reason']='connection-closed';break
                    received=time.monotonic_ns()-start
                    log.write(chunk);log.flush();digest.update(chunk)
                    if record['receive_records']<MAX_RECEIVE_RECORDS:
                        row=(json.dumps([record['bytes'],len(chunk),received],separators=(',',':'))+'\n').encode()
                        timing.write(row);timing.flush();timing_digest.update(row);record['receive_records']+=1
                    else:record['receive_index_truncated']=True
                    record['bytes']+=len(chunk)
                    if record['bytes']>=MAX_LOG_BYTES:record['end_reason']='size-limit';break
        except KeyboardInterrupt:record['end_reason']='user-stop'
        except OSError:
            record['end_reason']='local-io-error'
            raise
        finally:
            record['sha256']=digest.hexdigest();record['receive_index_sha256']=timing_digest.hexdigest()
            with private_file(output/'SESSION.json') as metadata:metadata.write((json.dumps(record,indent=2)+'\n').encode())
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
