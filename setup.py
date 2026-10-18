#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Local browser-based build assistant. Game data never leaves the user's PC except explicit PS5 installation."""
import argparse, hashlib, hmac, ipaddress, json, os, pathlib, platform, secrets, shutil, socket, subprocess, sys, threading, time, urllib.parse, webbrowser
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
ROOT=pathlib.Path(__file__).resolve().parent
sys.path.insert(0,str(ROOT/'tools'))
from check_rom import check as check_rom
from host_requirements import inspect as inspect_host
from assistant_builds import History, completed_entry, inspect as inspect_build, source_identity
from build_native_game_deployer import ROM_HASHES
SIZE=12*1024*1024
class Assistant:
    def __init__(self,root=ROOT,run=None,validate=check_rom,history=None):
        self.root=pathlib.Path(root);self.work=self.root/'work/assistant';self.validate=validate
        self.game=self.work/'game';self.token=secrets.token_urlsafe(32);self.lock=threading.RLock();self.runner=run or self.run
        self.state={'busy':False,'stage':'Ready','error':'','roms':{},'log':[],'built':False,'installed':False}
        self.inputs={};self.completed=None
        history_path=(pathlib.Path.home()/'.local/state/dkrr-ps5/assistant.json' if self.root==ROOT else self.work/'private/history.json')
        self.history=History(history or history_path)
        self.recover()
    def recover(self):
        try:
            rows=self.history.read();source=source_identity(self.root)
        except (OSError,ValueError,TypeError) as e:
            self.note('Saved build history not reused: '+str(e));return
        for row in rows:
            # Only current-source builds can be restored. Older outputs stay
            # in history without rehashing their large app/package files.
            if row.get('source_sha256')!=source:continue
            try:verified=inspect_build(row)
            except (OSError,ValueError,KeyError,TypeError):
                self.note('A previous build is incomplete or changed; it remains preserved and is not reused.');continue
            if self.completed is None and row['source_sha256']==source:
                self.completed=row;self.game=verified['game'];self.inputs=verified['roms']
                self.state.update(built=True,roms={r:{'revision':'US 1.0' if r=='77' else 'US 1.1 / Rev A','sha1':ROM_HASHES[int(r)]} for r in self.inputs},stage='Verified completed build restored — ready to install')
                break
        # Recovered state does not claim that a console installation succeeded.
    def note(self,line):
        with self.lock:self.state['log']=(self.state['log']+[str(line).rstrip()])[-400:]
    def status(self):
        with self.lock:
            result=json.loads(json.dumps(self.state));return result
    def preflight(self):
        return inspect_host(self.root)
    def accept(self,data):
        if len(data)!=SIZE:raise ValueError('Choose a clean 12 MiB US DKR dump.')
        with self.lock:
            if self.state['busy']:raise ValueError('Wait for the running operation.')
            folder=self.work/'private/inputs';folder.mkdir(parents=True,exist_ok=True,mode=0o700)
            os.chmod(self.work/'private',0o700)
            p=folder/('upload-'+secrets.token_hex(8))
            try:
                with p.open('xb') as stream:stream.write(data)
                p.chmod(0o600);result=self.validate(p)
                if not result.get('supported'):raise ValueError('Unsupported ROM. Use an unmodified US 1.0 or US 1.1 / Rev A dump; PAL is unsupported.')
                rev='80' if 'v80' in result['revision'] else '77'
                dest=folder/('owned-us'+rev+'.rom')
                if dest.exists() and dest.read_bytes()!=data:raise ValueError('A different byte-order copy of this revision is already selected. Restart with the same input or use the existing selection.')
                if dest.exists():p.unlink()
                else:p.rename(dest)
                self.inputs[rev]=dest
                self.state['roms'][rev]={'revision':result['revision'],'sha1':result['canonical_sha1']};self.state['built']=False;self.completed=None
                return result
            finally:
                if p.exists():p.unlink()
    def environment(self,jobs):
        env={k:v for k,v in os.environ.items() if not k.startswith('DKR_')}
        env.update(DKR_PS5_WORK=str(self.work/'sdl'),DKR_SDL_WORK=str(self.work/'sdl'),DKR_GPU_WORK=str(self.work/'gpu'),DKR_RT64_WORK=str(self.work/'rt64'),DKR_GAME_WORK=str(self.game),DKR_PS5_JOBS=str(jobs),DKR_GPU_JOBS=str(jobs),DKR_PS5_HIGH_REFRESH_BUILD='1',DKR_PS5_LINE_TABLES='1')
        if set(self.state['roms']) != {'77','80'}:
            raise ValueError('Choose both owned US ROMs: 1.0 and 1.1 / Rev A.')
        env.update(DKR_ROM_V77=str(self.inputs.get('77',self.work/'private/inputs/owned-us77.rom')),DKR_ROM_V80=str(self.inputs.get('80',self.work/'private/inputs/owned-us80.rom')))
        return env
    def run(self,argv,env):
        self.note('$ '+' '.join(pathlib.Path(str(x)).name if str(x).startswith(str(self.root)) else str(x) for x in argv))
        output=[]
        with subprocess.Popen([str(x) for x in argv],cwd=self.root,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,errors='replace') as process:
            for line in process.stdout:self.note(line);output.append(line);output=output[-1000:]
            if process.wait()!=0:raise RuntimeError('Step failed. See the last log lines; fix the cause and run again.')
        return ''.join(output)
    def build(self,jobs):
        if not 1<=jobs<=32:raise ValueError('Choose 1–32 build jobs.')
        pre=self.preflight()
        if not pre['ok']:raise ValueError('Missing prerequisites: '+', '.join(pre['missing']))
        # A failed partial decompilation must never be mistaken for a reusable
        # completed game build. Reuse public compiler/renderer work, keep game
        # attempts separate and preserve previous outputs without overwriting.
        with self.lock:self.state['built']=False
        self.game=self.work/('game-'+time.strftime('%Y%m%d-%H%M%S')+'-'+secrets.token_hex(3))
        source_before=source_identity(self.root)
        env=self.environment(jobs);game=self.game
        sdk=self.work/'sdl/sdk/ps5-payload-sdk'
        steps=[('Build SDL / SDK',['bash','build.sh']),('Build graphics driver',['bash','build_gpu.sh']),('Build RT64',['bash','build_rt64.sh']),('Build DKR-R',['bash','build_integrated.sh']),('Record build',[sys.executable,'tools/record_native_build.py',str(game)]),('Create universal installer and update',[sys.executable,'tools/build_universal_installer.py',str(sdk),str(game/'private/native/PPSA94678'),str(game/'private/CANDIDATE.json'),str(game/'private/universal')])]
        for label,argv in steps:
            with self.lock:self.state['stage']=label
            self.runner(argv,env)
        required=[game/'private/CANDIDATE.json',game/'private/universal/CANDIDATE.json',*[game/'private/universal'/name for name in ('DKRR-Installer.elf','DKRR-Update.elf','DKRR-Update.dkru')]]
        if not all(p.is_file() for p in required):raise RuntimeError('Build returned without all required outputs; installation stays disabled.')
        entry=completed_entry(self.root,game)
        if entry['source_sha256']!=source_before:raise RuntimeError('Source changed during compilation; outputs preserved but not enabled for installation.')
        inspect_build(entry)
        self.history.remember(entry);self.completed=entry
        with self.lock:
            self.state.update(built=True,stage='Build complete — ready to install',error='')
    def install(self,host,closed,mode='install'):
        address=ipaddress.ip_address(host)
        if address.version!=4 or not address.is_private or address.is_loopback or address.is_unspecified or address.is_multicast:raise ValueError('Enter your PS5 private LAN IPv4 address.')
        if closed is not True:raise ValueError('Confirm DKR-R is completely closed before installing.')
        if mode not in ('install','update'):raise ValueError('Choose Install or Update.')
        if not self.state['built'] or self.completed is None:raise ValueError('Build DKR-R or reopen a verified completed build before installing.')
        if source_identity(self.root)!=self.completed['source_sha256']:raise ValueError('Source or licence inputs changed. Build again before installing.')
        # Recheck actual bytes just before deployment; a prior receipt or a UI
        # boolean cannot authorize a changed native executable or installer.
        current=inspect_build(self.completed)
        env=self.environment(4);payload=current['payloads']['DKRR-Installer.elf' if mode=='install' else 'DKRR-Update.elf']
        marker='DKRR UNIVERSAL INSTALL PASS.' if mode=='install' else 'DKRR UNIVERSAL UPDATE PASS.'
        with self.lock:self.state.update(installed=False,stage='Update DKR-R' if mode=='update' else 'Install DKR-R — both ROMs must be on USB in DKRR')
        output=self.runner([sys.executable,'tools/send_elf.py',str(address),str(payload)],env)
        if marker not in output:raise RuntimeError('No authoritative installer success marker. Inspect the console log before trying again.')
        with self.lock:self.state.update(installed=True,stage='Files verified. Scan /data/homebrew with ShadowMountPlus if needed and launch the PS5 icon. No PC or resident save service is required by this candidate.')
    def start(self,operation,args):
        with self.lock:
            if self.state['busy']:raise ValueError('An operation is already running.')
            self.state.update(busy=True,error='')
        def task():
            try:
                if operation=='build':self.build(int(args.get('jobs',4)))
                elif operation=='install':self.install(args.get('host',''),args.get('closed'),args.get('mode','install'))
                else:raise ValueError('Unknown operation.')
            except Exception as e:
                with self.lock:self.state.update(error=str(e),stage='Stopped')
            finally:
                with self.lock:self.state['busy']=False
        threading.Thread(target=task,daemon=True).start()
def make_server(app,port=0):
    class Handler(BaseHTTPRequestHandler):
        def log_message(self,*args):pass
        def respond(self,code,value,content='application/json'):
            data=value if isinstance(value,bytes) else json.dumps(value).encode()
            self.send_response(code);self.send_header('Content-Type',content);self.send_header('Content-Length',str(len(data)));self.send_header('Cache-Control','no-store');self.send_header('X-Content-Type-Options','nosniff');self.send_header('Content-Security-Policy',"default-src 'self'; script-src 'self' 'unsafe-inline'; style-src 'self' 'unsafe-inline'; connect-src 'self'; frame-ancestors 'none'");self.end_headers();self.wfile.write(data)
        def allowed(self):return self.headers.get('Host')=='127.0.0.1:'+str(self.server.server_port) and hmac.compare_digest(self.headers.get('X-Setup-Token',''),app.token)
        def do_GET(self):
            path=urllib.parse.urlsplit(self.path).path
            if path=='/':
                self.respond(200,(app.root/'tools/setup_ui.html').read_bytes(),'text/html; charset=utf-8');return
            if not self.allowed():self.respond(403,{'error':'Forbidden'});return
            if path=='/status':self.respond(200,app.status())
            elif path=='/preflight':self.respond(200,app.preflight())
            else:self.respond(404,{'error':'Not found'})
        def do_POST(self):
            if not self.allowed():self.respond(403,{'error':'Forbidden'});return
            try:
                size=int(self.headers.get('Content-Length','0'))
                if urllib.parse.urlsplit(self.path).path=='/rom':
                    if size!=SIZE:raise ValueError('ROM must be exactly 12 MiB.')
                    data=self.rfile.read(size)
                    self.respond(200,app.accept(data));return
                if not 0<size<=4096:raise ValueError('Invalid request size.')
                args=json.loads(self.rfile.read(size))
                if not isinstance(args,dict):raise ValueError('Expected an action object.')
                operation=urllib.parse.urlsplit(self.path).path.lstrip('/')
                if operation not in ('build','install'):raise ValueError('Unknown action.')
                app.start(operation,args);self.respond(202,{'started':operation})
            except (ValueError,OSError) as e:self.respond(400,{'error':str(e)})
    return ThreadingHTTPServer(('127.0.0.1',port),Handler)
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--no-browser',action='store_true');p.add_argument('--port',type=int,default=0);args=p.parse_args()
    app=Assistant();server=make_server(app,args.port);url='http://127.0.0.1:'+str(server.server_port)+'/#'+app.token
    print('DKR-R PS5 Build Assistant\n'+url+'\nKeep this terminal open. Ctrl+C stops the local assistant. No ROM is sent to GitHub.',flush=True)
    if not args.no_browser:webbrowser.open(url)
    try:server.serve_forever()
    except KeyboardInterrupt:pass
    finally:server.server_close()
if __name__=='__main__':main()
