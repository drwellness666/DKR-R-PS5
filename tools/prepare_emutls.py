#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Generate a scoped PS5 TLS ordering fix from the pinned LLVM SDK source."""
import hashlib
import pathlib
import sys

if len(sys.argv) != 3:
    raise SystemExit('Usage: prepare_emutls.py PINNED_SDK_EMUTLS OUTPUT_C')
source = pathlib.Path(sys.argv[1]).read_bytes()
expected = '86813748aa5668112a0a5891601f5ae36660294a749a013fef7c3bbbd805737c'
if hashlib.sha256(source).hexdigest() != expected:
    raise SystemExit('Refusing an unreviewed emutls source revision')
before = b'#ifdef __BIONIC__\n// There are 4 pthread key cleanup rounds'
if source.count(before) != 1:
    raise SystemExit('Unexpected emutls cleanup layout')
source = source.replace(before, b'#if defined(__BIONIC__) || defined(__PROSPERO__)\n// There are 4 pthread key cleanup rounds')
pathlib.Path(sys.argv[2]).write_bytes(source)
