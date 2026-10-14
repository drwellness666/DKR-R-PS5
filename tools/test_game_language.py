#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Check actual upstream language encoding without ROMs, saves or a console."""
import pathlib,subprocess,sys,tempfile
if len(sys.argv)!=2:raise SystemExit('Usage: test_game_language.py ABSOLUTE_GAME_WORK')
root=pathlib.Path(__file__).resolve().parent.parent
game=pathlib.Path(sys.argv[1]).resolve()/'source/DKR-R/runtime-recomp/src/game'
with tempfile.TemporaryDirectory(prefix='dkrr-language-') as folder:
    out=pathlib.Path(folder)/'language'
    subprocess.run(['clang++','-std=c++20','-Wall','-Wextra','-Werror','-I',str(game),'-I',str(root/'src'),
                    str(root/'tests/game_language.cpp'),str(game/'dkr_save_codec.cpp'),'-o',str(out)],check=True)
    subprocess.run([str(out)],check=True)
