#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Host acceptance of bundle planning and the actual C updater; no ROM/console."""
import hashlib
import json
from pathlib import Path
import subprocess
import tarfile
import tempfile
from build_native_game_bundle_updater import CORE, inventory_hash, make_plan

root = Path(__file__).resolve().parent.parent
with tempfile.TemporaryDirectory(prefix='dkrr-bundle-update-') as directory:
    work = Path(directory)
    previous, current, destination = (work / name for name in ('previous', 'current', 'destination'))
    for app in (previous, current):
        for name in CORE:
            path = app / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(b'old-fixture-' + name.encode())
        (app / 'sce_sys/param.json').write_text(json.dumps({'titleId': 'PPSA94678'}))
        (app / 'eboot.bin').write_bytes(bytes.fromhex('4f153d1d') + b'old-synthetic-code')
    (current / 'eboot.bin').write_bytes(bytes.fromhex('4f153d1d') + b'new-synthetic-code')
    (current / 'sce_sys/icon0.png').write_bytes(b'new-icon-fixture')
    (current / 'assets/licenses').mkdir(parents=True)
    (current / 'assets/licenses/new.txt').write_text('new fixture notice')
    plan, old, new = make_plan(previous, current)
    assert set(plan['changed_files']) == {'eboot.bin', 'sce_sys/icon0.png'}
    assert plan['new_files_for_installer'] == ['assets/licenses/new.txt']
    assert plan['preflight_files'][-1] == 'eboot.bin'
    assert plan['previous_manifest_sha256'] == inventory_hash(old)
    for name in ('sce_sys/icon0.png', 'sce_sys/pic0.dds'):
        saved = (previous / name).read_bytes()
        (previous / name).unlink()
        legacy, _, _ = make_plan(previous, current)
        assert name in legacy['new_files_for_installer'] and name not in legacy['preflight_files']
        try:
            make_plan(current, previous)
            raise AssertionError('Incomplete new app accepted')
        except ValueError:
            pass
        (previous / name).write_bytes(saved)
    paired, _, _ = make_plan(previous,current)
    assert not paired['launch_poster_retirement_required']
    assert paired['obsolete_files_preserved']==[]
    (current / 'assets/link.png').symlink_to(current / 'sce_sys/icon0.png')
    try:
        make_plan(previous, current)
        raise AssertionError('Symlink accepted')
    except ValueError:
        pass
    (current / 'assets/link.png').unlink()
    (current / 'assets/rom.z64').write_bytes(b'fixture')
    try:
        make_plan(previous, current)
        raise AssertionError('ROM input accepted')
    except ValueError:
        pass
    (current / 'assets/rom.z64').unlink()
    with tarfile.open(root / 'vendor/ps5-sdk-source.tar.gz') as archive:
        data = archive.extractfile('crt/nid.c').read().decode()
    data = data.replace('nid_encode(', 'dkrr_update_unused_nid_encode(')
    data += '''\n#include <errno.h>
#include <unistd.h>
int dkrr_sha1_fd(int fd, unsigned char digest[20]) {
 SHA1_CTX context; unsigned char bytes[8192]; ssize_t got; SHA1Init(&context);
 for (;;) { got=read(fd,bytes,sizeof(bytes)); if(got<0 && errno==EINTR)continue;
  if(got<0)return -1; if(!got)break; SHA1Update(&context,bytes,(unsigned)got); }
 SHA1Final(digest,&context); return 0;
}
'''
    (work / 'sha1.c').write_text(data)
    header = 'struct update_file { const char *path; const unsigned char *start; unsigned long long old_size,new_size; const char *old_sha1,*new_sha1; unsigned mode; };\n'
    entries = []
    for index, name in enumerate(plan['preflight_files']):
        payload = (current / name).read_bytes()
        header += f'static const unsigned char data_{index}[]={{' + ','.join(str(b) for b in payload) + '};\n'
        entries.append('{' + json.dumps(str(destination / name)) + f',data_{index},{old[name]["size"]},{new[name]["size"]},"{old[name]["sha1"]}","{new[name]["sha1"]}",{new[name]["mode"]:04o}' + '}')
    header += 'static const struct update_file update_files[]={\n' + ',\n'.join(entries) + '\n};\n'
    (work / 'native_update_files.h').write_text(header)
    # Keep upstream SDK warnings separate from project code's strict checks.
    subprocess.run(['clang', '-std=c11', '-D_GNU_SOURCE', '-O2', '-fsanitize=address,undefined',
                    '-c', str(work / 'sha1.c'), '-o', str(work / 'sha1.o')], check=True)
    subprocess.run(['clang', '-std=c11', '-D_GNU_SOURCE', '-O2', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-I', str(work), str(root / 'src/update_native_probe.c'),
                    str(work / 'sha1.o'), '-o', str(work / 'updater')], check=True)
    def reset():
        for name in plan['preflight_files']:
            path = destination / name
            path.parent.mkdir(parents=True, exist_ok=True)
            if path.is_symlink():
                path.unlink()
            path.write_bytes((previous / name).read_bytes())
    def contents():
        return {name: (destination / name).read_bytes() for name in plan['preflight_files']}
    def run(success):
        result = subprocess.run([str(work / 'updater')], capture_output=True, text=True)
        assert (result.returncode == 0) == success, result.stdout + result.stderr
    reset()
    run(True)
    assert contents() == {name: (current / name).read_bytes() for name in plan['preflight_files']}
    assert (destination / 'eboot.bin').stat().st_mode & 0o777 == 0o755
    run(True)  # Repeating an already complete update must be harmless.
    reset()
    (destination / 'sce_module/libc.prx').write_bytes(b'foreign module')
    before = contents()
    run(False)
    assert contents() == before  # Unknown unchanged dependency blocks every change.
    reset()
    (destination / 'sce_sys/icon0.png').unlink()
    (destination / 'sce_sys/icon0.png').symlink_to(current / 'sce_sys/icon0.png')
    old_code = (destination / 'eboot.bin').read_bytes()
    run(False)
    assert (destination / 'eboot.bin').read_bytes() == old_code
    reset()
    temp = destination / 'sce_sys/icon0.png.dkrr-v4-update'
    temp.write_text('existing unrelated temp')
    before = contents()
    run(False)
    assert contents() == before and temp.read_text() == 'existing unrelated temp'
    temp.unlink()
    # A partly completed known bundle can be resumed: old/new bytes are accepted.
    (destination / 'sce_sys/icon0.png').write_bytes((current / 'sce_sys/icon0.png').read_bytes())
    run(True)
    assert contents() == {name: (current / name).read_bytes() for name in plan['preflight_files']}
    reset()
    displaced = work / 'displaced-destination'
    destination.rename(displaced)
    destination.symlink_to(displaced, target_is_directory=True)
    before = contents()
    run(False)
    assert contents() == before
    destination.unlink()
    displaced.rename(destination)
print('PASS: actual native updater under ASan/UBSan; full preflight, changed art/code, repeat/resume, unknown files, symlinks and temp collision refusal')
