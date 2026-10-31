#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Retain pinned SLJIT; select the platform SDK's executable memory chunks."""
import argparse,hashlib,json
from pathlib import Path
def stage(source,output):
    raw=source.read_bytes()
    expected='3d3d4ba39fe892995643eb6e68a54a67f0b631c622c80879de43146cb4b92303'
    if hashlib.sha256(raw).hexdigest()!=expected:raise ValueError('Unreviewed SLJIT source')
    text=raw.decode();needle='#if defined(_WIN32)\n#include "allocator_src/sljitExecAllocatorWindows.c"'
    if text.count(needle)!=1:raise ValueError('Unexpected SLJIT allocator selection')
    text=text.replace(needle,'#if defined(__PROSPERO__) || defined(DKRR_TEST_NATIVE_EXEC)\n#include "ps5_sljit_exec_allocator.inl"\n#elif defined(_WIN32)\n#include "allocator_src/sljitExecAllocatorWindows.c"')
    output.mkdir(parents=True,exist_ok=True);dest=output/source.name
    if not dest.exists() or dest.read_text()!=text:dest.write_text(text)
    (output/'RECEIPT.json').write_text(json.dumps(dict(source=str(source),input_sha256=expected,output_sha256=hashlib.sha256(text.encode()).hexdigest()),indent=2)+'\n')
    return dest
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('source',type=Path);p.add_argument('output',type=Path);a=p.parse_args();stage(a.source.resolve(),a.output.resolve())
