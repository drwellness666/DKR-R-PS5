#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Check the compiler's actual include graph and the linked PS5 display UI."""
import argparse
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('game',type=Path);a=p.parse_args()
g=a.game.resolve();build=g/'game-ps5';domain=build/'native-domain'
dependency=build/'CMakeFiles/DKRPortGame.dir/native-domain/src/game/runtime_ui.cpp.o.d'
text=dependency.read_text().replace('\\\n',' ')
expected=str(domain/'src/game/runtime_graphics_ui.inl')
wrong=str(domain/'stages/upstream-sync/graphics.inl')
if expected not in text or wrong in text:
    raise SystemExit('The actual UI object still includes the upstream graphics alias instead of the final PS5 page')
obj=dependency.with_suffix('').read_bytes()
for desktop_label in (b'Window mode\0',b'Manual target (advanced)\0'):
    if desktop_label in obj:raise SystemExit('Desktop graphics control remains in the actual PS5 UI object: '+desktop_label.decode())
raw=(g/'native-link/llvm-pie.elf').read_bytes()
for label in (b'Active output:',b'TV output resolution',b'TV refresh & smoothness',b'HDR output'):
    if label not in raw:raise SystemExit('Missing linked PS5 display control: '+label.decode())
print('PASS actual compiler UI dependencies and linked native output/status controls; physical layout remains a console check')
