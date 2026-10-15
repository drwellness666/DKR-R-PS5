#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise the actual installer C on private temporary files, never a console."""
import os
import pathlib
import subprocess
import tempfile

root = pathlib.Path(__file__).resolve().parent.parent
with tempfile.TemporaryDirectory(prefix='dkrr-installer-') as temp:
    work = pathlib.Path(temp)
    parent = work / 'own-data'
    parent.mkdir(mode=0o755)
    journal = parent / 'diagnostics'
    identity = work / 'param.json'
    identity.write_bytes(b'qualified-own-title\n')
    identity.chmod(0o644)
    # Exactly the support helper's one byte-guarded identity, using the real C
    # implementation. No save/game/ROM input and no privileged operations.
    (work / 'native_game_files.h').write_text(
        'struct native_file {const char *path;const unsigned char *start,*end;unsigned mode;};\n'
        'static const unsigned char identity[]="qualified-own-title\\n";\n'
        'static const char *native_dirs[]={"'+str(parent)+'","'+str(journal)+'"};\n'
        'static const struct native_file native_files[]={{"'+str(identity)+'",identity,identity+sizeof(identity)-1,0644}};\n')
    binary = work / 'installer'
    subprocess.run(['clang', '-std=c11', '-D_POSIX_C_SOURCE=200809L', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-I'+str(work),
                    '-DDKR_DIAGNOSTICS_DIRECTORY="'+str(journal)+'"',
                    str(root / 'src/install_native_game.c'), '-o', str(binary)], check=True)
    env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0', UBSAN_OPTIONS='halt_on_error=1')
    def run(expected):
        result = subprocess.run([str(binary)], capture_output=True, env=env)
        assert result.returncode == expected, (result.returncode, result.stdout, result.stderr)
    run(0)
    assert journal.stat().st_mode & 0o7777 == 0o1777
    assert parent.stat().st_mode & 0o7777 == 0o755
    old = journal / 'existing.log'
    old.write_bytes(b'keep-existing-session\n'); old.chmod(0o600)
    before = old.stat()
    run(0)
    assert old.read_bytes() == b'keep-existing-session\n'
    assert old.stat().st_mtime_ns == before.st_mtime_ns and old.stat().st_mode & 0o7777 == 0o600
    journal.chmod(0o700); run(0)
    assert journal.stat().st_mode & 0o7777 == 0o700
    identity.write_bytes(b'unknown-title\n'); run(2)
    assert identity.read_bytes() == b'unknown-title\n' and old.read_bytes() == b'keep-existing-session\n'
    old.unlink(); journal.rmdir()
    target = work / 'unrelated'; target.mkdir(mode=0o750)
    journal.symlink_to(target, target_is_directory=True); run(1)
    assert target.stat().st_mode & 0o7777 == 0o750 and not list(target.iterdir())
print('PASS actual installer: sticky new journal leaf, unchanged parent/existing permissions and files, identity collision and symlink refusal')
