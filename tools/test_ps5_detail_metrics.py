#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,os,subprocess,tempfile
from pathlib import Path
from stage_ps5_detail_metrics import compose
p=argparse.ArgumentParser();p.add_argument('game',type=Path);a=p.parse_args();r=Path(__file__).resolve().parents[1]
source=a.game.resolve()/'game-ps5/generated/detail-records-repair/detail_metrics.hpp'
with tempfile.TemporaryDirectory(prefix='dkrr-detail-log-') as temporary:
 t=Path(temporary);(t/'metrics.hpp').write_text(compose(source.read_text()))
 (t/'test.cpp').write_text('''#include "metrics.hpp"
#include <cassert>
int main(){dkr::runtime::detail::FrameMetrics metrics;
 unsigned poses=3,uploads=5,batches=7,hits=11,captures=13;
 metrics.scene=7;metrics.frames=1;metrics.reported=metrics.Clock::now()-std::chrono::seconds(3);
 metrics.begin(7,2,poses,uploads,batches,hits,captures);
 assert(metrics.frames==1&&metrics.observations==2&&!poses&&!uploads&&!batches&&!hits&&!captures);
}
'''.replace('metrics.Clock::now()','dkr::runtime::detail::FrameMetrics::Clock::now()'))
 subprocess.run(['clang++','-std=c++20','-O1','-g','-Wall','-Wextra','-Werror','-I'+str(r/'src'),'-I'+str(t),str(t/'test.cpp'),'-o',str(t/'test')],check=True)
 for setting in (None,'0','false','1','10'):
  env=dict(os.environ);env.pop('DKR_TRACK_PROFILE',None)
  if setting is not None:env['DKR_TRACK_PROFILE']=setting
  run=subprocess.run([str(t/'test')],env=env,check=True,capture_output=True,text=True)
  assert bool(run.stderr)==(setting=='1'),(setting,run.stderr)
  if setting=='1':assert '[graphics][detail-cost]' in run.stderr
 print('PASS actual FrameMetrics: zero render-thread log writes by default, explicit opt-in only, identical frame/counter reset semantics')
