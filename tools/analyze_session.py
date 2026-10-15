#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Resolve this build's recorded RIP using its verified private ELF and entry."""
import argparse,hashlib,json,pathlib,re,struct,subprocess
def elf_layout(data):
    if len(data)<64 or data[:7]!=b'\x7fELF\x02\x01\x01':raise ValueError('Expected little-endian ELF64')
    header=struct.unpack_from('<16sHHIQQQIHHHHHH',data)
    if header[2]!=62 or header[1] not in (2,3):raise ValueError('Expected x86-64 executable/PIE')
    entry,phoff,phsize,phcount=header[4],header[5],header[9],header[10]
    if phsize!=56 or phcount>256 or phoff+phsize*phcount>len(data):raise ValueError('Invalid program headers')
    ranges=[]
    for i in range(phcount):
        kind,flags,offset,va,pa,filesz,memsz,align=struct.unpack_from('<IIQQQQQQ',data,phoff+i*phsize)
        if kind==1 and flags&1:ranges.append((va,va+memsz))
    if not any(low<=entry<high for low,high in ranges):raise ValueError('ELF entry outside executable segments')
    return entry,ranges
def analyze(log,manifest,elf,addr2line='addr2line'):
    data=elf.read_bytes()
    if hashlib.sha256(data).hexdigest()!=manifest['symbol_elf_sha256']:raise ValueError('Private symbol ELF hash mismatch')
    entry,ranges=elf_layout(data);crashes=[];captures=[];events=[];sessions=[];active=None
    for line in log.splitlines():
        if len(line)>16384:continue
        if match:=re.search(r'DKRR_SESSION v=1 build=([a-z0-9-]{1,64}) pid=([0-9]{1,10}) entry=(0x[0-9a-f]{16})$',line):
            active={'build_id':match[1],'pid':int(match[2]),'entry':match[3],'matches_candidate':match[1]==manifest['build_id'],'clean_shutdown_marker':False}
            sessions.append(active)
        if line.endswith('DKRR_SESSION_END') and active is not None:active['clean_shutdown_marker']=True
        if re.search(r'system_xo_violation|page fault|SIGSEGV|SIGBUS|SIGILL|fault address|kernel panic|can.t start game|\bRIP:',line,re.I):
            if len(events)<200:events.append(line[:1024])
        if 'DKRR_CRASH v=1 ' in line:
            body=line.split('DKRR_CRASH v=1 ',1)[1];match=re.fullmatch(r'build=([a-z0-9-]{1,64})((?: [a-z]+=[^ ]+)+)',body)
            if not match:continue
            pairs=re.findall(r'([a-z]+)=(0x[0-9a-f]{16})',match[2]);fields=dict(pairs);row={'build_id':match[1],'raw':fields,'attributed':False}
            expected={'signal','code','address','rip','rsp','trap','error','entry','stage'}
            if set(fields)!=expected or len(pairs)!=9 or match[2]!=''.join(' '+key+'='+value for key,value in pairs):row['reason']='incomplete or ambiguous crash record'
            elif match[1]!=manifest['build_id']:row['reason']='different build; symbols not applied'
            else:
                runtime=int(fields['entry'],16);rip=int(fields['rip'],16);base=runtime-entry;relative=rip-base
                if base<0 or base%4096 or not any(low<=relative<high for low,high in ranges):row['reason']='RIP/load base outside matching executable segments'
                else:
                    result=subprocess.run([addr2line,'-f','-C','-i','-e',str(elf),hex(relative)],text=True,capture_output=True,check=True)
                    row.update(attributed=True,load_base=hex(base),elf_address=hex(relative),symbolization=result.stdout.strip())
            crashes.append(row)
        if 'DKRR_CAPTURE v=1 ' in line:
            body=line.split('DKRR_CAPTURE v=1 ',1)[1];pairs=re.findall(r'([a-z0-9-]+)=([a-z0-9-]+)',body);fields=dict(pairs)
            if len(pairs)!=len(fields):continue
            if fields.get('build')!=manifest['build_id']:continue
            try:
                row={k:int(fields[k]) for k in ('intervals','elapsed-ns','max-gap-ns','over33','over50','over100','dropped')}
                if any(v<0 or v>2**64-1 for v in row.values()):continue
                if not 0<=row['over100']<=row['over50']<=row['over33']<=row['intervals']:continue
                row['present_returns_per_second']=row['intervals']*1e9/row['elapsed-ns'] if row['elapsed-ns'] else None
                row['complete_samples']=row['dropped']==0;captures.append(row)
            except (KeyError,ValueError):continue
    return {'schema':1,'build_id':manifest['build_id'],'crashes':crashes,'captures':captures,'journal_sessions':sessions,'unattributed_os_events':events,'note':'OS/loader events remain unattributed unless this build supplies its exact record. No crash is inferred from disconnect; no guessed load base or cross-build symbolization.'}
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('log',type=pathlib.Path);p.add_argument('manifest',type=pathlib.Path);p.add_argument('symbol_elf',type=pathlib.Path);p.add_argument('--addr2line',default='addr2line');a=p.parse_args()
    try:r=analyze(a.log.read_text(errors='replace'),json.loads(a.manifest.read_text()),a.symbol_elf,a.addr2line)
    except (ValueError,KeyError,OSError,subprocess.CalledProcessError) as e:p.error(str(e))
    print(json.dumps(r,indent=2))
