#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Check real source staging and package planning without installing host packages."""
import hashlib,pathlib,sys,tempfile,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from stage_windows_source import stage
from host_requirements import package_plan
class Staging(unittest.TestCase):
    def fixture(self,root):
        source=root/'Windows source ü & spaces';source.mkdir()
        names=['setup.py','Start-Linux.sh','tools/start_build_assistant.sh','tools/setup_ui.html','vendor/public.txt']
        for name in names:
            p=source/name;p.parent.mkdir(exist_ok=True);p.write_text('public '+name)
        (source/'SHA256SUMS').write_text(''.join(hashlib.sha256((source/n).read_bytes()).hexdigest()+'  '+n+'\n' for n in names))
        (source/'vendor/SHA256SUMS').write_text('fixture inventory\n')
        (source/'private').mkdir();(source/'private/owned-fixture.txt').write_text('must stay outside staged source')
        return source
    def test_inventory_only_and_reuse(self):
        with tempfile.TemporaryDirectory() as temp:
            p=pathlib.Path(temp);s=self.fixture(p);dest=stage(s,p/'linux')
            self.assertEqual((dest/'setup.py').read_bytes(),(s/'setup.py').read_bytes())
            self.assertFalse((dest/'private').exists());self.assertEqual(stage(s,p/'linux'),dest)
            (dest/'setup.py').write_text('tampered')
            with self.assertRaises(ValueError):stage(s,p/'linux')
    def test_source_and_cached_parent_symlinks(self):
        with tempfile.TemporaryDirectory() as temp:
            p=pathlib.Path(temp);s=self.fixture(p);dest=stage(s,p/'linux')
            (dest/'tools').rename(p/'redirect');(dest/'tools').symlink_to(p/'redirect',target_is_directory=True)
            with self.assertRaises(ValueError):stage(s,p/'linux')
            (s/'tools').rename(p/'source-redirect');(s/'tools').symlink_to(p/'source-redirect',target_is_directory=True)
            with self.assertRaises(ValueError):stage(s,p/'other-linux')
    def test_invalid_inventory_and_packages(self):
        with tempfile.TemporaryDirectory() as temp:
            p=pathlib.Path(temp);s=self.fixture(p)
            with (s/'SHA256SUMS').open('a') as f:f.write('0'*64+'  ../private.txt\n')
            with self.assertRaises(ValueError):stage(s,p/'linux')
        ubuntu=package_plan({'ID':'ubuntu','ID_LIKE':'debian'});self.assertEqual(ubuntu[:2],['apt-get','install'])
        for tool in ('python3','python3-venv','cmake','clang','libclang-dev','llvm-dev','lld','libpcre2-dev','spirv-tools'):
            self.assertIn(tool,ubuntu)
        self.assertEqual(package_plan({'ID':'cachyos','ID_LIKE':'arch'})[0],'pacman')
        self.assertEqual(package_plan({'ID':'fedora'})[0],'dnf')
        self.assertEqual(package_plan({'ID':'unknown'}),[])
if __name__=='__main__':unittest.main()
