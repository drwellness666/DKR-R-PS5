#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Plan/build an exact-previous-app update for the private PPSA94678 preview."""
from prepare_controller_audio import CUE_COUNT
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

TITLE = 'PPSA94678'
CORE = {'eboot.bin', 'sce_module/libc.prx', 'sce_sys/param.json',
        'sce_sys/icon0.png', 'sce_sys/pic0.dds', 'sce_sys/pic1.dds'}
LEGACY_ART = {'sce_sys/pic1.dds'}
PREVIOUS_CORE = {'eboot.bin', 'sce_module/libc.prx', 'sce_sys/param.json'}
ASSET_EXTENSIONS = {'.png', '.bmp', '.json', '.txt', '.hlsl', '.md'}

def inventory(app, *, previous=False):
    if not app.is_absolute() or app.is_symlink() or not app.is_dir():
        raise ValueError('Use an ordinary absolute private application directory')
    files = {}
    for path in sorted(app.rglob('*')):
        if path.is_symlink():
            raise ValueError('Symlinks forbidden in application inputs')
        if not path.is_file():
            if not path.is_dir():
                raise ValueError('Special application file forbidden')
            continue
        name = path.relative_to(app).as_posix()
        if len(('/data/homebrew/' + TITLE + '/' + name + '.dkrr-v9999-update').encode()) >= 256:
            raise ValueError('Application path exceeds native updater limit')
        owned_audio = name in {f'assets/controller-audio/us-v{revision}/{i}.pcm' for revision in (77,80) for i in range(CUE_COUNT)}
        if name not in CORE and not owned_audio and not (previous and name in LEGACY_ART) and not (name.startswith('assets/') and path.suffix.lower() in ASSET_EXTENSIONS):
            raise ValueError('Unapproved application input: ' + name)
        if any(c in str(path) for c in '\n\r"\\'):
            raise ValueError('Unsupported input path')
        data = path.read_bytes()
        files[name] = {'size': len(data), 'sha256': hashlib.sha256(data).hexdigest(),
                       'sha1': hashlib.sha1(data).hexdigest(),
                       'mode': 0o755 if name in ('eboot.bin', 'sce_module/libc.prx') else 0o644}
    required = PREVIOUS_CORE if previous else CORE
    if not required <= files.keys():
        raise ValueError('Incomplete application: previous needs executable/module/identity; current needs icon and selection background too')
    if json.loads((app / 'sce_sys/param.json').read_text()).get('titleId') != TITLE:
        raise ValueError('Unexpected application identity')
    with (app / 'eboot.bin').open('rb') as stream:
        if stream.read(4) != bytes.fromhex('4f153d1d'):
            raise ValueError('Expected private native FSELF executable')
    return files

def inventory_hash(files):
    # Content/path identity is independent of local host timestamps or paths.
    return hashlib.sha256(json.dumps(files, sort_keys=True, separators=(',', ':')).encode()).hexdigest()

def make_plan(previous, current):
    # Earlier previews predate selection/launch artwork. Missing old artwork
    # is added by the collision-refusing installer after the common-file update.
    old, new = inventory(previous, previous=True), inventory(current)
    common = sorted(old.keys() & new.keys())
    changed = [name for name in common if old[name]['sha256'] != new[name]['sha256']]
    added = sorted(new.keys() - old.keys())
    retained = sorted(old.keys() - new.keys())
    # Metadata and executable are committed after their required assets.
    order = sorted(common, key=lambda name: (name == 'eboot.bin', name == 'sce_sys/param.json', name))
    return {'schema': 1, 'title_id': TITLE, 'previous_manifest_sha256': inventory_hash(old),
            'current_manifest_sha256': inventory_hash(new), 'changed_files': changed,
            'preflight_files': order, 'new_files_for_installer': added,
            'obsolete_files_preserved': retained,
            'launch_poster_retirement_required': 'sce_sys/pic1.dds' in retained,
            'limits': ['No installation, title launch or console connection is performed by this builder.',
                       'Every common file must match recorded old or new bytes before any update.',
                       'The runtime commits each changed file atomically; the whole bundle is not an atomic transaction.',
                       'After updating, use the collision-refusing installer for additional files.',
                       'Compare the recorded preceding build to the console before deploying; local files alone do not establish console identity.']}, old, new

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('sdk', type=Path)
    parser.add_argument('previous', type=Path)
    parser.add_argument('current', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--plan-only', action='store_true')
    parser.add_argument('--expected-previous-manifest-sha256')
    parser.add_argument('--sequence', type=int, default=17)
    args = parser.parse_args()
    if not 1 <= args.sequence <= 9999:
        parser.error('Sequence must be between 1 and 9999')
    if any(not path.is_absolute() for path in (args.sdk, args.previous, args.current, args.output)):
        parser.error('Use absolute input/output paths')
    plan, old, new = make_plan(args.previous, args.current)
    if args.plan_only:
        print(json.dumps(plan, indent=2))
        return
    if args.expected_previous_manifest_sha256 != plan['previous_manifest_sha256']:
        parser.error('Recorded previous manifest differs; inspect --plan-only first')
    if 'private' not in args.output.parts or args.output.exists() or args.output.is_symlink():
        parser.error('Use a new output ELF under a private build directory')
    root = Path(__file__).resolve().parent.parent
    gpu = Path(os.environ.get('DKR_GPU_WORK', str(root / 'work/vulkan')))
    work = args.output.parent / (args.output.stem + '-source')
    work.mkdir(parents=True, exist_ok=False)
    source = (gpu / 'PS5_PayloadSDK/crt/nid.c').read_text().replace('nid_encode(', 'dkrr_update_unused_nid_encode(')
    source += '''\n#include <errno.h>
extern long read(int, void *, unsigned long);
int dkrr_sha1_fd(int fd, unsigned char digest[20]) {
 SHA1_CTX context; unsigned char bytes[8192]; long got; SHA1Init(&context);
 for (;;) {
  got=read(fd,bytes,sizeof(bytes)); if(got<0 && errno==EINTR)continue;
  if(got<0)return -1; if(got==0)break;
  SHA1Update(&context,bytes,(unsigned int)got);
 }
 SHA1Final(digest,&context); return 0;
}
'''
    (work / 'sdk_sha1_file.c').write_text(source)
    assembly = '.section .rodata\n'
    header = 'struct update_file { const char *path; const unsigned char *start; unsigned long long old_size,new_size; const char *old_sha1,*new_sha1; unsigned mode; };\n'
    entries = []
    for index, name in enumerate(plan['preflight_files']):
        symbol = '0'
        if name in plan['changed_files']:
            symbol = f'native_update_{index}_start'
            header += f'extern const unsigned char {symbol}[];\n'
            assembly += f'.balign 16\n.global {symbol}\n{symbol}:\n.incbin "{args.current / name}"\n'
        entries.append('{"/data/homebrew/' + TITLE + '/' + name + '",' + symbol +
                       f',{old[name]["size"]},{new[name]["size"]},"{old[name]["sha1"]}","{new[name]["sha1"]}",{new[name]["mode"]:04o}' + '}')
    header += 'static const struct update_file update_files[]={\n' + ',\n'.join(entries) + '\n};\n'
    (work / 'native_update_files.h').write_text(header)
    (work / 'embedded.S').write_text(assembly)
    (work / 'update-plan.json').write_text(json.dumps(plan, indent=2) + '\n')
    cc = str(args.sdk / 'bin/prospero-clang')
    for file in ('sdk_sha1_file.c', 'embedded.S'):
        subprocess.run([cc, '-O2', '-c', str(work / file), '-o', str(work / (file + '.o'))], check=True)
    subprocess.run([cc, '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', f'-DDKR_UPDATE_VERSION={args.sequence}',
                    '-I', str(work), str(root / 'src/update_native_probe.c'), str(work / 'sdk_sha1_file.c.o'),
                    str(work / 'embedded.S.o'), '-o', str(args.output)], check=True)
    print('Built private own-title bundle updater:', args.output)
    print('Changed files:', len(plan['changed_files']), '; checked common files:', len(plan['preflight_files']))
    print('No console change. Generated ELF contains private ROM-derived game code; exclude from publication.')

if __name__ == '__main__':
    main()
