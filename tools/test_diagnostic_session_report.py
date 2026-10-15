#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Real loopback transport, preserved bytes, receipt checks and matched symbols."""
import hashlib
import contextlib
import json
import pathlib
import re
import resource
import signal
import socket
import stat
import subprocess
import tempfile
import threading
from unittest.mock import patch

import collect_session
import report_diagnostic_session
from analyze_session import analyze, elf_layout
from report_diagnostic_session import build_report, observations, validate_receive_index


with tempfile.TemporaryDirectory(prefix='dkrr-combined-diagnostics-') as temporary:
    work = pathlib.Path(temporary)
    source = work/'program.c'
    source.write_text('int marker(void){return 42;} int main(void){return marker();}\n')
    elf = work/'symbols.elf'
    subprocess.run(['clang', '-O0', '-g', '-fPIE', '-pie', str(source), '-o', str(elf)], check=True)
    entry, _ = elf_layout(elf.read_bytes())
    symbols = subprocess.check_output(['nm', '-n', str(elf)], text=True)
    rip = int(re.search(r'^([0-9a-f]+) T marker$', symbols, re.M)[1], 16)
    base = 0x700000000000
    manifest = {'build_id': 'integrated-test', 'symbol_elf_sha256': hashlib.sha256(elf.read_bytes()).hexdigest()}
    crash = 'DKRR_CRASH v=1 build=integrated-test'+''.join(
        f' {key}=0x{value:016x}' for key, value in {
            'signal': 11, 'code': 1, 'address': 0xd0, 'rip': base+rip, 'rsp': base+0x30000,
            'trap': 14, 'error': 4, 'entry': base+entry, 'stage': 4}.items())+'\n'
    journal = work/'journal.log'
    journal.write_text(f'DKRR_SESSION v=1 build=integrated-test pid=123 entry=0x{base+entry:016x}\n'+crash)
    # The capture's operator label deliberately differs from the game's own record.
    raw = (b'<118>unrelated background service\npartial UTF-8: \xe2\x82\xac\r\n'+
           b'wsi/videoout: 119.88 Hz accepted, but a vblank comes every 20.872 ms; 59.94 Hz restored\n'+
           b'[DKR-R game] [perf][startup] elapsed=8936.201ms event=first-successful-presentation\n'+
           b'wsi/videoout: 3840x2160 framebuffers registered as set 0, buffers 0-4\n'+
           b'[DKR-R game] [boot][ps5] hide-splash rc=0x00000000 after first presentation\n'+
           b'last partial line')

    def capture_bytes(name, payload, **overrides):
        listener = socket.socket()
        listener.bind(('127.0.0.1', 0))
        listener.listen(1)
        port = listener.getsockname()[1]
        def send():
            with listener:
                client, _ = listener.accept()
                with client:
                    try:
                        client.sendall(payload[:49])
                        client.sendall(payload[49:])
                    except (BrokenPipeError, ConnectionResetError):
                        pass
        sender = threading.Thread(target=send)
        sender.start()
        output = work/'private'/name
        with patch.multiple(collect_session, **overrides) if overrides else contextlib.nullcontext():
            record = collect_session.capture('127.0.0.1', port, output, 2, 'other-build')
        sender.join(5)
        assert not sender.is_alive()
        return output, record

    session, receipt = capture_bytes('normal', raw)
    assert (session/'kernel.log').read_bytes() == raw
    assert receipt['end_reason'] == 'connection-closed' and not receipt['crash_inferred_from_disconnect']
    assert stat.S_IMODE(session.stat().st_mode) == 0o700
    for name in ('kernel.log', 'RECEIVE.jsonl', 'SESSION.json'):
        assert stat.S_IMODE((session/name).stat().st_mode) == 0o600
    index = (session/'RECEIVE.jsonl').read_bytes()
    rows = validate_receive_index(index, receipt, len(raw))
    assert sum(row[1] for row in rows) == len(raw)
    result = build_report(session, manifest, elf, [journal])
    assert not result['capture_label_matches_candidate'] and not result['capture_label_is_runtime_proof']
    assert result['sources'][1]['diagnostics']['crashes'][0]['attributed']
    assert 'marker' in result['sources'][1]['diagnostics']['crashes'][0]['symbolization']
    assert not result['sources'][1]['diagnostics']['journal_sessions'][0]['clean_shutdown_marker']
    events = result['sources'][0]['runtime_observations']
    assert [event['kind'] for event in events] == ['refresh-fallback', 'first-present-return', 'registered-framebuffers', 'hide-launch-image']
    assert events[0]['observed_vblank_ms'] == 20.872 and events[0]['restored_hz'] == 59.94
    assert all(event['pc_receive_elapsed_ns'] is not None and not event['candidate_attributed'] for event in events)
    assert not result['crash_inferred_from_disconnect']
    json.dumps(result, allow_nan=False)

    def refuses(action):
        try:
            action()
        except ValueError:
            return
        raise AssertionError('Invalid evidence accepted')

    refuses(lambda: build_report(session, dict(manifest, symbol_elf_sha256='0'*64), elf))
    (session/'kernel.log').write_bytes(raw+b'changed')
    refuses(lambda: build_report(session, manifest, elf))
    (session/'kernel.log').write_bytes(raw)
    refuses(lambda: validate_receive_index(index+b'\n', receipt, len(raw)))
    for values in ([[0, len(raw), 1], [len(raw)+1, 1, 2]], [[0, len(raw), -1]], [[True, 1, 2]], [[0, len(raw)-1, 1]]):
        data = b''.join((json.dumps(row)+'\n').encode() for row in values)
        invalid = dict(receipt, receive_index_sha256=hashlib.sha256(data).hexdigest(), receive_records=len(values))
        refuses(lambda: validate_receive_index(data, invalid, len(raw)))

    # Legacy raw captures remain readable; no receipt timestamps are invented.
    legacy = dict(receipt, schema=1)
    (session/'SESSION.json').write_text(json.dumps(legacy))
    result = build_report(session, manifest, elf)
    assert not result['receive_index']['available']
    assert all(event['pc_receive_elapsed_ns'] is None for event in result['sources'][0]['runtime_observations'])
    # Same format accepted from a timestamping external Klog viewer.
    assert observations(b'[2026-10-15T10:20:30.123+02:00] '+raw)[0]['restored_hz'] == 59.94

    limited, receipt = capture_bytes('bounded', b'x'*4096, MAX_LOG_BYTES=100)
    assert receipt['end_reason'] == 'size-limit' and receipt['bytes'] == 100
    assert (limited/'kernel.log').read_bytes() == b'x'*100
    validate_receive_index((limited/'RECEIVE.jsonl').read_bytes(), receipt, 100)

    # Empty stream and capped index are valid transport states, never crashes.
    empty, receipt = capture_bytes('empty', b'')
    assert receipt['bytes'] == 0 and build_report(empty, manifest, elf)['sources'][0]['runtime_observations'] == []
    with patch.object(collect_session, 'MAX_RECEIVE_RECORDS', 0):
        truncated, receipt = capture_bytes('index-capped', raw)
    assert receipt['receive_index_truncated'] and receipt['receive_records'] == 0
    assert (truncated/'kernel.log').read_bytes() == raw
    with patch.object(report_diagnostic_session, 'MAX_RECEIVE_RECORDS', 0):
        capped = build_report(truncated, manifest, elf)
    assert capped['receive_index']['truncated']
    assert all(event['pc_receive_elapsed_ns'] is None for event in capped['sources'][0]['runtime_observations'])

    for malformed in (crash.replace(' rip=', ' unknown='), crash.rstrip()+' rip=0x0000000000000000\n', crash.replace(' code=', ' code=bad extra=')):
        parsed = analyze(malformed, manifest, elf)
        assert not any(row['attributed'] for row in parsed['crashes'])
    captured = 'DKRR_CAPTURE v=1 build=integrated-test intervals=60 elapsed-ns=1000000000 max-gap-ns=60000000 over33=1 over50=1 over100=0 dropped=0'
    assert analyze(captured, manifest, elf)['captures'][0]['present_returns_per_second'] == 60
    assert not analyze(captured+' intervals=99', manifest, elf)['captures']
    assert not analyze(captured.replace('over100=0', 'over100=7'), manifest, elf)['captures']
    old = journal.read_text().replace('integrated-test', 'older-build')+'DKRR_SESSION_END\n'
    parsed = analyze(old, manifest, elf)
    assert not parsed['crashes'][0]['attributed'] and not parsed['journal_sessions'][0]['matches_candidate']
    assert parsed['journal_sessions'][0]['clean_shutdown_marker']

    # Exercise the complete path using our actual signal handler and an actual
    # PIE fault in a contained Linux child, then resolve that recorded RIP.
    root = pathlib.Path(__file__).resolve().parent.parent
    fault_data = work/'fault-data'
    fault_data.mkdir()
    fault_source = work/'real_fault.c'
    fault_source.write_text('''#include <stdint.h>
#include "ps5_diagnostics.h"
extern void _start(void);
__attribute__((noinline)) void actual_fault(void) { *(volatile int *)(uintptr_t)0xd0=1; }
int main(void) {
    if(dkrr_diag_initialize("integrated-host-fault",(uintptr_t)&_start)!=0)return 2;
    dkrr_diag_stage(DKR_DIAG_PRESENT);
    actual_fault();
    return 3;
}
''')
    fault_elf = work/'real_fault.elf'
    subprocess.run(['clang', '-std=c11', '-O0', '-g', '-fPIE', '-pie', '-I', str(root/'src'),
                    '-DDKR_DIAG_DATA_ROOT="'+str(fault_data)+'"', str(fault_source),
                    str(root/'src/ps5_diagnostics.c'), '-o', str(fault_elf)], check=True)
    child = subprocess.run([str(fault_elf)], preexec_fn=lambda: resource.setrlimit(resource.RLIMIT_CORE, (0, 0)))
    assert child.returncode == -signal.SIGSEGV
    real_journals = list((fault_data/'diagnostics').glob('run-*.log'))
    assert len(real_journals) == 1
    fault_manifest = {'build_id': 'integrated-host-fault', 'symbol_elf_sha256': hashlib.sha256(fault_elf.read_bytes()).hexdigest()}
    end_to_end = build_report(empty, fault_manifest, fault_elf, real_journals)
    crash_result = end_to_end['sources'][1]['diagnostics']['crashes'][0]
    assert crash_result['attributed'] and crash_result['raw']['address'] == '0x00000000000000d0'
    assert 'actual_fault' in crash_result['symbolization'] and 'real_fault.c:' in crash_result['symbolization']

print('PASS real TCP/raw bytes/receipts/bounds, private permissions, legacy/external logs, tamper and wrong-build refusal; actual signal handler -> real child SIGSEGV -> saved journal -> exact PIE source line')
