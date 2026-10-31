#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Reuse the original data-only import dispatcher inside the native game."""
from pathlib import Path
import argparse,hashlib,json
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();root=Path(__file__).resolve().parents[1]
source=a.source/'runtime-recomp/ps5-release/native-base/src/game/mods/legacy_mod_worker.cpp';original=source.read_text();anchor='#if defined(_WIN32)\nint wmain'
if original.count(anchor)!=1:raise ValueError('Worker entry point changed')
text=original[:original.index(anchor)]
text=text.replace('        constrain_import_worker();','        dkrr::imports::checkpoint();').replace('        sandbox_import_worker(request.parent_path());','        // Native import uses the existing bounded data parsers, not process limits.')
needle='        const auto report=[&](const Progress& progress) {'
if text.count(needle)!=1:raise ValueError('Worker progress contract changed')
text=text.replace(needle,needle+'\n            dkrr::imports::checkpoint();')
text='#include "ps5_embedded_importer.hpp"\n#include "ps5_import_control.hpp"\n#include <algorithm>\n#include <mutex>\n#include <thread>\n#include <sys/stat.h>\n'+text+'\n#include "ps5_embedded_importer.inl"\n'
a.output.mkdir(parents=True,exist_ok=True);target=a.output/'embedded_importer.cpp'
if not target.exists() or target.read_text()!=text:target.write_text(text)
(a.output/'RECEIPT.json').write_text(json.dumps(dict(schema=1,source=str(source),input_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),output_sha256=hashlib.sha256(text.encode()).hexdigest(),worker='native data import; existing parser sizes and transaction publication retained'),indent=2)+'\n')
