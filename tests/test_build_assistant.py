# SPDX-License-Identifier: GPL-3.0-or-later
"""Actual HTTP auth/upload bounds, build ordering, isolation and install completion gates."""
import hashlib,importlib.util,io,json,pathlib,subprocess,tempfile,threading,urllib.request,urllib.error,unittest
from unittest.mock import patch
ROOT=pathlib.Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('assistant',ROOT/'setup.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
class Tests(unittest.TestCase):
 def setUp(self):
  self.tmp=tempfile.TemporaryDirectory();self.root=pathlib.Path(self.tmp.name);(self.root/'tools').mkdir();(self.root/'tools/setup_ui.html').write_text('<title>Build Assistant</title>');self.commands=[]
  self.fixture_rom=b'local synthetic test data'.ljust(module.SIZE,b'0')
  self.rom_patch=patch.dict(module.ROM_HASHES,{77:hashlib.sha1(self.fixture_rom).hexdigest(),80:hashlib.sha1(self.fixture_rom).hexdigest()});self.rom_patch.start()
  self.app=module.Assistant(self.root,run=self.run_step)
 def tearDown(self):self.rom_patch.stop();self.tmp.cleanup()
 def run_step(self,argv,env):
  self.commands.append((argv,env.copy()))
  if argv[1:2]==['tools/record_native_build.py']:
   p=self.app.game/'private';p.mkdir(parents=True)
   app=p/'native/PPSA94678';(app/'sce_sys').mkdir(parents=True);(app/'sce_module').mkdir()
   build='integrated-'+'a'*24
   for name in ('icon0.png','pic0.dds','pic1.dds'):(app/'sce_sys'/name).write_bytes(b'art')
   (app/'eboot.bin').write_bytes(bytes.fromhex('4f153d1d')+build.encode())
   (app/'sce_module/libc.prx').write_bytes(b'module')
   (app/'sce_sys/param.json').write_text(json.dumps({'titleId':'PPSA94678','attribute3':0x40040}))
   link=self.app.game/'native-link';link.mkdir();(link/'native-inputs.json').write_text(json.dumps({'build_id':build}))
   (link/'program.c').write_text('const char id[]=\"'+build+'\"; int main(void){return id[0]==0;}')
   subprocess.run(['clang','-g','-fPIE','-pie',str(link/'program.c'),'-o',str(link/'llvm-pie.elf')],check=True)
   (p/'roms').mkdir()
   for rev in (77,80):(p/'roms'/f'dkr.us.v{rev}.z64').write_bytes(self.fixture_rom)
   (p/'roms/selected-revisions.json').write_text(json.dumps({'revisions':[77,80]}))
   from record_native_build import record
   record(self.app.game)
  if argv[1:2]==['tools/build_native_game_installer.py']:(self.app.game/'private/dkrr-install.elf').write_bytes(b'\x7fELFfixture')
  if argv[1:2]==['tools/build_owned_rom_installer.py']:(self.app.game/'private/dkrr-install-owned-rom.elf').write_bytes(b'\x7fELFfixture')
  if argv[:2]==['bash','tools/build_save_metadata.sh']:
   (self.root/'bin').mkdir(exist_ok=True);(self.root/'bin/dkrr-save-metadata.elf').write_bytes(b'\x7fELFfixture')
  if 'tools/build_native_game_deployer.py' in argv:
   out=pathlib.Path(argv[5]);out.parent.mkdir(parents=True);out.write_bytes(b'\x7fELFupdate fixture')
  if 'tools/send_elf.py' in argv:return 'DKR-R UPDATE PASS.' if 'dkrr-update' in argv[3] else 'DKR-R INSTALL PASS.'
  return ''
 def test_orchestration_and_safe_retries(self):
  self.app.state['roms']={'77':{'revision':'US v1.0'},'80':{'revision':'US Rev A'}};self.app.preflight=lambda:{'ok':True}
  self.app.build(2);first=self.app.game
  self.assertTrue(self.app.state['built']);self.assertEqual(self.commands[3][0],['bash','build_integrated.sh']);env=self.commands[3][1]
  self.assertEqual(env['DKR_GPU_JOBS'],'2');self.assertNotIn('DKR_PIPELINE',env);self.assertNotIn('DKR_ROM',env)
  self.app.build(2);self.assertNotEqual(first,self.app.game);self.assertTrue((first/'private/CANDIDATE.json').exists())
  self.app.install('192.168.1.50',True);self.assertTrue(self.app.state['installed'])
  sends=[x[0] for x in self.commands if 'tools/send_elf.py' in x[0]];self.assertEqual(len(sends),1);self.assertTrue(sends[0][3].endswith('/dkrr-install.elf'))
 def test_single_revision_and_install_guards(self):
  self.app.state['roms']={'80':{'revision':'US Rev A'}}
  with self.assertRaises(ValueError):self.app.environment(4)
  self.app.state['roms']['77']={'revision':'US v1.0'};env=self.app.environment(4);self.assertNotIn('DKR_ROM',env);self.assertIn('DKR_ROM_V77',env)
  for host,closed in [('8.8.8.8',True),('127.0.0.1',True),('0.0.0.0',True),('192.168.1.50',False)]:
   with self.assertRaises(ValueError):self.app.install(host,closed)
  self.app.preflight=lambda:{'ok':True};self.app.build(2);self.app.runner=lambda *a:'Transfer complete.'
  with self.assertRaises(RuntimeError):self.app.install('192.168.1.50',True)
  self.assertFalse(self.app.state['installed'])
 def prepared(self):
  self.app.state['roms']={'77':{'revision':'US 1.0'},'80':{'revision':'US Rev A'}};self.app.preflight=lambda:{'ok':True};self.app.build(2)
 def test_recover_completed_build_and_update(self):
  self.prepared();game=self.app.game;entry=self.app.completed.copy();before=len(self.commands)
  restored=module.Assistant(self.root,run=self.run_step)
  self.assertTrue(restored.state['built']);self.assertFalse(restored.state['installed']);self.assertEqual(restored.game,game)
  self.assertEqual(restored.inputs['77'],game/'private/roms/dkr.us.v77.z64');self.assertEqual(len(self.commands),before)
  restored.install('192.168.1.50',True,entry['receipt_sha256'])
  self.assertTrue(restored.state['installed'])
  self.assertEqual(self.commands[-2][0][1],'tools/build_native_game_deployer.py');self.assertTrue(self.commands[-1][0][3].endswith('dkrr-update.elf'))
  self.assertEqual(len([x for x in self.commands[before:] if 'tools/send_elf.py' in x[0]]),1)
  self.assertEqual((game/'private/dkrr-install.elf').read_bytes(),b'\x7fELFfixture')
 def test_changed_outputs_never_reused_or_sent(self):
  self.prepared();game=self.app.game
  for relative in ('private/dkrr-install.elf','private/native/PPSA94678/eboot.bin','native-link/llvm-pie.elf','private/roms/dkr.us.v77.z64'):
   p=game/relative;old=p.read_bytes();p.write_bytes(old+b'changed')
   self.assertFalse(module.Assistant(self.root,run=self.run_step).state['built'])
   before=len(self.commands)
   with self.assertRaises(ValueError):self.app.install('192.168.1.50',True)
   self.assertEqual(before,len(self.commands));p.write_bytes(old)
 def test_source_change_preserves_previous_but_disables_reuse(self):
  self.prepared();(self.root/'build_game.sh').write_text('changed build recipe')
  restored=module.Assistant(self.root,run=self.run_step)
  self.assertFalse(restored.state['built']);self.assertEqual(len(restored.state['previous_builds']),1)
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
 def test_unknown_predecessor_refused(self):
  self.prepared();before=len(self.commands)
  with self.assertRaises(ValueError):self.app.install('192.168.1.50',True,'unrecognized')
  self.assertEqual(before,len(self.commands))
 def test_rom_checker_and_http_security(self):
  with self.assertRaises(ValueError):self.app.accept(b'not a ROM')
  invalid=bytes.fromhex('80371240')+bytes(module.SIZE-4)
  with self.assertRaises(ValueError):self.app.accept(invalid)
  self.assertFalse(list((self.app.work/'private/inputs').glob('upload-*')))
  server=module.make_server(self.app);worker=threading.Thread(target=server.serve_forever,daemon=True);worker.start();base='http://127.0.0.1:'+str(server.server_port)
  try:
   with urllib.request.urlopen(base) as response:self.assertIn(b'Build Assistant',response.read())
   with self.assertRaises(urllib.error.HTTPError) as e:urllib.request.urlopen(base+'/status')
   self.assertEqual(e.exception.code,403)
   headers={'X-Setup-Token':self.app.token}
   with urllib.request.urlopen(urllib.request.Request(base+'/status',headers=headers)) as response:self.assertFalse(json.load(response)['built'])
   for path,data in [('/rom',b'bad'),('/unknown',b'{}')]:
    with self.assertRaises(urllib.error.HTTPError) as e:urllib.request.urlopen(urllib.request.Request(base+path,data=data,headers=headers))
    self.assertEqual(e.exception.code,400)
   bad=dict(headers,Host='attacker.example')
   with self.assertRaises(urllib.error.HTTPError) as e:urllib.request.urlopen(urllib.request.Request(base+'/status',headers=bad))
   self.assertEqual(e.exception.code,403)
  finally:server.shutdown();server.server_close();worker.join()
if __name__=='__main__':unittest.main()
