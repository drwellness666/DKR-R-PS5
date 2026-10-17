#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise the actual revision adapter with sparse non-ROM filesystem fixtures."""
import ctypes
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent

class State(ctypes.Structure):
    _fields_ = [(key, ctypes.c_uint) for key in ('active', 'preferred', 'compiled_mask', 'available_mask')]

with tempfile.TemporaryDirectory(prefix='dkrr-revisions-') as temporary:
    directory = Path(temporary)
    for mask in (1, 2, 3):
        data = directory / str(mask)
        (data / 'roms').mkdir(parents=True)
        (data / 'preferences').mkdir()
        pref = data / 'preferences/boot-revision.txt'
        legacy = data / 'boot-revision.txt'
        legacy.write_text('80\n')
        roms = [data / f'roms/dkr.us.v{revision}.z64' for revision in (77, 80)]
        for rom in roms:
            with rom.open('wb') as output:
                output.truncate(12 * 1024 * 1024)
        saves = data / 'profile-us-v77/saves/dkr.us.v77.bin'
        saves.parent.mkdir(parents=True)
        saves.write_bytes(b'fixture progress must stay intact')
        library = directory / f'revisions-{mask}.so'
        subprocess.run(['clang++', '-std=c++20', '-O1', '-Wall', '-Wextra', '-Werror',
                        '-shared', '-fPIC',
                        f'-DDKR_REVISION_DATA_ROOT="{data}"',
                        f'-DDKR_PAYLOAD_V77_ENABLED={int(bool(mask & 1))}',
                        f'-DDKR_PAYLOAD_V80_ENABLED={int(bool(mask & 2))}',
                        str(ROOT / 'src/ps5_revision_selection.cpp'), '-o', str(library)], check=True)
        api = ctypes.CDLL(str(library))
        api.dkrr_ps5_revision_state.restype = State
        api.dkrr_ps5_set_active_revision(80 if mask == 2 else 77)
        active = api.dkrr_ps5_active_revision()
        assert api.dkrr_ps5_initial_revision() == (80 if mask in (2, 3) else 77)
        legacy.unlink()
        for text, requested in [('77', 77), ('80\n', 80), ('1.0\r\n', 77), ('1.1\n', 80)]:
            pref.write_text(text)
            expected = requested if mask == 3 else (80 if mask == 2 else 77)
            assert api.dkrr_ps5_initial_revision() == expected
        for malformed in ['1.1junk', ' 80', '80\x00', '80\n\n', '', '12345678']:
            pref.write_text(malformed)
            assert api.dkrr_ps5_choose_revision(80 if mask == 2 else 77) == -1
            assert pref.read_text() == malformed
        pref.unlink()
        for requested, flag in ((77, 1), (80, 2)):
            rc = api.dkrr_ps5_choose_revision(requested)
            assert rc == (0 if mask & flag else -1)
            if rc == 0:
                assert pref.read_text() == ('1.0\n' if requested == 77 else '1.1\n')
                assert api.dkrr_ps5_active_revision() == active
        assert api.dkrr_ps5_choose_revision(99) == -1
        pref.unlink()
        unrelated = data / 'unrelated.txt'
        unrelated.write_text('keep')
        pref.symlink_to(unrelated)
        assert api.dkrr_ps5_choose_revision(80 if mask == 2 else 77) == -1
        assert unrelated.read_text() == 'keep' and pref.is_symlink()
        pref.unlink()
        pending = data / 'preferences/boot-revision.txt.next'
        pending.write_text('interrupted choice; preserve')
        assert api.dkrr_ps5_choose_revision(80 if mask == 2 else 77) == 0
        assert pending.read_text() == 'interrupted choice; preserve' and pref.is_file()
        pending.unlink()
        pref.unlink()
        roms[0].unlink()
        assert api.dkrr_ps5_choose_revision(77) == -1
        if mask == 3:
            assert api.dkrr_ps5_initial_revision() == 80
        roms[0].symlink_to(roms[1])
        assert api.dkrr_ps5_choose_revision(77) == -1
        roms[1].write_bytes(b'wrong size')
        assert api.dkrr_ps5_choose_revision(80) == -1
        assert saves.read_bytes() == b'fixture progress must stay intact'
        assert api.dkrr_ps5_revision_state().active == active
print('PASS: actual single/dual selection, legacy preferences, verified writes, missing ROMs, malformed/symlink/collision refusal and save preservation')
