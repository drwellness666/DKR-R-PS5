#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Actual patching beneath an outer Git root; bad-context/no-change refusal."""
import hashlib,pathlib,subprocess,tempfile
from apply_dependency_patch import apply_dependency_patch
with tempfile.TemporaryDirectory(prefix='dkrr-patch-scope-') as folder:
    outer=pathlib.Path(folder);subprocess.run(['git','init','--quiet',str(outer)],check=True)
    target=outer/'work/source/dependency';(target/'include').mkdir(parents=True);file=target/'include/context.h';file.write_text('line one\nold type\nline three\n')
    patch=outer/'change.patch';patch.write_text('diff --git a/include/context.h b/include/context.h\n--- a/include/context.h\n+++ b/include/context.h\n@@ -1,3 +1,3 @@\n line one\n-old type\n+named type\n line three\n')
    before={str(p.relative_to(outer/'.git')):p.read_bytes() for p in (outer/'.git').rglob('*') if p.is_file()}
    receipt=apply_dependency_patch(target,patch);assert file.read_text()=='line one\nnamed type\nline three\n'
    assert receipt['files'][0]['after_sha256']==hashlib.sha256(file.read_bytes()).hexdigest();assert not (target/'.git').exists()
    after={str(p.relative_to(outer/'.git')):p.read_bytes() for p in (outer/'.git').rglob('*') if p.is_file()};assert before==after
    changed=file.read_bytes()
    try:apply_dependency_patch(target,patch)
    except subprocess.CalledProcessError:pass
    else:raise AssertionError('Already applied/bad context accepted')
    assert file.read_bytes()==changed
    for text in ('not a patch\n','diff --git a/../outside b/../outside\n','diff --git a/.git/config b/.git/config\n'):
        patch.write_text(text)
        try:apply_dependency_patch(target,patch)
        except ValueError:pass
        else:raise AssertionError('Unsafe/empty patch accepted')
        assert file.read_bytes()==changed
    file.unlink();file.symlink_to(outer/'change.patch');patch.write_text('diff --git a/include/context.h b/include/context.h\n')
    try:apply_dependency_patch(target,patch)
    except ValueError:pass
    else:raise AssertionError('Symlink dependency accepted')
    print('PASS actual nested-Git patching, verified byte changes, outer repo preservation, bad-context/traversal/symlink refusal')
