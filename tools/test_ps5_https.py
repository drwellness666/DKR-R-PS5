#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise the production transport against a real local TLS fixture."""
import argparse,http.server,os,pathlib,ssl,subprocess,tempfile,threading,time
root=pathlib.Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--native-libraries',type=pathlib.Path);a=p.parse_args()
with tempfile.TemporaryDirectory(prefix='dkrr-https-') as folder:
    t=pathlib.Path(folder)
    subprocess.run(['openssl','req','-x509','-newkey','rsa:2048','-nodes','-keyout',str(t/'ca.key'),'-out',str(t/'ca.pem'),'-days','1','-subj','/CN=DKRR transport fixture'],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
    subprocess.run(['openssl','req','-newkey','rsa:2048','-nodes','-keyout',str(t/'server.key'),'-out',str(t/'server.csr'),'-subj','/CN=localhost'],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
    (t/'extensions').write_text('subjectAltName=DNS:localhost\nextendedKeyUsage=serverAuth\n')
    subprocess.run(['openssl','x509','-req','-in',str(t/'server.csr'),'-CA',str(t/'ca.pem'),'-CAkey',str(t/'ca.key'),'-CAcreateserial','-out',str(t/'server.pem'),'-days','1','-extfile',str(t/'extensions')],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
    (t/'roots.h').write_text('static constexpr char dkrr_tls_roots[]=R"CA('+ (t/'ca.pem').read_text()+')CA";\n')
    class Handler(http.server.BaseHTTPRequestHandler):
        def log_message(self,*args):pass
        def do_POST(self):self.respond(200,self.rfile.read(int(self.headers['Content-Length'])))
        def respond(self,status,body,length=None):
            self.send_response(status);self.send_header('Content-Length',str(len(body) if length is None else length))
            if status==302:self.send_header('Location','https://untrusted.invalid/never-follow')
            self.end_headers();self.wfile.write(body)
        def do_GET(self):
            if self.path=='/good':self.respond(200,b'verified')
            elif self.path=='/redirect':self.respond(302,b'redirect')
            elif self.path=='/large':self.respond(200,b'x'*1024)
            elif self.path=='/truncated':self.respond(200,b'short',100)
            elif self.path=='/paced':
                self.send_response(200);self.send_header('Content-Length',str(8192*20));self.end_headers()
                for _ in range(20):self.wfile.write(b'p'*8192);self.wfile.flush();time.sleep(.05)
            elif self.path=='/stalled':
                self.send_response(200);self.send_header('Content-Length','1024');self.end_headers();self.wfile.flush();time.sleep(3)
            else:self.respond(404,b'not found')
    class Server(http.server.ThreadingHTTPServer):
        def handle_error(self,*args):pass
    server=Server(('127.0.0.1',0),Handler);context=ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER);context.load_cert_chain(str(t/'server.pem'),str(t/'server.key'));server.socket=context.wrap_socket(server.socket,server_side=True)
    thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
    objects=[]
    if a.native_libraries:
        libs=a.native_libraries.resolve()
        for relative in ('src/ps5_net.c','tests/ps5_scenet_host.c'):
            obj=t/(pathlib.Path(relative).stem+'.o')
            subprocess.run(['clang','-std=c11','-D_GNU_SOURCE','-Wno-macro-redefined','-O1','-g','-fsanitize=address,undefined','-I'+str(root/'src'),'-c',str(root/relative),'-o',str(obj)],check=True)
            objects.append(str(obj))
        flags=['-I'+str(libs/'curl/include'),str(libs/'curl-scenet/lib/libcurl.a'),*[''+str(libs/'install/lib'/name) for name in ('libmbedtls.a','libmbedx509.a','libmbedcrypto.a')],'-lm','-ldl']
    else:
        flags=subprocess.check_output(['pkg-config','--cflags','--libs','libcurl'],text=True).split()
    subprocess.run(['clang++','-std=c++20','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-DDKRR_TLS_ROOTS_HEADER="roots.h"','-I'+str(root/'src'),'-I'+str(t),str(root/'src/ps5_https.cpp'),str(root/'tests/ps5_https.cpp'),*objects,*flags,'-pthread','-o',str(t/'test')],check=True)
    try:subprocess.run([str(t/'test'),f'https://localhost:{server.server_port}'],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
    finally:server.shutdown();server.server_close();thread.join()
