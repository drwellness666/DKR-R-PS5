#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Synthetic codec vectors; no original sound or ROM data."""
import itertools,os,pathlib,subprocess,sys,tempfile,unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
from vadpcm_decode import decode
from prepare_controller_audio import Bank
class Decode(unittest.TestCase):
    def test_signed_residuals_and_vector_state(self):
        frame=bytes.fromhex('00 01 23 45 67 89 ab cd ef')
        samples=list(range(8))+list(range(-8,0))
        self.assertEqual(decode(frame*2,2,1,[0]*16),samples*2)
        self.assertEqual(decode(frame*2,2,1,[0]*8+[2048]*8),list(itertools.accumulate(samples*2)))
    def test_saturation_and_bad_input(self):
        self.assertEqual(decode(bytes([0xf0])+b'\x7f'*8,2,1,[0]*16),[32767,-32768]*8)
        for data,order,predictors,book in [(b'\x01'+b'\0'*8,2,1,[0]*16),(b'\0'*8,2,1,[0]*16),(b'\0'*9,3,1,[0]*24)]:
            with self.assertRaises(ValueError):decode(data,order,predictors,book)
        with self.assertRaises(ValueError):Bank(b'not a ROM',0)
class Jingle(unittest.TestCase):
    def test_native_bounded_sequence_parser(self):
        root=pathlib.Path(__file__).resolve().parents[1]
        with tempfile.TemporaryDirectory(prefix='dkrr-cseq-') as folder:
            out=pathlib.Path(folder)/'jingle'
            subprocess.run(['clang','-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(root/'tests/controller_jingle.c'),str(root/'installer/vadpcm.c'),'-lm','-o',str(out)],check=True)
            subprocess.run([str(out)],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
if __name__=='__main__':unittest.main()
