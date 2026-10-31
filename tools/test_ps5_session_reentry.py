#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise real overlay initialization and the staged registry function."""
import argparse, os, shlex, subprocess, tempfile
from pathlib import Path
from stage_ps5_session import compose
p=argparse.ArgumentParser(description=__doc__);p.add_argument('game',type=Path);a=p.parse_args()
g=a.game.resolve()
with tempfile.TemporaryDirectory(prefix='dkrr-session-reentry-') as temporary:
    t=Path(temporary)
    source=g/'source/N64ModernRuntime/librecomp/src/overlays.cpp'
    (t/'overlays.cpp').write_text(compose('overlays.cpp',source.read_text()))
    (t/'test.cpp').write_text('''#include "overlays.cpp"
int main() {
    SectionTableEntry a[2]{}; a[0].rom_addr=0x100; a[0].ram_addr=0x80000400; a[0].index=0;
    a[1].rom_addr=0x900; a[1].ram_addr=0x80000800; a[1].index=3;
    SectionTableEntry b[1]{}; b[0].rom_addr=0x500; b[0].ram_addr=0x80001000; b[0].index=1;
    for(int i=0;i<2000;++i) {
        const bool odd=i&1;
        recomp::overlays::register_overlays({odd?b:a,odd?1U:2U,odd?2U:4U},{nullptr,0});
        recomp::overlays::init_overlays();
        assert(loaded_sections.empty()&&func_map.empty());
        assert(code_sections_by_rom.size()==(odd?1U:2U));
        assert(code_sections_by_rom.contains(odd?0x500U:0x100U));
        assert(!code_sections_by_rom.contains(odd?0x100U:0x500U));
        assert(section_addresses[odd?1:3]==(int32_t)(odd?0x80001000U:0x80000800U));
        loaded_sections.emplace_back(0x80000400,0); func_map[0x80000400]=nullptr;
    }
    free(section_addresses);section_addresses=nullptr;
    puts("PASS real overlay initializer: 2000 alternating layouts, old indices/functions/ranges cleared, ASan/UBSan and no leaked address tables");
}
''')
    flags_file=g/'game-ps5/n64-modern-runtime/librecomp/CMakeFiles/librecomp.dir/flags.make'
    includes=shlex.split(next(line.split('=',1)[1] for line in flags_file.read_text().splitlines() if line.startswith('CXX_INCLUDES =')))
    flags=['-std=c++20','-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-ffunction-sections','-fdata-sections']
    subprocess.run(['clang++',*flags,*includes,'-I'+str(source.parent),str(t/'test.cpp'),'-Wl,--gc-sections','-o',str(t/'test')],check=True)
    env=dict(os.environ,ASAN_OPTIONS='detect_leaks=1',UBSAN_OPTIONS='halt_on_error=1')
    subprocess.run([str(t/'test')],env=env,check=True,timeout=20)
    # Compile the actual staged registration body with just its ownership
    # boundary stubbed; verify replacement plus rejection before any mutation.
    raw=(g/'game-ps5/native-session/recomp.cpp').read_text()
    lifetime=raw[:raw.index('#include <cstdio>')]
    if raw.count('NativeWorkerScope native_worker_scope;')!=1:
        raise RuntimeError('The actual runtime entry does not own its worker lifetime')
    (t/'lifetime.cpp').write_text(lifetime+'''\n#include <cassert>
#include <cstdio>
enum class Gate {None, Running, Quit};
int main(){
    Gate guest_gate=Gate::None;
    for(int i=0;i<2000;++i){
        assert(!dkrr_native_runtime_active());
        { NativeWorkerScope session; assert(dkrr_native_runtime_active());
          guest_gate=Gate::Quit; // A stop request is not proof of joined owners.
          assert(dkrr_native_runtime_active());
          bool rejected=false;try{NativeWorkerScope duplicate;}catch(const std::runtime_error&){rejected=true;}
          assert(rejected&&dkrr_native_runtime_active());
        }
        assert(guest_gate==Gate::Quit&&!dkrr_native_runtime_active());
        try { NativeWorkerScope early_failure; throw std::runtime_error("before workers start"); }
        catch(const std::runtime_error&){}
        assert(!dkrr_native_runtime_active());
    }
    puts("PASS actual native worker lifetime: 2000 joined sessions, stale Quit accepted, concurrent entry rejected and early exceptions released");
}
''')
    subprocess.run(['clang++',*flags,str(t/'lifetime.cpp'),'-pthread','-o',str(t/'lifetime')],check=True)
    subprocess.run([str(t/'lifetime')],env=env,check=True,timeout=20)
    start=raw.index('bool recomp::register_game(');at=raw.index('{',start);depth=1;end=at+1
    while depth:
        depth+=(raw[end]=='{')-(raw[end]=='}');end+=1
    body=raw[start:end]
    (t/'registry.cpp').write_text('''#include <cassert>
#include <cstdio>
#include <map>
#include <memory>
#include <mutex>
#include <string>
namespace ultramodern { bool active=false; }
extern "C" int dkrr_native_runtime_active(){return ultramodern::active ? 1 : 0;}
namespace recomp { struct GameEntry { std::u8string game_id; unsigned rom_hash; std::string mod_game_id; }; bool register_game(const GameEntry&); }
std::mutex game_roms_mutex,mod_context_mutex;
std::map<std::u8string,recomp::GameEntry> game_roms;
struct Mods { void register_game(const std::string&){} }; auto mod_context=std::make_unique<Mods>();
'''+body+'''
int main(){
    for(unsigned i=0;i<2000;++i) {
        unsigned hash=(i&1)?80:77; assert(recomp::register_game({u8"dkr.us.v77",hash,{}}));
        assert(game_roms.size()==1&&game_roms.begin()->second.rom_hash==hash);
        ultramodern::active=true;assert(!recomp::register_game({u8"dkr.us.v77",999,{}}));
        assert(game_roms.begin()->second.rom_hash==hash);ultramodern::active=false;
    }
    puts("PASS actual native game registry body: 2000 replacements after stop; active-session mutation refused");
}
''')
    subprocess.run(['clang++',*flags,str(t/'registry.cpp'),'-pthread','-o',str(t/'registry')],check=True)
    subprocess.run([str(t/'registry')],env=env,check=True,timeout=20)
