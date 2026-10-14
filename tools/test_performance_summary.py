#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Synthetic log acceptance tests; no ROM, hardware or timing claim."""
import json
from summarize_performance import summarize

sample = '''private unrelated path /home/person/secret and 192.168.1.2
[perf][startup] elapsed=1.0ms event=persistent-log-ready
[perf][total] vulkan-graphics-pipeline calls=2 wall-ms=20.000 bytes=0
[perf][total] vulkan-graphics-pipeline calls=5 wall-ms=42.000 bytes=0
[perf][present-interval] n=300 ms(p50/p95/p99/max)=16.000/18.000/33.000/80.000
[perf][present-interval] n=300 ms(p50/p95/p99/max)=16.000/17.000/19.000/21.000
[graphics][performance] presented=59.7 sim=30.0 graphics=30.0 vi=60.0 target=60 scale=2.0
[perf][track-history] render-gpu n=120 ms(p50/p95/p99)=2.000/4.000/8.000
[perf][startup] elapsed=0.5ms event=persistent-log-ready
[perf][total] UI-build calls=1 wall-ms=1.000 bytes=0
'''
result = summarize(sample)
first, second = result['sessions']
assert first['latest_region_totals']['vulkan-graphics-pipeline']['calls'] == 5
assert first['latest_region_totals']['vulkan-graphics-pipeline']['wall_ms'] == 42
assert first['reported_present_intervals'] == 600
assert first['largest_reported_present_gap_ms'] == 80
assert len(first['present_windows']) == 2 and first['present_windows'][1]['p99_ms'] == 19
assert first['rates'][0]['simulation_hz'] == 30
assert first['track_history'][0]['region'] == 'render-gpu'
assert second['largest_reported_present_gap_ms'] is None
encoded = json.dumps(result)
assert '/home/person' not in encoded and '192.168' not in encoded
assert summarize('unrelated startup only')['sessions'] == []
malformed = '''[perf][present-interval] n=300 ms(p50/p95/p99/max)=50/20/30/40
[perf][present-interval] n=0 ms(p50/p95/p99/max)=1/2/3/4
[perf][present-interval] n=300 ms(p50/p95/p99/max)=nan/inf/3/4
[perf][total] unknown-private-identifier calls=1 wall-ms=1 bytes=0
'''
assert summarize(malformed)['sessions'] == []
reset = summarize('[perf][total] UI-build calls=10 wall-ms=2 bytes=0\n[perf][total] UI-build calls=1 wall-ms=1 bytes=0')['sessions'][0]
assert reset['counter_resets'] == ['UI-build']
assert reset['latest_region_totals']['UI-build']['calls'] == 1
assert summarize('x' * 20000 + sample.replace('\n', ''))['sessions'] == []
json.dumps(result, allow_nan=False)
print('PASS: real upstream log formats, separate windows/sessions, cumulative totals, resets, malformed input and private-line exclusion')
