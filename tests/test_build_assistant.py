# SPDX-License-Identifier: GPL-3.0-or-later
"""Actual assistant orchestration, complete universal receipts and HTTP boundaries."""
import hashlib,importlib.util,json,os,pathlib,subprocess,tempfile,threading,time,urllib.request,urllib.error,unittest
from unittest.mock import patch
ROOT=pathlib.Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('assistant',ROOT/'setup.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
from assistant_builds import PAYLOADS,completed_entry,inspect as inspect_build,source_identity
from build_universal_installer import PUBLIC_FILES

class Tests(unittest.TestCase):
 def setUp(self):
  self.tmp=tempfile.TemporaryDirectory();self.root=pathlib.Path(self.tmp.name)
  (self.root/'tools').mkdir();(self.root/'tools/setup_ui.html').write_text('<title>Build Assistant</title>');self.commands=[]
  self.fixture_rom=b'local synthetic test data'.ljust(module.SIZE,b'0')
  self.rom_patch=patch.dict(module.ROM_HASHES,{77:hashlib.sha1(self.fixture_rom).hexdigest(),80:hashlib.sha1(self.fixture_rom).hexdigest()});self.rom_patch.start()
  self.app=module.Assistant(self.root,run=self.run_step)
 def tearDown(self):self.rom_patch.stop();self.tmp.cleanup()
 def run_step(self,argv,env):
  self.commands.append((argv,env.copy()))
  if argv[1:2]==['tools/record_native_build.py']:
   p=self.app.game/'private';p.mkdir(parents=True);app=p/'native/PPSA94678'
   build='integrated-'+'a'*24
   for name in PUBLIC_FILES:
    f=app/name;f.parent.mkdir(parents=True,exist_ok=True);f.write_bytes(('synthetic public '+name).encode())
   (app/'eboot.bin').write_bytes(bytes.fromhex('4f153d1d')+build.encode())
   (app/'sce_sys/param.json').write_text(json.dumps({'titleId':'PPSA94678','attribute3':0x40040}))
   for rev in (77,80):
    for i in range(33):
     f=app/f'assets/controller-audio/us-v{rev}/{i}.pcm';f.parent.mkdir(parents=True,exist_ok=True);f.write_bytes(b'\x00\x00')
   link=self.app.game/'native-link';link.mkdir();(link/'native-inputs.json').write_text(json.dumps({'build_id':build}))
   (link/'program.c').write_text('const char id[]=\"'+build+'\"; int main(void){return id[0]==0;}')
   subprocess.run(['clang','-g','-fPIE','-pie',str(link/'program.c'),'-o',str(link/'llvm-pie.elf')],check=True)
   (p/'roms').mkdir()
   for rev in (77,80):(p/'roms'/f'dkr.us.v{rev}.z64').write_bytes(self.fixture_rom)
   (p/'roms/selected-revisions.json').write_text(json.dumps({'revisions':[77,80]}))
   from record_native_build import record
   record(self.app.game)
  if argv[1:2]==['tools/build_universal_installer.py']:
   package=pathlib.Path(argv[-1]);package.mkdir()
   native=json.loads((self.app.game/'private/CANDIDATE.json').read_text());payloads={}
   for name in PAYLOADS:
    data=(b'DKRRPK2\n' if name.endswith('.dkru') else b'\x7fELF')+b'synthetic ROM-free payload'
    (package/name).write_bytes(data);payloads[name]={'size':len(data),'sha256':hashlib.sha256(data).hexdigest()}
   (package/'CANDIDATE.json').write_text(json.dumps(dict(schema=1,build_id=native['build_id'],
    files={k:v for k,v in native['files'].items() if k in PUBLIC_FILES},payloads=payloads,
    roms_embedded=False,original_pcm_embedded=False)))
  if 'tools/send_elf.py' in argv:
   return 'DKRR UNIVERSAL UPDATE PASS.' if argv[-1].endswith('DKRR-Update.elf') else 'DKRR UNIVERSAL INSTALL PASS.'
  return ''
 def prepared(self):
  self.app.state['roms']={'77':{'revision':'US 1.0'},'80':{'revision':'US Rev A'}}
  self.app.preflight=lambda:{'ok':True};self.app.build(2)
 def test_orchestration_and_safe_retries(self):
  self.prepared();first=self.app.game
  self.assertTrue(self.app.state['built']);self.assertEqual(self.commands[3][0],['bash','build_integrated.sh'])
  env=self.commands[3][1];self.assertEqual(env['DKR_GPU_JOBS'],'2');self.assertNotIn('DKR_PIPELINE',env);self.assertNotIn('DKR_ROM',env)
  self.assertEqual(env['DKR_PS5_HIGH_REFRESH_BUILD'],'1')
  self.assertEqual(self.commands[-1][0][1],'tools/build_universal_installer.py')
  self.assertEqual(self.commands[-1][0][3:], [str(first/'private/native/PPSA94678'),str(first/'private/CANDIDATE.json'),str(first/'private/universal')])
  self.assertFalse(any('build_native_game_installer.py' in str(x) or '--with-owned-roms' in x for argv,_ in self.commands for x in argv))
  self.assertEqual(len(inspect_build(self.app.completed)['spec']['files']),103)
  self.app.build(2);self.assertNotEqual(first,self.app.game);self.assertTrue((first/'private/CANDIDATE.json').exists())
  self.app.install('192.168.1.50',True);self.assertTrue(self.app.state['installed'])
  sends=[x[0] for x in self.commands if 'tools/send_elf.py' in x[0]]
  self.assertEqual(len(sends),1);self.assertTrue(sends[0][3].endswith('/private/universal/DKRR-Installer.elf'))
 def test_single_revision_and_install_guards(self):
  self.app.state['roms']={'80':{'revision':'US Rev A'}}
  with self.assertRaises(ValueError):self.app.environment(4)
  self.app.state['roms']['77']={'revision':'US v1.0'};env=self.app.environment(4);self.assertNotIn('DKR_ROM',env);self.assertIn('DKR_ROM_V77',env)
  for host,closed in [('8.8.8.8',True),('127.0.0.1',True),('0.0.0.0',True),('192.168.1.50',False)]:
   with self.assertRaises(ValueError):self.app.install(host,closed)
  self.app.preflight=lambda:{'ok':True};self.app.build(2);self.app.runner=lambda *a:'Transfer complete.'
  with self.assertRaises(RuntimeError):self.app.install('192.168.1.50',True)
  self.assertFalse(self.app.state['installed'])
 def test_recover_completed_build_and_update_without_predecessor(self):
  self.prepared();game=self.app.game;before=len(self.commands)
  restored=module.Assistant(self.root,run=self.run_step)
  self.assertTrue(restored.state['built']);self.assertFalse(restored.state['installed']);self.assertEqual(restored.game,game)
  self.assertEqual(restored.inputs['77'],game/'private/roms/dkr.us.v77.z64');self.assertEqual(len(self.commands),before)
  restored.install('192.168.1.50',True,'update');self.assertTrue(restored.state['installed'])
  self.assertEqual(len(self.commands)-before,1)
  self.assertTrue(self.commands[-1][0][3].endswith('/private/universal/DKRR-Update.elf'))
 def test_changed_outputs_never_reused_or_sent(self):
  self.prepared();game=self.app.game
  for relative in ('private/universal/DKRR-Installer.elf','private/universal/DKRR-Update.elf','private/universal/DKRR-Update.dkru',
                   'private/universal/CANDIDATE.json','private/native/PPSA94678/eboot.bin','native-link/llvm-pie.elf','private/roms/dkr.us.v77.z64'):
   with self.subTest(relative=relative):
    p=game/relative;old=p.read_bytes();p.write_bytes(old+b'changed')
    self.assertFalse(module.Assistant(self.root,run=self.run_step).state['built'])
    before=len(self.commands)
    with self.assertRaises(ValueError):self.app.install('192.168.1.50',True,'update')
    self.assertEqual(before,len(self.commands));p.write_bytes(old)
 def test_source_and_licences_disable_restore_and_send(self):
  self.prepared();history=self.app.history.path.read_bytes()
  for relative in ('build_game.sh','installer/main.c','LICENSE','THIRD_PARTY.md','licenses/COMPONENT-NOTICES.txt','licenses/CONTENT-PERMISSIONS.json'):
   with self.subTest(relative=relative):
    p=self.root/relative;p.parent.mkdir(parents=True,exist_ok=True);p.write_text('changed build input')
    with patch.object(module,'inspect_build',side_effect=AssertionError('Unrelated large build should not be read')):
     self.assertFalse(module.Assistant(self.root,run=self.run_step).state['built'])
    before=len(self.commands)
    with self.assertRaisesRegex(ValueError,'Source or licence'):self.app.install('192.168.1.50',True)
    self.assertEqual(len(self.commands),before);self.assertEqual(self.app.history.path.read_bytes(),history);p.unlink()
 def test_prose_only_does_not_invalidate_completed_build(self):
  self.prepared();(self.root/'README.md').write_text('new community introduction')
  self.assertTrue(module.Assistant(self.root,run=self.run_step).state['built'])
 def test_history_symlink_and_mid_build_source_change(self):
  self.prepared();history=self.app.history.path;old_history=history.read_bytes();old_game=self.app.game
  moved=history.with_name('preserved.json');history.rename(moved);history.symlink_to(moved)
  self.assertFalse(module.Assistant(self.root,run=self.run_step).state['built'])
  self.assertEqual(moved.read_bytes(),old_history);history.unlink();moved.rename(history)
  def changed(argv,env):
   result=self.run_step(argv,env)
   if 'tools/record_native_build.py' in argv:(self.root/'tools/changed.py').write_text('changed during build')
   return result
  self.app.runner=changed
  with self.assertRaises(RuntimeError):self.app.build(2)
  self.assertFalse(self.app.state['built']);self.assertEqual(history.read_bytes(),old_history)
  self.assertTrue((old_game/'private/CANDIDATE.json').is_file())
 def test_unknown_mode_refused_before_send(self):
  self.prepared();before=len(self.commands)
  for mode in ('unrecognized','../DKRR-Update.elf',None):
   with self.assertRaises(ValueError):self.app.install('192.168.1.50',True,mode)
  self.assertEqual(before,len(self.commands))
 def test_missing_public_package_prevents_completion(self):
  self.app.state['roms']={'77':{},'80':{}};self.app.preflight=lambda:{'ok':True}
  def missing(argv,env):
   result=self.run_step(argv,env)
   if 'tools/build_universal_installer.py' in argv:(self.app.game/'private/universal/DKRR-Update.dkru').unlink()
   return result
  self.app.runner=missing
  with self.assertRaisesRegex(RuntimeError,'required outputs'):self.app.build(2)
  self.assertFalse(self.app.state['built']);self.assertFalse(self.app.history.path.exists())
 def test_package_matches_recorded_app_and_is_rom_free(self):
  self.prepared();p=self.app.game/'private/universal/CANDIDATE.json';original=p.read_bytes()
  for key,value in [('build_id','integrated-'+'b'*24),('files',{}),('roms_embedded',True),('original_pcm_embedded',True)]:
   with self.subTest(key=key):
    package=json.loads(original);package[key]=value;p.write_text(json.dumps(package))
    # Even a newly hashed history entry cannot bless a mismatched package.
    with self.assertRaises(ValueError):inspect_build(completed_entry(self.root,self.app.game))
  p.write_bytes(original)
 def test_payload_format_cannot_be_blessed_by_new_hashes(self):
  self.prepared();p=self.app.game/'private/universal/DKRR-Update.elf';p.write_bytes(b'not an ELF payload')
  receipt=p.parent/'CANDIDATE.json';record=json.loads(receipt.read_text())
  record['payloads'][p.name]={'size':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()};receipt.write_text(json.dumps(record))
  with self.assertRaisesRegex(ValueError,'format'):inspect_build(completed_entry(self.root,self.app.game))
 def test_old_rom_containing_history_is_preserved_not_restored(self):
  self.prepared();history=self.app.history.path
  entry={'game':str(self.app.game),'source_sha256':source_identity(self.root),'installer_sha256':'0'*64,'receipt_sha256':self.app.completed['receipt_sha256']}
  history.write_text(json.dumps({'schema':1,'builds':[entry]}));before=history.read_bytes()
  self.assertFalse(module.Assistant(self.root,run=self.run_step).state['built'])
  self.assertEqual(history.read_bytes(),before)
 def test_changed_payload_fifo_is_rejected_without_reading(self):
  self.prepared();p=self.app.game/'private/universal/DKRR-Update.elf';old=p.read_bytes();p.unlink();os.mkfifo(p)
  with self.assertRaises(ValueError):inspect_build(self.app.completed)
  p.unlink();p.write_bytes(old)
 def test_http_install_and_update_use_universal_payloads(self):
  self.prepared();server=module.make_server(self.app)
  worker=threading.Thread(target=server.serve_forever,daemon=True);worker.start()
  base='http://127.0.0.1:'+str(server.server_port)
  headers={'X-Setup-Token':self.app.token,'Content-Type':'application/json'}
  try:
   for mode,name in [('install','DKRR-Installer.elf'),('update','DKRR-Update.elf')]:
    with self.subTest(mode=mode):
     before=len(self.commands)
     data=json.dumps({'host':'192.168.1.50','closed':True,'mode':mode}).encode()
     with urllib.request.urlopen(urllib.request.Request(base+'/install',data=data,headers=headers)) as response:
      self.assertEqual(response.status,202)
     deadline=time.monotonic()+3
     while self.app.status()['busy'] and time.monotonic()<deadline:time.sleep(.01)
     with urllib.request.urlopen(urllib.request.Request(base+'/status',headers=headers)) as response:state=json.load(response)
     self.assertFalse(state['busy']);self.assertEqual(state['error'],'');self.assertTrue(state['installed'])
     self.assertEqual(len(self.commands),before+1);self.assertEqual(pathlib.Path(self.commands[-1][0][-1]).name,name)
  finally:server.shutdown();server.server_close();worker.join()
 def test_rom_checker_and_http_security(self):
  with self.assertRaises(ValueError):self.app.accept(b'not a ROM')
  invalid=bytes.fromhex('80371240')+bytes(module.SIZE-4)
  with self.assertRaises(ValueError):self.app.accept(invalid)
  self.assertFalse(list((self.app.work/'private/inputs').glob('upload-*')))
  server=module.make_server(self.app);worker=threading.Thread(target=server.serve_forever,daemon=True);worker.start();base='http://127.0.0.1:'+str(server.server_port)
  try:
   with urllib.request.urlopen(base) as response:self.assertIn(b'Build Assistant',response.read())
   with self.assertRaises(urllib.error.HTTPError) as e:urllib.request.urlopen(base+'/status')
   self.assertEqual(e.exception.code,403);e.exception.close()
   headers={'X-Setup-Token':self.app.token}
   with urllib.request.urlopen(urllib.request.Request(base+'/status',headers=headers)) as response:self.assertFalse(json.load(response)['built'])
   for path,data in [('/rom',b'bad'),('/unknown',b'{}')]:
    with self.assertRaises(urllib.error.HTTPError) as e:urllib.request.urlopen(urllib.request.Request(base+path,data=data,headers=headers))
    self.assertEqual(e.exception.code,400);e.exception.close()
   bad=dict(headers,Host='attacker.example')
   with self.assertRaises(urllib.error.HTTPError) as e:urllib.request.urlopen(urllib.request.Request(base+'/status',headers=bad))
   self.assertEqual(e.exception.code,403);e.exception.close()
  finally:server.shutdown();server.server_close();worker.join()
if __name__=='__main__':unittest.main()
