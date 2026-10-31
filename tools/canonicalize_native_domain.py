#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Give capsule aliases and final native sources one consistent header identity.

The upstream cache overlays share a source domain. The PS5 tarball composer
must reconstruct that domain too: pragma-once cannot join copied header paths.
No input file is changed; compiler paths point at fresh, recorded copies.
"""
import argparse,hashlib,json,re,importlib.util
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source',type=Path,required=True);p.add_argument('--build',type=Path,required=True);a=p.parse_args()
source,build=a.source.resolve(),a.build.resolve();domain=build/'native-domain';domain.mkdir(exist_ok=True)
base=source/'runtime-recomp/ps5-release/native-base';final=build/'generated/ps5-final';suffixes={'.c','.cpp','.h','.hpp','.inl','.inc'}
patched={Path(row['logical']).relative_to('runtime-recomp') for row in json.loads((final/'swaps.json').read_text())}
records={};planned={}
def sha(data):return hashlib.sha256(data).hexdigest()
def logical(path):
    if path.is_relative_to(base):return path.relative_to(base)
    normal=source/'runtime-recomp'
    if path.is_relative_to(normal) and path.relative_to(normal).parts[0] in ('src','include','tests'):return path.relative_to(normal)
    roles=normal/'ps5-release/roles'
    if path.is_relative_to(roles):
        parts=path.relative_to(roles).parts[1:]
        if parts and parts[0]=='native':parts=parts[1:]
        if parts and parts[0] in ('src','include','tests'):
            key=Path(*parts);original=base/key
            if key in patched or (original.is_file() and path.is_file() and path.read_bytes()==original.read_bytes()):return key
    return None
# The immutable capsule supplies all unaffected source/header files.
for sub in ('src','include','tests'):
    for path in (base/sub).rglob('*'):
        if path.is_file() and path.suffix in suffixes:planned[domain/path.relative_to(base)]=path
# Native changes are already exact-hash verified after the official layers.
for key in patched:planned[domain/key]=final/'runtime-recomp'/key
# Preserve every official feature's local generated include graph.
for path in (build/'generated').rglob('*'):
    if path.is_file() and path.suffix in suffixes and not path.is_relative_to(final):planned[domain/'stages'/path.relative_to(build/'generated')]=path
# Preserve non-identical retained role headers separately; never merge ABIs.
for path in (source/'runtime-recomp/ps5-release/roles').rglob('*'):
    if path.is_file() and path.suffix in suffixes and logical(path) is None:
        planned[domain/'roles'/path.relative_to(source/'runtime-recomp/ps5-release/roles')]=path

def mapped(path):
    path=Path(path)
    if path.is_relative_to(final/'runtime-recomp'):return domain/path.relative_to(final/'runtime-recomp')
    key=logical(path)
    if key is not None:return domain/key
    if path.is_relative_to(build/'generated'):return domain/'stages'/path.relative_to(build/'generated')
    roles=source/'runtime-recomp/ps5-release/roles'
    if path.is_relative_to(roles):return domain/'roles'/path.relative_to(roles)
    # Include directory roots need not themselves be regular files.
    if path==source/'runtime-recomp/src':return domain/'src'
    return path
# Absolute include paths are source adapters' recorded links. Relative includes
# keep their directory layout, and CMake remaps the matching search directories.
inc=re.compile(r'^(\s*#\s*include\s*")([^"\n]+)(")',re.M)
def upstream_adapter(script):
    file=source/'patches/release-prep/v1.0.9'/script
    spec=importlib.util.spec_from_file_location('dkrr_'+script.replace('.','_'),file)
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module
snapshots=upstream_adapter('android_snapshot.py');shared=upstream_adapter('android_shared.py')
from stage_ps5_detail_metrics import compose as native_detail_metrics
from stage_ps5_render_waits import compose as native_render_waits
from stage_ps5_ui_wording import compose as native_ui_wording
from stage_ps5_texture_io import compose as native_texture_io
for dest,path in sorted(planned.items()):
    text=path.read_text()
    if path.name=='online_directory.cpp':text=snapshots.compose(path,domain).decode()
    elif path.name in ('vehicle_binding.cpp','achievements.cpp'):text=shared.compose(path).decode()
    elif path.name=='texture_gallery.inl':text=text.replace('std::to_string(stamp.modified.time_since_epoch().count())','dkr::release::file_stamp(stamp.modified)')
    elif path.name=='texture_dump.cpp':text='#include "ps5_store_compat.hpp"\n'+text
    elif path.name=='detail_metrics.hpp':text=native_detail_metrics(text)
    elif path.name=='runtime_legacy_mods.cpp':
        text='#if defined(__PROSPERO__)\n#include "ps5_custom_horn.hpp"\nvoid dkrr_controller_mod_horn(const std::uint8_t*,unsigned,const dkrr::ps5_speaker::HornBank&);\n#endif\n'+text
        before='        return state->presentation.sound(state->roster,state->selector->entries(),player,native_character,sound);'
        after='        const auto mapped=state->presentation.sound(state->roster,state->selector->entries(),player,native_character,sound);\n#if defined(__PROSPERO__)\n        // Feed the admitted bank, rather than guessing a retail horn ID.\n        // The optional speaker worker decodes it; guest audio is untouched.\n        if(mapped!=sound && sound==0x156+native_character && player<4) {\n            const auto& entries=state->selector->entries();\n            const auto found=std::ranges::find(entries,state->roster.active(player),&dkr::mods::AllocatedCharacter::id);\n            if(found!=entries.end()) {\n                const auto& bank=found->race_audio;const auto& cue=bank.cues[16];\n                dkrr_controller_mod_horn(rdram,player,{state,bank.control,bank.samples,cue.sound,cue.volume,cue.pitch});\n            }\n        }\n#endif\n        return mapped;'
        if text.count(before)!=1:raise ValueError('Custom horn presentation interface changed')
        text=text.replace(before,after)
        before='    launch.store(nullptr);characters.store(nullptr);menu.store(nullptr);session.store(std::move(value));'
        after=before+'\n#if defined(__PROSPERO__)\n    for(auto& source:dkrr::ps5_speaker::custom_horns)source.store(nullptr);\n#endif'
        if text.count(before)!=1:raise ValueError('Custom horn session lifecycle changed')
        text=text.replace(before,after)
    elif path.name=='record_store.inl':
        before='    if(!path.empty()&&!storage::writable_path(path,error))return false;'
        after='''#if defined(__PROSPERO__)
    // Native title stat can omit nlink. Read existing custom records under
    // the same scoped lease used by atomic record writes; never fabricate
    // metadata or allow an unleased generic save-storage inspection.
    std::shared_ptr<storage::Lease> owner;
    if(!path.empty()){owner=storage::acquire(path,error);if(!owner)return false;}
#else
    if(!path.empty()&&!storage::writable_path(path,error))return false;
#endif'''
        if text.count(before)!=1:raise ValueError('Custom record admission changed')
        text=text.replace(before,after)
    elif path.name=='rt64_present_queue.cpp' and path.is_relative_to(build/'generated/ps5-hdr-renderer'):text=native_render_waits(text)
    elif path.name=='directory_tls.hpp':
        needle='inline std::string directory_ca_bundle(){'
        if text.count(needle)!=1:raise ValueError('Directory TLS interface changed')
        text='#include "ps5_https.hpp"\n'+text.replace(needle,needle+'\n#if defined(__PROSPERO__)\n    return std::string(dkrr::https::trusted_roots(),dkrr::https::trusted_roots_size());\n#endif',1)

    if path.name=='achievement_ui.inl':
        # The original fixed 100px label clips at console font scaling.
        text=text.replace('SettingsButton("Keyboard",100.F)', 'SettingsButton("TYPE",100.F)')

    if path.name=='magic_code_dispatch.h' and 'extern DKR_CODE_TLS' in text:
        # These two POD TLS variables are zero-initialized in runtime_code_mods.
        # State that guarantee explicitly: native conversion cannot import
        # Clang's optional, undefined C++ TLS dynamic-initializer thunks.
        if text.count('#define DKR_CODE_TLS thread_local')!=1:raise ValueError('Magic-code TLS declaration changed')
        text=text.replace('#define DKR_CODE_TLS thread_local','#define DKR_CODE_TLS constinit thread_local')

    if path.name in ('legacy_mod_import.cpp','legacy_mod_format.cpp','legacy_mod_stage.cpp'):
        text='#include "ps5_import_control.hpp"\n'+text
        # Every public bounded operation and archive iteration can stop the
        # native job before another allocation, file or decoded member.
        if path.name=='legacy_mod_format.cpp':
            text=text.replace('Bytes read_file(const std::filesystem::path& path, std::size_t limit) {','Bytes read_file(const std::filesystem::path& path, std::size_t limit) {\n    dkrr::imports::checkpoint();')
            text=text.replace('void write_new_file(const std::filesystem::path& path, View bytes) {','void write_new_file(const std::filesystem::path& path, View bytes) {\n    dkrr::imports::checkpoint();')
            text=text.replace('while(written<bytes.size()) {','while(written<bytes.size()) {\n        dkrr::imports::checkpoint();')
        else:
            text=re.sub(r'(^[A-Za-z][^;\n]*\)\s*\{)',r'\1\n    dkrr::imports::checkpoint();',text,flags=re.M)
    text=native_ui_wording(path.name,text)
    if path.name in ('runtime_texture_packs.cpp','store_texture_adapter.inl'):text=native_texture_io(text)
    def include_path(m):
        path=Path(m[2])
        if m[2].startswith('../../../../modding/'):
            reference=source/'runtime-recomp/ps5-release/roles/SDK2/modding'/m[2][len('../../../../modding/'):]
            if not reference.is_file():raise ValueError('Missing exact SDK symbol table')
            return m[1]+str(mapped(reference))+m[3]
        return m[1]+str(mapped(path))+m[3] if path.is_absolute() else m[0]
    text=inc.sub(include_path,text)
    dest.parent.mkdir(parents=True,exist_ok=True)
    if not dest.exists() or dest.read_text()!=text:dest.write_text(text)
    records[str(dest.relative_to(domain))]=dict(input=str(path),input_sha256=sha(path.read_bytes()),output_sha256=sha(text.encode()))
# Retained relative includes (for example ../../../../modding) also use
# role directory search paths. Keep those paths, with links to the single
# canonical header file rather than another pragma-once copy.
roles=source/'runtime-recomp/ps5-release/roles'
for path in roles.rglob('*'):
    if path.is_file() and path.suffix in suffixes:
        key=logical(path)
        if key is None:continue
        alias=domain/'roles'/path.relative_to(roles);target=domain/key
        alias.parent.mkdir(parents=True,exist_ok=True)
        if alias.is_symlink():
            if alias.resolve()!=target.resolve():alias.unlink()
            else:continue
        if alias.exists():
            if alias.read_bytes()!=target.read_bytes():raise ValueError('Inconsistent source-domain alias')
            alias.unlink()
        alias.symlink_to(target)

# Longest prefixes first. File-specific role aliases retain their verified map.
cmake=['# Generated source-domain paths; no source input was modified.','set(_dkrr_domain "'+str(domain)+'")']
for path in (source/'runtime-recomp/ps5-release/roles').rglob('*'):
    if path.is_file() and path.suffix in suffixes:
        dest=mapped(path);key=hashlib.sha256(str(path).encode()).hexdigest()
        cmake.append('set_property(GLOBAL PROPERTY DKRR_DOMAIN_'+key+' "'+str(dest)+'")')
cmake+=['function(dkrr_native_domain_path input output)',
        '  string(SHA256 key "${input}")',
        '  get_property(exact GLOBAL PROPERTY "DKRR_DOMAIN_${key}")',
        '  if(exact)',
        '    set(${output} "${exact}" PARENT_SCOPE)',
        '    return()',
        '  endif()',
        '  set(value "${input}")']
for prefix,replacement in ((final/'runtime-recomp',domain),(base,domain),(source/'runtime-recomp/src',domain/'src'),(source/'runtime-recomp/include',domain/'include'),(source/'runtime-recomp/tests',domain/'tests'),(build/'generated',domain/'stages'),(source/'runtime-recomp/ps5-release/roles',domain/'roles')):
    cmake.append('  string(REPLACE "'+str(prefix)+'" "'+str(replacement)+'" value "${value}")')
cmake+=['  set(${output} "${value}" PARENT_SCOPE)','endfunction()']
(domain/'paths.cmake').write_text('\n'.join(cmake)+'\n');(domain/'RECEIPT.json').write_text(json.dumps(dict(schema=1,files=records),indent=2)+'\n')
print('PASS canonical native source/header domain:',len(records),'recorded files')
