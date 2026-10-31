#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Fingerprint native inputs locally; this private inventory is never published."""
import argparse,hashlib,json,pathlib
def identify(root,game,sdl,support,high):
    groups={'source':root/'src','installer':root/'installer','patches':root/'patches','game':game/'game-ps5',
            'native-https':game/'native-https','native-dns':game/'native-dns','native':support/'tooling/native','sdl':sdl/'build/sdl',
            'samplerate':sdl/'build/samplerate','licenses':root/'licenses',
            'game-assets':game/'game-ps5/assets','title-assets':game/'private/title-art-paired'}
    rows=[]
    for name,folder in groups.items():
        for file in sorted(folder.rglob('*')):
            if not file.is_file():continue
            if name=='native-dns' and file.name not in ('libcares.a','DNS-RECEIPT.json'):continue
            if name=='native-https' and file.name not in ('libcurl.a','HTTPS-RECEIPT.json','ps5_tls_roots.generated.h'):continue
            if name in ('game','sdl','samplerate') and file.suffix!='.a' and file.name not in ('dkr-ps5-profile.json','dkr-release.json'):continue
            if name in ('source','installer','patches','native') and file.suffix not in ('.c','.cpp','.h','.hpp','.inl','.patch','.json','.vert','.frag'):continue
            rows.append({'path':name+'/'+str(file.relative_to(folder)),'size':file.stat().st_size,'sha256':hashlib.sha256(file.read_bytes()).hexdigest()})
    for relative in ('VULKAN_SOURCES.json','PORT_RELEASE.json','GAME_SOURCES.json','tools/compose_upstream_release.py','tools/prepare_camera_kernels.py','tools/StagePS5Release.cmake','tools/StagePS5NextRelease.cmake','tools/StagePS5Network.cmake','tools/stage_ps5_network.py','tools/StagePS5Session.cmake','tools/StagePS5NativePolicy.cmake','tools/stage_ps5_sljit.py','tools/stage_ps5_session.py','tools/verify_native_ui.py','src/ps5_runtime_session.h','tools/canonicalize_native_domain.py','tools/stage_ps5_detail_metrics.py','tools/stage_ps5_render_waits.py','tools/stage_ps5_ui_wording.py','tools/stage_ps5_texture_io.py','tools/stage_embedded_importer.py','tools/prepare_owned_kernels.py','tools/stage_ps5_release_final.py','tools/prepare_native_https.py','tools/prepare_native_dns.py','tools/NativeSocketTarget.cmake','tools/verify_native_network.py','HTTPS_SOURCES.json','vendor/Mozilla-CA-20261024.pem','tools/link_native_game.sh','tools/native_build_identity.py','tools/generate_native_import_check.py','tools/build_universal_installer.py','tools/StagePS5HDR.cmake','tools/build_hdr_shaders.py','tools/apply_ps5_hdr_driver.py','vendor/ps5-sdk-source.tar.gz'):
        file=root/relative;rows.append({'path':relative,'size':file.stat().st_size,'sha256':hashlib.sha256(file.read_bytes()).hexdigest()})
    # Record the exact generated source domain in addition to compiled archives.
    for relative in ('ps5-release-graph.json','native-domain/RECEIPT.json','native-network/RECEIPT.json','native-session/RECEIPT.json','native-exec/RECEIPT.json',
                     'generated/ps5-final/swaps.json',
                     'generated/ps5-embedded-importer/RECEIPT.json'):
        file=game/'game-ps5'/relative
        if file.is_file():rows.append({'path':'game-provenance/'+relative,'size':file.stat().st_size,'sha256':hashlib.sha256(file.read_bytes()).hexdigest()})
    gpu=support.parent
    for relative in ('LICENSE','THIRD_PARTY.md','TITLE_ART_SOURCES.json','homebrew/DKR-R-Game/ARTWORK.md',
                     'tools/verify_component_notices.py','tools/prepare_title_art.sh',
                     'tools/prepare_upstream_title_art.py','tools/validate_title_art.py'):
        file=root/relative
        if file.is_file():rows.append({'path':relative,'size':file.stat().st_size,'sha256':hashlib.sha256(file.read_bytes()).hexdigest()})
    dependencies=[gpu/'radv-build/src/amd/vulkan/libvulkan_radeon.a',gpu/'native-title/libc.prx',
                  gpu/'native-title/ps5-native-tool',support/'sce_sys/param-radv.json']+sorted((support/'.deps/native/ps5-payload-sdk/target/lib').glob('*'))
    for file in dependencies:
        if file.is_file():rows.append({'path':'gpu/'+str(file.relative_to(gpu)),'size':file.stat().st_size,'sha256':hashlib.sha256(file.read_bytes()).hexdigest()})
    record={'schema':1,'high_refresh_build':high,'inputs':rows}
    digest=hashlib.sha256(json.dumps(record,sort_keys=True,separators=(',',':')).encode()).hexdigest()
    record['build_id']='integrated-'+digest[:24]
    return record
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for arg in ('root','game','sdl','support','output'):p.add_argument(arg,type=pathlib.Path)
    p.add_argument('--high-refresh',type=int,choices=(0,1),required=True);a=p.parse_args()
    result=identify(a.root,a.game,a.sdl,a.support,a.high_refresh)
    a.output.write_text(json.dumps(result,indent=2)+'\n');print(result['build_id'])
