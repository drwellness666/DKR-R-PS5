#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Real resolver integration against a local UDP/TCP DNS fixture, with ASan."""
import argparse,hashlib,json,os,pathlib,shutil,socket,struct,subprocess,tarfile,tempfile,threading,time,urllib.request
p=argparse.ArgumentParser();p.add_argument('source',nargs='?',type=pathlib.Path);p.add_argument('build',nargs='?',type=pathlib.Path);p.add_argument('--native',action='store_true');p.add_argument('--cmake',default='cmake');p.add_argument('--cache',type=pathlib.Path);a=p.parse_args();r=pathlib.Path(__file__).resolve().parents[1]
if bool(a.source)!=bool(a.build):p.error('Supply both source and build, or neither')
prepared=None
if a.source is None:
 prepared=tempfile.TemporaryDirectory(prefix='dkrr-dns-deps-');base=pathlib.Path(prepared.name)
 row=next(v for v in json.loads((r/'GAME_SOURCES.json').read_text())['sources'] if v.get('name')=='c-ares')
 archive=base/row['file'];cached=a.cache/row['file'] if a.cache else None
 if cached and cached.is_file():shutil.copyfile(cached,archive)
 else:
  with urllib.request.urlopen(row['url'],timeout=30) as stream:
   data=stream.read(32*1024*1024+1)
  if len(data)>32*1024*1024:raise ValueError('DNS source archive exceeds size cap')
  archive.write_bytes(data)
 if hashlib.sha256(archive.read_bytes()).hexdigest()!=row['sha256']:raise ValueError('DNS source archive hash mismatch')
 with tarfile.open(archive) as source:source.extractall(base,filter='data')
 a.source=base/'c-ares-1.34.9';a.build=base/'build'
 subprocess.run([a.cmake,'-S',str(a.source),'-B',str(a.build),'-DCARES_STATIC=ON','-DCARES_SHARED=OFF','-DCARES_THREADS=OFF','-DCARES_BUILD_TOOLS=OFF','-DCARES_BUILD_TESTS=OFF','-DHAVE_O_NONBLOCK=0','-DHAVE_FCNTL_O_NONBLOCK=0','-DHAVE_IOCTL_FIONBIO=1','-DCMAKE_C_FLAGS=-fsanitize=address,undefined -fno-omit-frame-pointer','-DCMAKE_BUILD_TYPE=Release'],check=True,stdout=subprocess.DEVNULL)
 subprocess.run([a.cmake,'--build',str(a.build),'-j2'],check=True,stdout=subprocess.DEVNULL)

udp=socket.socket(socket.AF_INET,socket.SOCK_DGRAM);udp.bind(('127.0.0.1',0));port=udp.getsockname()[1]
tcp=socket.socket();tcp.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1);tcp.bind(('127.0.0.1',port));tcp.listen(8)
def name(raw):return b''.join(bytes([len(part)])+part for part in raw.split(b'.'))+b'\0'
def reply(data,stream=False):
 at=12;parts=[]
 while data[at]:n=data[at];at+=1;parts.append(data[at:at+n]);at+=n
 at+=1;qtype=struct.unpack('!H',data[at:at+2])[0];end=at+4;host=b'.'.join(parts)
 if host==b'timeout.test':return None
 if host==b'missing.test':return data[:2]+struct.pack('!HHHHH',0x8183,1,0,0,0)+data[12:end]
 if host==b'tcp.test' and not stream:return data[:2]+struct.pack('!HHHHH',0x8380,1,0,0,0)+data[12:end]
 addr=socket.inet_pton(socket.AF_INET6,'2001:db8::17') if qtype==28 else socket.inet_aton('192.0.2.17')
 rr=lambda owner,typ,raw:owner+struct.pack('!HHIH',typ,1,30,len(raw))+raw
 records=rr(b'\xc0\x0c',qtype,addr);count=1
 if host==b'alias.test':records=rr(b'\xc0\x0c',5,name(b'answer.test'))+rr(name(b'answer.test'),qtype,addr);count=2
 return data[:2]+struct.pack('!HHHHH',0x8180,1,count,0,0)+data[12:end]+records
def serve_udp():
 while True:
  data,peer=udp.recvfrom(4096);answer=reply(data)
  if answer:
   if b'wrong-id' in data:udp.sendto(bytes([answer[0]^255])+answer[1:],peer)
   udp.sendto(answer,peer)
def serve_tcp():
 while True:
  conn,_=tcp.accept()
  with conn:
   raw=conn.recv(2)
   if len(raw)!=2:continue
   length=struct.unpack('!H',raw)[0];data=b''
   while len(data)<length:data+=conn.recv(length-len(data))
   answer=reply(data,True)
   if answer:conn.sendall(struct.pack('!H',len(answer))+answer)
threading.Thread(target=serve_udp,daemon=True).start();threading.Thread(target=serve_tcp,daemon=True).start()
with tempfile.TemporaryDirectory(prefix='dkrr-dns-test-') as tmp:
 out=pathlib.Path(tmp)/'test'
 extra=['-DDKRR_TEST_NATIVE_NET','-Wno-macro-redefined',str(r/'src/ps5_net.c'),str(r/'tests/ps5_scenet_host.c')] if a.native else [str(r/'tests/ps5_network_restrictions.c'),'-Wl,--wrap=fcntl','-Wl,--wrap=ioctl','-Wl,--wrap=pipe']
 subprocess.run(['clang','-D_GNU_SOURCE','-std=c11','-g','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-DDKRR_DNS_TEST_PORT='+str(port),'-I'+str(r/'src'),'-I'+str(a.source/'include'),'-I'+str(a.build),str(r/'src/ps5_dns.c'),str(r/'tests/ps5_dns.c'),*extra,str(a.build/'lib/libcares.a'),'-Wl,--wrap=getifaddrs','-Wl,--wrap=freeifaddrs','-pthread','-o',str(out)],check=True)
 native=pathlib.Path(tmp)/'native-service'
 subprocess.run(['clang','-D_GNU_SOURCE','-std=c11','-g','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-I'+str(a.source/'include'),'-I'+str(a.build),str(r/'src/ps5_dns.c'),str(r/'tests/ps5_dns_native_service.c'),str(a.build/'lib/libcares.a'),'-pthread','-o',str(native)],check=True)
 env=dict(os.environ,UBSAN_OPTIONS='halt_on_error=1',ASAN_OPTIONS='detect_leaks=1')
 subprocess.run([str(native)],env=env,check=True,timeout=20)
 subprocess.run([str(out)],env=env,check=True,timeout=30)
 started=time.monotonic();subprocess.run([str(out),'timeout'],env=env,check=True,timeout=6);assert time.monotonic()-started<5
