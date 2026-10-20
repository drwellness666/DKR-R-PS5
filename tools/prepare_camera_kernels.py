#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Generate revision-specific upstream draw-only kernels from owned CPU inputs."""
from pathlib import Path
import argparse
import hashlib
import importlib.util
import json
import shutil
import sys


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def prepare(game, pipeline, builds):
    source = game / 'source/DKR-R'
    private = game / 'private'
    recipe = source / 'patches/camera-plus/scene_kernel_pipeline.py'
    spec = importlib.util.spec_from_file_location('dkrr_camera_generator', recipe)
    generator = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(generator)
    # The upstream manifest uses root-relative provenance. Keep private copies
    # under our own private build root, rather than writing into source inputs.
    generator.ROOT = private
    generator.new_output.__globals__['ROOT'] = private
    header = game / 'source/N64Recomp/include/recomp.h'
    for revision in (77, 80):
        target = private / 'build' / ('camera-v' + str(revision))
        if target.exists():
            verify(target)
            record = json.loads((target / 'PS5-KERNEL-RECEIPT.json').read_text())
            originals = pipeline / ('generated-v' + str(revision))
            current = {p.name:sha(p) for p in sorted(originals.iterdir())
                       if p.is_file() and p.suffix in ('.c', '.h')}
            if (record['generator_sha256'] != sha(recipe) or
                    record['source_cpu_files'] != current or
                    sha(header) != generator.HEADER_HASH or
                    record['files']['camera_shadow_kernel.c'] != sha(source / 'patches/camera-shadow/camera_shadow_kernel.c')):
                raise ValueError('Camera kernel inputs changed; use a fresh game build directory')
            continue
        inputs = private / 'build' / ('camera-input-v' + str(revision))
        inputs.mkdir(parents=True, mode=0o700, exist_ok=True)
        originals = pipeline / ('generated-v' + str(revision))
        for path in originals.iterdir():
            if path.is_file() and path.suffix in ('.c', '.h'):
                shutil.copy2(path, inputs / path.name)
        elf = builds[revision] / ('dkr.us.v' + str(revision) + '.elf')
        generator.generate(inputs, header, elf, revision, target)
        generator.verify(target)
        base = (target / 'manifest.json').read_bytes()
        hooks = target / 'camera_scene_hooks.c'
        text = hooks.read_text()
        for suffix in ('begin', 'end'):
            old = (f'unsigned dkr_camera_v{revision}_dkr_shadow_interpolation_{suffix}'
                   '(uint8_t* ram,recomp_context* ctx,...)'
                   '{(void)ram;(void)ctx;return 0;}\n')
            if text.count(old) != 1:
                raise ValueError('Camera shadow hook differs from the pinned recipe')
            text = text.replace(old, '')
        hooks.write_text(text)
        shadow = source / 'patches/camera-shadow/camera_shadow_kernel.c'
        shutil.copy2(shadow, target / shadow.name)
        record = dict(schema=1, revision=revision, generator_sha256=sha(recipe),
                      original_manifest_sha256=hashlib.sha256(base).hexdigest(),
                      source_cpu_files={p.name:sha(p) for p in sorted(inputs.iterdir())},
                      files={p.name:sha(p) for p in sorted(target.iterdir()) if p.is_file()},
                      transformations='Pinned upstream shadow-sidecar hook replacement',
                      draw_only=True, logic_ticks=False)
        (target / 'PS5-KERNEL-RECEIPT.json').write_text(json.dumps(record, indent=2) + '\n')
        verify(target)


def verify(target):
    record = json.loads((target / 'PS5-KERNEL-RECEIPT.json').read_text())
    files = {p.name:sha(p) for p in target.iterdir() if p.is_file() and p.name != 'PS5-KERNEL-RECEIPT.json'}
    if files != record['files']:
        raise ValueError('Prepared PS5 camera kernel differs from its receipt')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('game', type=Path)
    parser.add_argument('pipeline', type=Path)
    parser.add_argument('--v77-build', type=Path, required=True)
    parser.add_argument('--v80-build', type=Path, required=True)
    args = parser.parse_args()
    prepare(args.game.resolve(), args.pipeline.resolve(), {77:args.v77_build, 80:args.v80_build})
    print('PASS both revision-specific draw-only camera/shadow kernels')
