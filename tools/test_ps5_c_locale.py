#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise the actual game JSON/water parser with native-title locale failure."""
import argparse,json,os,subprocess,tempfile,zipfile
from pathlib import Path
r=Path(__file__).resolve().parents[1];p=argparse.ArgumentParser();p.add_argument('game',type=Path);p.add_argument('--archive',type=Path);a=p.parse_args();g=a.game.resolve();d=g/'game-ps5/native-domain'
# Public Sherbet Land 1.0.0 profile values, no game/model/texture data.
metadata={'version':1,'materials':[],'profile':{'absorption':0.006000000052154064,'roughness':0.15000000596046448,'visibility':650.0,'ripple':0.6499999761581421,'foam':0.6000000238418579,'deep':[0.022216796875,0.033203125,0.042236328125],'shallow':[0.034423828125,0.0576171875,0.048095703125],'sky':[0.076171875,0.1142578125,0.080078125],'sun':[0.3499999940395355,0.8500000238418579,0.25]}}
if a.archive:
 with zipfile.ZipFile(a.archive) as z:metadata=json.loads(z.read('Sherbet Land.dkrmap/manifest.json'))['enhancedWater']
with tempfile.TemporaryDirectory(prefix='dkrr-c-locale-') as temp:
 t=Path(temp);(t/'profile.json').write_text(json.dumps(metadata));source=r'''#define JSON_ASSERT(x) ((void)0)
#include "enhanced_water_metadata.hpp"
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <thread>
#include <vector>
extern "C" struct lconv* dkrr_ps5_localeconv(void);
static bool fixed=false;
extern "C" struct lconv* __wrap_localeconv(void){if(fixed)return dkrr_ps5_localeconv();static char empty[]="";static struct lconv broken{};broken.decimal_point=empty;broken.thousands_sep=empty;broken.grouping=empty;return &broken;}
static void require(bool condition){if(!condition)std::abort();}
int main(int,char**argv){std::ifstream file(argv[1]);std::string text((std::istreambuf_iterator<char>(file)),{});using namespace dkr::runtime::water::enhanced;Metadata out;std::string error;
 auto damaged=nlohmann::json::parse(text);require(damaged["profile"]["absorption"].get<double>()==0);require(!parse_metadata(damaged,out,error));require(error=="water profile values exceed their safe ranges");
 fixed=true;auto good=nlohmann::json::parse(text);require(parse_metadata(good,out,error));require(out.profile&&out.profile->absorption>.0059F&&out.profile->roughness>.149F&&out.profile->sun.x>.349F);
 require(good==nlohmann::json::parse(good.dump()));
 auto invalid=good;invalid["profile"]["absorption"]=-1;require(!parse_metadata(invalid,out,error));invalid=good;invalid["profile"]["sun"]={0,0,0};require(!parse_metadata(invalid,out,error));
 std::vector<std::thread> threads;for(unsigned n=0;n<8;++n)threads.emplace_back([&]{for(unsigned i=0;i<1000;++i){auto j=nlohmann::json::parse(text);Metadata m;std::string e;require(parse_metadata(j,m,e));require(j==good);}});for(auto&t:threads)t.join();
 puts("PASS actual water/JSON parser: empty native decimal reproduces Sherbet rejection; SDK C-locale backport preserves all fractions, round trips, safety rejection and 8000 concurrent parses");}
''';(t/'test.cpp').write_text(source)
 flags=['-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined']
 subprocess.run(['clang','-std=c11',*flags,'-c',str(r/'src/ps5_c_locale.c'),'-o',str(t/'locale.o')],check=True)
 subprocess.run(['clang++','-std=c++20',*flags,'-pthread','-I'+str(d/'src/game'),'-I'+str(g/'source/DKR-R/extern/rt64/src/contrib'),str(t/'test.cpp'),str(t/'locale.o'),'-Wl,--wrap=localeconv','-o',str(t/'test')],check=True)
 subprocess.run([str(t/'test'),str(t/'profile.json')],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
