#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise the prepared SDL PS5 driver's actual rumble code, without hardware."""
import pathlib,subprocess,sys,tempfile
if len(sys.argv)!=2:raise SystemExit('Usage: test_controller_rumble.py ABSOLUTE_SDL_WORK')
root=pathlib.Path(__file__).resolve().parent.parent
source=pathlib.Path(sys.argv[1]).resolve()/'src/ps5-SDL/src/joystick/ps5/SDL_ps5joystick.c'
text=source.read_text();start=text.index('static int PS5_JoystickRumble(')
end=text.index('\nstatic int PS5_JoystickSetLED',start)
with tempfile.TemporaryDirectory(prefix='dkrr-rumble-') as folder:
    folder=pathlib.Path(folder);(folder/'ps5_rumble.inc').write_text(text[start:end])
    out=folder/'rumble'
    subprocess.run(['clang','-std=c11','-Wall','-Wextra','-Werror','-I',str(folder),
                    str(root/'tests/controller_rumble.c'),'-o',str(out)],check=True)
    subprocess.run([str(out)],check=True)
