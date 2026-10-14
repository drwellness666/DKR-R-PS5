#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Test the native resolver's stricter, no-link filesystem subset."""
import ctypes
import errno
from pathlib import Path
import subprocess
import tempfile

root=Path(__file__).resolve().parent.parent
with tempfile.TemporaryDirectory(prefix='dkrr-native-fs-') as directory:
    base=Path(directory)
    library=base/'filesystem.so'
    subprocess.run(['cc','-std=c11','-D_GNU_SOURCE','-D__PROSPERO__',
                    '-Drealpath=dkrr_ps5_realpath','-Dlstat=dkrr_ps5_lstat',
                    '-O2','-Wall','-Wextra','-Werror','-fPIC','-shared',
                    str(root/'src/ps5_realpath.c'),str(root/'src/ps5_lstat.c'),
                    '-o',str(library)],check=True)
    native=ctypes.CDLL(str(library),use_errno=True).dkrr_ps5_realpath
    native.argtypes=[ctypes.c_char_p,ctypes.c_void_p]
    native.restype=ctypes.c_void_p
    (base/'a/b').mkdir(parents=True)
    (base/'a/file').write_bytes(b'fixture')
    (base/'relative').symlink_to('a/b',target_is_directory=True)
    (base/'absolute').symlink_to(base/'a/b',target_is_directory=True)
    (base/'broken').symlink_to('missing')
    (base/'cycle').symlink_to('cycle')
    (base/'denied').mkdir(mode=0o000)
    successes={'a':base/'a','a/b/..':base/'a','a/../a/file':base/'a/file',
               './a//b/':base/'a/b','a/b/../../':base,'':base}
    failures={'missing':errno.ENOENT,'missing/..':errno.ENOENT,
              'a/file/child':errno.ENOTDIR,'a/file/..':errno.ENOTDIR,
              'relative':errno.ENOTSUP,'absolute':errno.ENOTSUP,
              'broken':errno.ENOTSUP,'cycle':errno.ENOTSUP,
              'relative/../file':errno.ENOTSUP,
              'denied/child':errno.EACCES}
    try:
        for suffix,expected in successes.items():
            output=ctypes.create_string_buffer(4096)
            assert native((str(base)+'/'+suffix).encode(),output)
            assert output.value.decode()==str(expected),(suffix,output.value,expected)
        for suffix,error in failures.items():
            output=ctypes.create_string_buffer(4096)
            ctypes.set_errno(0)
            assert not native((str(base)+'/'+suffix).encode(),output),suffix
            assert ctypes.get_errno()==error,(suffix,ctypes.get_errno(),error)
    finally:
        (base/'denied').chmod(0o700)
    print('PS5 native filesystem subset PASS: existing paths resolved; missing, non-directory, unreadable and symlink paths refused')
