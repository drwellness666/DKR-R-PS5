#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Summarize existing DKR-R performance reports locally; no console connection."""
import argparse
import json
import math
import os
from pathlib import Path
import re
import stat

NUMBER = r'(?:[0-9]+(?:\.[0-9]+)?)'
PRESENT = re.compile(r'\[perf\]\[present-interval\] n=(\d+) ms\(p50/p95/p99/max\)=(' + NUMBER + r')/(' + NUMBER + r')/(' + NUMBER + r')/(' + NUMBER + r')(?![\w.])')
TOTAL = re.compile(r'\[perf\]\[total\] ([a-zA-Z0-9-]+) calls=(\d+) wall-ms=(' + NUMBER + r') bytes=(\d+)(?!\w)')
RATES = re.compile(r'\[graphics\]\[performance\] presented=(' + NUMBER + r') sim=(' + NUMBER + r') graphics=(' + NUMBER + r') vi=(' + NUMBER + r') target=(\d+) scale=(' + NUMBER + r')(?![\w.])')
HISTORY = re.compile(r'\[perf\]\[track-history\] (matching|render-cpu|render-gpu|workload) n=(\d+) ms\(p50/p95/p99\)=(' + NUMBER + r')/(' + NUMBER + r')/(' + NUMBER + r')(?![\w.])')
REGIONS = frozenset(('snapshot-copy', 'full-sync', 'gpu-fence', 'readback', 'friend-snapshot',
    'vulkan-compute-pipeline', 'vulkan-graphics-pipeline', 'UI-build', 'buffer-upload',
    'buffer-upload-wait', 'fence-framebuffer', 'fence-workload', 'fence-present',
    'fence-texture', 'fence-other', 'idle-gpu-submit', 'idle-gpu-skipped',
    'online-state-capture', 'online-state-apply', 'online-state-encode',
    'online-state-decode', 'online-pacing-view'))

def summarize(text):
    """Keep each reported window intact; percentiles cannot be averaged or merged."""
    sessions = []
    def fresh():
        return {'present_windows': [], 'latest_region_totals': {}, 'rates': [],
                'track_history': [], 'counter_resets': []}
    current = fresh()
    for line in text.splitlines():
        if len(line) > 16384:
            continue
        if '[perf][startup]' in line and 'event=persistent-log-ready' in line:
            if any(current.values()):
                sessions.append(current)
            current = fresh()
        if match := PRESENT.search(line):
            n, *values = match.groups()
            values = list(map(float, values))
            if int(n) > 0 and all(math.isfinite(v) for v in values) and values == sorted(values):
                current['present_windows'].append(dict(zip(('n', 'p50_ms', 'p95_ms', 'p99_ms', 'max_ms'), [int(n), *values])))
        if match := TOTAL.search(line):
            name, calls, wall, size = match.groups()
            if name not in REGIONS or not math.isfinite(float(wall)):
                continue
            entry = {'calls': int(calls), 'wall_ms': float(wall), 'bytes': int(size)}
            previous = current['latest_region_totals'].get(name)
            if previous and any(entry[key] < previous[key] for key in entry):
                current['counter_resets'].append(name)
            current['latest_region_totals'][name] = entry
        if match := RATES.search(line):
            values = list(map(float, match.groups()))
            if all(math.isfinite(v) for v in values):
                current['rates'].append(dict(zip(('presented_fps', 'simulation_hz', 'graphics_hz', 'vi_hz', 'target_hz', 'scale'), values)))
        if match := HISTORY.search(line):
            name, n, *values = match.groups()
            values = list(map(float, values))
            if int(n) > 0 and all(math.isfinite(v) for v in values) and values == sorted(values):
                current['track_history'].append({'region': name, 'n': int(n), 'p50_ms': values[0], 'p95_ms': values[1], 'p99_ms': values[2]})
    if any(current.values()):
        sessions.append(current)
    for session in sessions:
        windows = session['present_windows']
        session['reported_present_intervals'] = sum(window['n'] for window in windows)
        session['largest_reported_present_gap_ms'] = max((window['max_ms'] for window in windows), default=None)
    return {'schema': 1, 'sessions': sessions, 'limitations': [
        'Presentation intervals measure successful swapchain returns, not display scanout.',
        'Each reported percentile window is retained; percentiles are not pooled or averaged.',
        'Region totals are the latest cumulative observations, not sums of repeated reports.',
        'Region WALL times overlap across scopes/threads; they are not additive CPU or GPU utilization.',
        'Track histories roll across scene boundaries; no cold/warm or texture-pack state is inferred.',
        'No timing reports means unmeasured, not zero; this report does not establish a stutter cause.',
        'Only selected numerical performance fields are retained; raw log lines are excluded.']}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('log', type=Path)
    args = parser.parse_args()
    fd = os.open(args.log, os.O_RDONLY | os.O_NOFOLLOW)
    with os.fdopen(fd, 'rb') as source:
        info = os.fstat(source.fileno())
        if not stat.S_ISREG(info.st_mode) or info.st_size > 64 * 1024 * 1024:
            parser.error('Expected an ordinary local log of at most 64 MiB')
        data = source.read(64 * 1024 * 1024 + 1)
        if len(data) > 64 * 1024 * 1024:
            parser.error('Log grew beyond 64 MiB')
    print(json.dumps(summarize(data.decode('utf-8', errors='replace')), indent=2, allow_nan=False))

if __name__ == '__main__':
    main()
