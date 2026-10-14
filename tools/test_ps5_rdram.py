#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parent.parent
with tempfile.TemporaryDirectory(prefix='dkrr-rdram-') as directory:
    binary=Path(directory)/'model'
    subprocess.run(['cc','-std=c11','-D_GNU_SOURCE','-DDKR_RDRAM_HOST_TEST',
                    '-DDKRR_RDRAM_RESERVE_BYTES=16777216ULL','-DDKRR_RDRAM_COMMIT_BYTES=2097152ULL',
                    '-O2','-Wall','-Wextra','-Werror','-pthread','-I',str(root/'tests'),
                    str(root/'src/ps5_rdram.c'),str(root/'tests/ps5_rdram_model.c'),
                    '-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
