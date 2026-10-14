#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Compare the PS5 filesystem resolver with host libc in isolated fixtures."""
import ctypes
import itertools
import pathlib
import subprocess
import tempfile

root=pathlib.Path(__file__).resolve().parent.parent
with tempfile.TemporaryDirectory(prefix='dkrr-realpath-') as directory:
    base=pathlib.Path(directory)
    library=base/'resolver.so'
    subprocess.run(['cc','-std=c11','-D_GNU_SOURCE','-Drealpath=dkrr_ps5_realpath',
                    '-O2','-Wall','-Wextra','-Werror','-fPIC','-shared',
                    str(root/'src/ps5_realpath.c'),'-o',str(library)],check=True)
    ours=ctypes.CDLL(str(library),use_errno=True).dkrr_ps5_realpath
    libc=ctypes.CDLL(None,use_errno=True)
    reference=libc.realpath
    for function in (ours,reference):
        function.argtypes=[ctypes.c_char_p,ctypes.c_void_p]
        function.restype=ctypes.c_void_p
    libc.free.argtypes=[ctypes.c_void_p]
    (base/'a/b').mkdir(parents=True)
    (base/'a/file').write_bytes(b'fixture')
    (base/'relative').symlink_to('a/b',target_is_directory=True)
    (base/'absolute').symlink_to(base/'a/b',target_is_directory=True)
    (base/'up').symlink_to('a/b/..',target_is_directory=True)
    (base/'broken').symlink_to('missing')
    (base/'cycle').symlink_to('cycle')
    (base/'filelink').symlink_to('a/file')
    (base/'denied').mkdir(mode=0o000)
    paths=['', '/', '///', '.', '..', str(base), 'x'*4096]
    elements=['a','b','file','.','..','relative','absolute','up','broken','cycle','filelink','missing','denied','']
    paths.extend(str(base)+'/'+'/'.join(parts)
                 for size in range(1,4)
                 for parts in itertools.product(elements,repeat=size))
    def result(function,path,allocate):
        buffer=None if allocate else ctypes.create_string_buffer(4096)
        ctypes.set_errno(0)
        pointer=function(path.encode(),buffer)
        error=ctypes.get_errno()
        if not pointer:
            return (False,error)
        value=ctypes.string_at(pointer)
        if allocate:
            libc.free(pointer)
        return (True,value)
    try:
        for path in paths:
            for allocate in (False,True):
                expected=result(reference,path,allocate)
                actual=result(ours,path,allocate)
                assert actual==expected,(path,allocate,expected,actual)
    finally:
        (base/'denied').chmod(0o700)
    print(f'PS5 realpath host comparison PASS: {len(paths)*2} existing, missing, symlink, cycle, permission and allocation cases')
