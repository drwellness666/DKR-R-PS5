#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Prepare the stopped native runtime for another selected retail revision."""
import argparse, hashlib, json
from pathlib import Path


def compose(name, text):
    if name == 'recomp.cpp':
        needle = 'bool recomp::register_game(const recomp::GameEntry& entry) {'
        insertion = needle + '\n    if (dkrr_native_runtime_active()) return false;'
        old = 'game_roms.insert({ entry.game_id, entry });'
        new = 'game_roms.insert_or_assign(entry.game_id, entry);'
        if text.count(needle) != 1 or text.count(old) != 1:
            raise ValueError('Unexpected native game registry')
        start = 'void recomp::start(const recomp::Configuration& cfg) {'
        if text.count(start) != 1:
            raise ValueError('Unexpected native worker lifetime')
        lifetime = '''static std::atomic<bool> native_worker_active{false};
extern "C" int dkrr_native_runtime_active(void) {
    return native_worker_active.load(std::memory_order_acquire) ? 1 : 0;
}
struct NativeWorkerScope {
    NativeWorkerScope() {
        if (native_worker_active.exchange(true,std::memory_order_acq_rel))
            throw std::runtime_error("A native game worker is already active.");
    }
    ~NativeWorkerScope() { native_worker_active.store(false,std::memory_order_release); }
};
'''
        return '#include <atomic>\n#include <stdexcept>\n' + lifetime + text.replace(needle, insertion).replace(old, new).replace(start, start+'\n    NativeWorkerScope native_worker_scope;')
    if name == 'overlays.cpp':
        old = '    func_map.clear();\n    section_addresses = (int32_t *)calloc(sections_info.total_num_sections, sizeof(int32_t));'
        new = '''    // Called only after the previous guest and renderer have joined.
    // Indices and loaded ranges belong to one revision, never the next one.
    func_map.clear();
    loaded_sections.clear();
    code_sections_by_rom.clear();
    free(section_addresses);
    section_addresses = (int32_t *)calloc(sections_info.total_num_sections, sizeof(int32_t));
    if (!section_addresses) throw std::bad_alloc();'''
        if text.count(old) != 1:
            raise ValueError('Unexpected native overlay initialization')
        return '#include <new>\n' + text.replace(old, new)
    raise ValueError('Unexpected session source')


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('output', type=Path)
    p.add_argument('sources', type=Path, nargs=2)
    a = p.parse_args()
    a.output.mkdir(parents=True, exist_ok=True)
    records = []
    for old in a.sources:
        raw = old.read_text()
        text = compose(old.name, raw)
        new = a.output / old.name
        if not new.exists() or new.read_text() != text:
            new.write_text(text)
        records.append(dict(old=str(old), new=str(new), input_sha256=hashlib.sha256(raw.encode()).hexdigest(),
                            output_sha256=hashlib.sha256(text.encode()).hexdigest()))
    (a.output/'RECEIPT.json').write_text(json.dumps(records, indent=2)+'\n')
