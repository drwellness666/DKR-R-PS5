#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Local browser-based build assistant. Game data never leaves the user's PC except explicit PS5 installation."""
import argparse, hashlib, hmac, ipaddress, json, os, pathlib, platform, secrets, shutil, socket, subprocess, sys, threading, time, urllib.parse, webbrowser
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
ROOT=pathlib.Path(__file__).resolve().parent
sys.path.insert(0,str(ROOT/'tools'))
from check_rom import check as check_rom
SIZE=12*1024*1024
TOOLS=('bash','make','git','curl','wget','tar','unzip','patch','pkg-config','clang','clang++','llvm-config','glslangValidator','flock','cmake','ld.lld','spirv-as','llvm-ar','llvm-ranlib','g++','bison','flex')
class Assistant:
    def __init__(self,root=ROOT,run=None,validate=check_rom):
        self.root=pathlib.Path(root);self.work=self.root/'work/assistant';self.validate=validate
        self.game=self.work/'game';self.token=secrets.token_urlsafe(32);self.lock=threading.RLock();self.runner=run or self.run
        self.state={'busy':False,'stage':'Ready','error':'','roms':{},'log':[],'built':False,'installed':False}
    def note(self,line):
        with self.lock:self.state['log']=(self.state['log']+[str(line).rstrip()])[-400:]
    def status(self):
        with self.lock:
            result=json.loads(json.dumps(self.state));result['helper_available']=(self.root/'bin/dkrr-save-metadata.elf').is_file();return result
    def preflight(self):
        missing=[x for x in TOOLS if not shutil.which(x)]
        if not (shutil.which('magick') or shutil.which('convert')):missing.append('ImageMagick')
        if platform.system()!='Linux' or platform.machine() not in ('x86_64','amd64'):missing.append('Linux x86-64 (native or WSL2)')
        if sys.version_info<(3,12):missing.append('Python 3.12 or newer')
        packages=''
        if pathlib.Path('/etc/arch-release').exists():packages='sudo pacman -S --needed base-devel git python cmake clang llvm lld curl wget unzip patch pkgconf pcre2 glslang imagemagick spirv-tools spirv-headers bison flex'
        elif pathlib.Path('/etc/debian_version').exists():packages='sudo apt install build-essential git python3 python3-venv cmake clang llvm-dev lld curl wget unzip patch pkg-config libpcre2-dev glslang-tools imagemagick spirv-tools spirv-headers bison flex'
        pcre=subprocess.run(['pkg-config','--exists','libpcre2-8'],capture_output=True).returncode==0 if shutil.which('pkg-config') else False
        if not pcre:missing.append('PCRE2 development files')
        free=shutil.disk_usage(self.root).free
        return {'missing':missing,'package_command':packages,'free_gib':round(free/2**30,1),'ok':not missing,'note':'First build downloads pinned public sources and needs several GB. Original ROM files stay unchanged.'}
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
                self.state['roms'][rev]={'revision':result['revision'],'sha1':result['canonical_sha1']};self.state['built']=False
                return result
            finally:
                if p.exists():p.unlink()
    def environment(self,jobs):
        env={k:v for k,v in os.environ.items() if not k.startswith('DKR_')}
        env.update(DKR_PS5_WORK=str(self.work/'sdl'),DKR_SDL_WORK=str(self.work/'sdl'),DKR_GPU_WORK=str(self.work/'gpu'),DKR_RT64_WORK=str(self.work/'rt64'),DKR_GAME_WORK=str(self.game),DKR_PS5_JOBS=str(jobs),DKR_GPU_JOBS=str(jobs),DKR_PS5_HIGH_REFRESH_BUILD='1',DKR_PS5_LINE_TABLES='1')
        revs=sorted(self.state['roms'])
        if not revs:raise ValueError('Select at least one supported ROM first.')
        if len(revs)==2:
            env.update(DKR_ROM_V77=str(self.work/'private/inputs/owned-us77.rom'),DKR_ROM_V80=str(self.work/'private/inputs/owned-us80.rom'))
        else:env['DKR_ROM']=str(self.work/('private/inputs/owned-us'+revs[0]+'.rom'))
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
        env=self.environment(jobs);game=self.game
        sdk=self.work/'sdl/sdk/ps5-payload-sdk'
        steps=[('Build SDL / SDK',['bash','build.sh']),('Build graphics driver',['bash','build_gpu.sh']),('Build RT64',['bash','build_rt64.sh']),('Build DKR-R',['bash','build_integrated.sh' if len(self.state['roms'])==2 else 'build_game.sh']),('Record build',[sys.executable,'tools/record_native_build.py',str(game)]),('Create game installer',[sys.executable,'tools/build_native_game_installer.py',str(sdk),str(game)]),('Create ROM installer',[sys.executable,'tools/build_owned_rom_installer.py',str(sdk),str(game)]),('Create save helper',['bash','tools/build_save_metadata.sh',str(sdk)])]
        for label,argv in steps:
            with self.lock:self.state['stage']=label
            self.runner(argv,env)
        required=[game/'private/dkrr-install-game-1.0.6-preview.elf',game/'private/dkrr-install-owned-rom.elf',self.root/'bin/dkrr-save-metadata.elf',game/'private/CANDIDATE.json']
        if not all(p.is_file() for p in required):raise RuntimeError('Build returned without all required outputs; installation stays disabled.')
        with self.lock:self.state.update(built=True,stage='Build complete — ready to install',error='')
    def install(self,host,closed):
        address=ipaddress.ip_address(host)
        if address.version!=4 or not address.is_private or address.is_loopback or address.is_unspecified or address.is_multicast:raise ValueError('Enter your PS5 private LAN IPv4 address.')
        if closed is not True:raise ValueError('Confirm DKR-R is completely closed before installing.')
        if not self.state['built']:raise ValueError('Complete this session\'s build before installing. Existing differing installations require the documented update route.')
        game=self.game/'private';env=self.environment(4)
        for name,marker in (('dkrr-install-game-1.0.6-preview.elf','NATIVE INSTALL PASS.'),('dkrr-install-owned-rom.elf','OWN ROM INSTALL PASS.')):
            with self.lock:self.state['stage']='Install '+('game' if name.startswith('dkrr-install-game') else 'owned ROM')
            output=self.runner([sys.executable,'tools/send_elf.py',str(address),str(game/name)],env)
            if marker not in output:raise RuntimeError('No authoritative installer success marker. Stop and inspect the console; do not retry blindly.')
        self.runner([sys.executable,'tools/send_elf.py',str(address),str(self.root/'bin/dkrr-save-metadata.elf'),'--no-wait'],env)
        with self.lock:self.state.update(installed=True,stage='Files installed. Scan /data/homebrew with ShadowMountPlus and launch DKR-R from its PS5 icon. Console save service sent for this session. The installed game itself needs no running PC; see docs/SAVES.md for the current console-reboot limitation.')
    def start(self,operation,args):
        with self.lock:
            if self.state['busy']:raise ValueError('An operation is already running.')
            self.state.update(busy=True,error='')
        def task():
            try:
                if operation=='build':self.build(int(args.get('jobs',4)))
                elif operation=='install':self.install(args.get('host',''),args.get('closed'))
                elif operation=='helper':
                    a=ipaddress.ip_address(args.get('host',''))
                    if a.version!=4 or not a.is_private or a.is_loopback or a.is_unspecified or a.is_multicast:raise ValueError('Enter a private LAN IPv4 address.')
                    self.runner([sys.executable,'tools/send_elf.py',str(a),str(self.root/'bin/dkrr-save-metadata.elf'),'--no-wait'],os.environ.copy())
                    self.note('Save helper sent; sending alone does not confirm execution.')
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
                args=json.loads(self.rfile.read(size));operation=urllib.parse.urlsplit(self.path).path.lstrip('/')
                if operation not in ('build','install','helper'):raise ValueError('Unknown action.')
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
