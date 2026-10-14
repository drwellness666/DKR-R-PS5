#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise actual upstream profiler headers on the host, without ROM or GPU."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
from summarize_performance import summarize

if len(sys.argv) != 2 or not Path(sys.argv[1]).is_absolute():
    raise SystemExit('Usage: test_upstream_performance.py ABSOLUTE_PREPARED_GAME_WORK')
include = Path(sys.argv[1]) / 'source/DKR-R/runtime-recomp/src/game'
if not (include / 'performance_trace.hpp').is_file():
    raise SystemExit('Prepare the public pinned game source first')
code = r'''
#include "performance_trace.hpp"
#include <cassert>
#include <thread>
#include <vector>
int main(int argc, char**) {
    namespace p = dkr::runtime::performance_trace;
    const bool expected = argc == 2;
    assert(p::enabled() == expected);
    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i) workers.emplace_back([] {
        for (int j = 0; j < 100; ++j) { p::Scope scope(p::Region::SnapshotCopy, 16); }
    });
    for (auto& worker : workers) worker.join();
    const auto counters = p::snapshot();
    assert(counters[0][0] == (expected ? 400U : 0U));
    assert(counters[0][2] == (expected ? 6400U : 0U));
    for (int i = 0; i < 301; ++i) p::record_present();
    p::report();
}
'''
with tempfile.TemporaryDirectory(prefix='dkrr-upstream-profiler-') as directory:
    work = Path(directory)
    (work / 'probe.cpp').write_text(code)
    subprocess.run(['clang++', '-std=c++20', '-Wall', '-Wextra', '-Werror', '-pthread',
                    '-I', str(include), str(work / 'probe.cpp'), '-o', str(work / 'probe')], check=True)
    env = os.environ.copy()
    env['DKR_POWER_PROFILE'] = '1'
    run = subprocess.run([str(work / 'probe'), 'enabled'], env=env, capture_output=True, text=True, check=True)
    measured = summarize(run.stderr)['sessions'][0]
    assert measured['latest_region_totals']['snapshot-copy']['calls'] == 400
    assert measured['latest_region_totals']['snapshot-copy']['bytes'] == 6400
    assert measured['reported_present_intervals'] == 300
    env.pop('DKR_POWER_PROFILE')
    run = subprocess.run([str(work / 'probe')], env=env, capture_output=True, text=True, check=True)
    assert summarize(run.stderr)['sessions'] == []
    env['DKR_POWER_PROFILE'] = '0'
    run = subprocess.run([str(work / 'probe')], env=env, capture_output=True, text=True, check=True)
    assert summarize(run.stderr)['sessions'] == []
print('PASS: actual upstream threaded counters, presentation report format, parser integration, disabled path; host timings are not PS5 performance')
