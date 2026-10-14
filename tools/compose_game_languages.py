#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Checked US retail Options language cycle; no PAL dispatch or save-file edits."""
import copy
import struct

# Exact sites from both matching US ELFs, not a region/version address delta.
SITES = {
    'us.v77': (0x80084520, (0x8008453C, 0x8008454C)),
    'us.v80': (0x80084958, (0x80084974, 0x80084984)),
}

def compose(policy, elf, revision):
    from compose_legacy_mod_policy import elf_functions, elf_sections
    from legacy_character_presentation_policy import reviewed_revision
    if revision not in SITES or reviewed_revision(elf) != revision:
        raise ValueError('Language cycle requires an exact reviewed US ELF')
    functions = elf_functions(elf)
    def function(name):
        choices = functions.get(name, set())
        if len(choices) != 1:
            raise ValueError('Ambiguous language function: ' + name)
        return next(iter(choices))
    caller, caller_size = function('menu_options_loop')
    getter, getter_size = function('get_language')
    setter, _ = function('set_language')
    words = {base + offset: struct.unpack_from('>I', data, offset)[0]
             for base, data in elf_sections(elf) for offset in range(0, len(data), 4)}
    getter_call, calls = SITES[revision]
    expected = {getter_call: 0x0C000000 | ((getter >> 2) & 0x03FFFFFF),
                getter_call + 4: 0, getter_call + 8: 0x000267C3,
                calls[0]: 0x0C000000 | ((setter >> 2) & 0x03FFFFFF),
                calls[0] + 4: 0x24040002,
                calls[1]: 0x0C000000 | ((setter >> 2) & 0x03FFFFFF),
                calls[1] + 4: 0x00002025,
                getter_call - 8: 0x10E00016, getter_call - 4: 0}
    for address, value in expected.items():
        if not caller <= address < caller + caller_size or words.get(address) != value:
            raise ValueError(f'Language site signature changed at {address:08X}')
    # The exact reviewed getter contains no nested call. a3/analogueX remains
    # live; full ELF identity is the authoritative register/data-flow guard.
    for address in range(getter, getter + getter_size, 4):
        opcode = words[address] >> 26
        if opcode == 3:
            raise ValueError('Language getter gained a nested call')
    result = copy.deepcopy(policy)
    for pc in calls:
        hook = dict(function='menu_options_loop', beforeVram=hex(pc),
                    text='extern void dkr_european_language_cycle(uint8_t*, recomp_context*); '
                         'dkr_european_language_cycle(rdram, ctx);',
                    reason='Expose intact English/French/German through the normal retail setter, preserving EEPROM save ownership.')
        patch = dict(function='menu_options_loop', vram=hex(pc + 4), value='0x00000000',
                     reason='Preserve the checked language argument supplied before JAL; do not overwrite it in the delay slot.')
        for key, row, address_key in [('functionHooks', hook, 'beforeVram'),
                                      ('instructionPatches', patch, 'vram')]:
            entries = result.setdefault(key, [])
            existing = [entry for entry in entries if int(entry[address_key], 0) == int(row[address_key], 0)]
            if existing and existing != [row]:
                raise ValueError('Conflicting language hook/patch')
            if not existing:
                entries.append(row)
    return result
