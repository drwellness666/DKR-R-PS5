#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Local loopback capture/relay and exact-ELF symbolization, no PS5 access."""
import hashlib,json,os,pathlib,re,signal,socket,subprocess,tempfile,threading
from analyze_session import analyze,elf_layout
from collect_session import capture
root=pathlib.Path(__file__).resolve().parent.parent
with tempfile.TemporaryDirectory(prefix='dkrr-session-') as temporary:
    work=pathlib.Path(temporary);private=work/'private';private.mkdir()
    source=work/'fault.c';source.write_text('int marker(void){return 42;} int main(void){return marker();}\n')
    elf=work/'symbols.elf';subprocess.run(['clang','-O0','-g','-fPIE','-pie',str(source),'-o',str(elf)],check=True)
    entry,ranges=elf_layout(elf.read_bytes());symbol=subprocess.check_output(['nm','-n',str(elf)],text=True)
    rip=int(re.search(r'^([0-9a-f]+) T marker$',symbol,re.M)[1],16);base=0x700000000000
    manifest={'build_id':'integrated-test','symbol_elf_sha256':hashlib.sha256(elf.read_bytes()).hexdigest()}
    def crash(build='integrated-test',where=base+rip,at=base+entry):
        values={'signal':11,'code':1,'address':0xd0,'rip':where,'rsp':base+0x30000,'trap':14,'error':4,'entry':at,'stage':4}
        return 'DKRR_CRASH v=1 build='+build+''.join(f' {k}=0x{v:016x}' for k,v in values.items())+'\n'
    log=crash()+crash('other-build')+crash(where=1)+crash(at=base+entry+1)+'DKRR_CAPTURE v=1 build=integrated-test intervals=60 elapsed-ns=1000000000 max-gap-ns=60000000 over33=1 over50=1 over100=0 dropped=0\n'
    result=analyze(log,manifest,elf);assert len(result['crashes'])==4 and sum(x['attributed'] for x in result['crashes'])==1
    assert 'marker' in result['crashes'][0]['symbolization'];assert result['captures'][0]['present_returns_per_second']==60
    try:analyze(log,dict(manifest,symbol_elf_sha256='0'*64),elf)
    except ValueError:pass
    else:raise AssertionError('Wrong ELF accepted')
    try:elf_layout(b'fake')
    except ValueError:pass
    else:raise AssertionError('Malformed ELF accepted')
    server=socket.socket();server.bind(('127.0.0.1',0));server.listen(1);port=server.getsockname()[1]
    def sender():
        with server:
            client,_=server.accept()
            with client:client.sendall(log[:83].encode());client.sendall(log[83:].encode())
    thread=threading.Thread(target=sender);thread.start()
    record=capture('127.0.0.1',port,private/'session',2,'integrated-test');thread.join(5)
    assert (private/'session/kernel.log').read_text()==log and record['end_reason']=='connection-closed'
    assert not record['crash_inferred_from_disconnect']
    fifo=work/'klog';os.mkfifo(fifo);feed=os.open(fifo,os.O_RDWR|os.O_NONBLOCK)
    listener=socket.socket();listener.bind(('127.0.0.1',0));port=listener.getsockname()[1]
    common=['clang','-std=c11','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',f'-DDKR_LOG_PORT={port}',f'-DDKR_LOG_DEVICE="{fifo}"',str(root/'src/ps5_session_relay.c'),'-o',str(work/'relay')]
    subprocess.run(common,check=True)
    busy=subprocess.run([str(work/'relay')],capture_output=True);assert busy.returncode==2;listener.close()
    relay=subprocess.Popen([str(work/'relay')],stderr=subprocess.PIPE,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
    try:
        ready=relay.stderr.readline();assert 'relay ready' in ready
        with socket.create_connection(('127.0.0.1',port),timeout=3) as pc:
            os.write(feed,log.encode());received=b''
            while len(received)<len(log):received+=pc.recv(4096)
            assert received==log.encode()
    finally:relay.send_signal(signal.SIGTERM);relay.wait(timeout=4);os.close(feed)
    assert relay.returncode==0
    print('PASS exact-build ELF hash/entry/segment attribution, mismatch refusal, chunked capture, disconnect semantics, actual bounded relay and occupied-port refusal')
