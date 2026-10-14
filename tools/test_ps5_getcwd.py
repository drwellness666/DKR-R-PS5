#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Check physical working-directory discovery, including moved directories."""
import ctypes
import errno
import os
from pathlib import Path
import subprocess
import tempfile

root=Path(__file__).resolve().parent.parent
original=os.getcwd()
with tempfile.TemporaryDirectory(prefix='dkrr-getcwd-') as directory:
    base=Path(directory)
    library=base/'getcwd.so'
    subprocess.run(['cc','-std=c11','-D_GNU_SOURCE','-Dgetcwd=dkrr_ps5_getcwd',
                    '-Dlstat=dkrr_ps5_lstat','-Dpathconf=dkrr_ps5_pathconf','-O2','-Wall','-Wextra','-Werror',
                    '-fPIC','-shared',str(root/'src/ps5_getcwd.c'),
                    str(root/'src/ps5_lstat.c'),str(root/'src/ps5_pathconf.c'),'-o',str(library)],check=True)
    module=ctypes.CDLL(str(library),use_errno=True)
    native=module.dkrr_ps5_getcwd
    native.argtypes=[ctypes.c_void_p,ctypes.c_size_t]
    native.restype=ctypes.c_void_p
    libc=ctypes.CDLL(None)
    libc.free.argtypes=[ctypes.c_void_p]
    nested=base/'a/b/c'
    nested.mkdir(parents=True)
    (base/'alias').symlink_to(nested,target_is_directory=True)
    cases=0
    query=module.dkrr_ps5_pathconf
    query.argtypes=[ctypes.c_char_p,ctypes.c_int]
    query.restype=ctypes.c_long
    for selector in ('PC_PATH_MAX','PC_NAME_MAX'):
        number=os.pathconf_names[selector]
        assert query(str(base).encode(),number)==os.pathconf(base,number)
    ctypes.set_errno(0)
    assert query(str(base/'missing').encode(),os.pathconf_names['PC_PATH_MAX'])==-1
    assert ctypes.get_errno()==errno.ENOENT
    ctypes.set_errno(0)
    assert query(str(base).encode(),-999)==-1 and ctypes.get_errno()==errno.EINVAL
    try:
        for path in [Path('/'),base,nested,base/'alias']:
            os.chdir(path)
            expected=os.getcwd().encode()
            for size in [len(expected)+1,4096]:
                buffer=ctypes.create_string_buffer(size)
                assert native(buffer,size)
                assert buffer.value==expected
                cases+=1
            for size in [0,1,len(expected)]:
                buffer=ctypes.create_string_buffer(max(1,size))
                ctypes.set_errno(0)
                assert not native(buffer,size)
                assert ctypes.get_errno()==(errno.EINVAL if size==0 else errno.ERANGE)
                cases+=1
            for size in [0,8192]:
                pointer=native(None,size)
                assert pointer and ctypes.string_at(pointer)==expected
                libc.free(pointer)
                cases+=1
        os.chdir(nested)
        nested.rename(nested.parent/'renamed')
        buffer=ctypes.create_string_buffer(4096)
        assert native(buffer,len(buffer)) and buffer.value==os.getcwd().encode()
        cases+=1
    finally:
        os.chdir(original)
    print(f'PS5 getcwd host comparison PASS: {cases} buffer, allocation, root, nested, alias and renamed-directory cases')
