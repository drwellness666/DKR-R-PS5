# SPDX-License-Identifier: GPL-3.0-or-later
"""Actual HTTP auth/upload bounds, build ordering, isolation and install completion gates."""
import importlib.util,io,json,pathlib,tempfile,threading,urllib.request,urllib.error,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('assistant',ROOT/'setup.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
class Tests(unittest.TestCase):
 def setUp(self):
  self.tmp=tempfile.TemporaryDirectory();self.root=pathlib.Path(self.tmp.name);(self.root/'tools').mkdir();(self.root/'tools/setup_ui.html').write_text('<title>Build Assistant</title>');self.commands=[]
  self.app=module.Assistant(self.root,run=self.run_step)
 def tearDown(self):self.tmp.cleanup()
 def run_step(self,argv,env):
  self.commands.append((argv,env.copy()))
  if argv[1:2]==['tools/record_native_build.py']:
   p=self.app.game/'private';p.mkdir(parents=True);(p/'CANDIDATE.json').write_text('{}')
  if argv[1:2]==['tools/build_native_game_installer.py']:(self.app.game/'private/dkrr-install-game-1.0.6-preview.elf').write_bytes(b'\x7fELFfixture')
  if argv[1:2]==['tools/build_owned_rom_installer.py']:(self.app.game/'private/dkrr-install-owned-rom.elf').write_bytes(b'\x7fELFfixture')
  if argv[:2]==['bash','tools/build_save_metadata.sh']:
   (self.root/'bin').mkdir(exist_ok=True);(self.root/'bin/dkrr-save-metadata.elf').write_bytes(b'\x7fELFfixture')
  if 'tools/send_elf.py' in argv:return 'OWN ROM INSTALL PASS.' if 'owned-rom' in argv[3] else 'NATIVE INSTALL PASS.'
  return ''
 def test_orchestration_and_safe_retries(self):
  self.app.state['roms']={'77':{'revision':'US v1.0'},'80':{'revision':'US Rev A'}};self.app.preflight=lambda:{'ok':True}
  self.app.build(2);first=self.app.game
  self.assertTrue(self.app.state['built']);self.assertEqual(self.commands[3][0],['bash','build_integrated.sh']);env=self.commands[3][1]
  self.assertEqual(env['DKR_GPU_JOBS'],'2');self.assertNotIn('DKR_PIPELINE',env);self.assertNotIn('DKR_ROM',env)
  self.app.build(2);self.assertNotEqual(first,self.app.game);self.assertTrue((first/'private/CANDIDATE.json').exists())
  self.app.install('192.168.1.50',True);self.assertTrue(self.app.state['installed'])
  sends=[x[0] for x in self.commands if 'tools/send_elf.py' in x[0]];self.assertEqual(len(sends),3);self.assertEqual(sends[-1][-1],'--no-wait')
 def test_single_revision_and_install_guards(self):
  self.app.state['roms']={'80':{'revision':'US Rev A'}};env=self.app.environment(4);self.assertIn('DKR_ROM',env);self.assertNotIn('DKR_ROM_V77',env)
  for host,closed in [('8.8.8.8',True),('127.0.0.1',True),('0.0.0.0',True),('192.168.1.50',False)]:
   with self.assertRaises(ValueError):self.app.install(host,closed)
  self.app.state['built']=True;self.app.runner=lambda *a:'Transfer complete.'
  with self.assertRaises(RuntimeError):self.app.install('192.168.1.50',True)
  self.assertFalse(self.app.state['installed'])
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
