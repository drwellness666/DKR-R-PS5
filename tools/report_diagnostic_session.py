#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Combine saved local Klog, own journal and runtime evidence; no network access."""
import argparse
import bisect
import hashlib
import json
import os
import pathlib
import re
import stat
import subprocess

from analyze_session import analyze
from collect_session import MAX_LOG_BYTES, MAX_RECEIVE_RECORDS
from summarize_performance import summarize


def read_regular(path, limit):
    with os.fdopen(os.open(path, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK), 'rb') as source:
        info = os.fstat(source.fileno())
        if not stat.S_ISREG(info.st_mode) or info.st_size > limit:
            raise ValueError('Expected a bounded ordinary local file')
        data = source.read(limit + 1)
        if len(data) > limit:
            raise ValueError('Local file grew beyond the input limit')
        return data


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def validate_receive_index(data, metadata, log_size):
    if sha256(data) != metadata.get('receive_index_sha256'):
        raise ValueError('Receive index hash mismatch')
    rows = []
    offset = elapsed = 0
    for line in data.splitlines():
        row = json.loads(line)
        if (not isinstance(row, list) or len(row) != 3 or
                any(type(value) is not int for value in row)):
            raise ValueError('Invalid receive index row')
        at, size, when = row
        if at != offset or not 1 <= size <= 32768 or when < elapsed or when > 2**64-1:
            raise ValueError('Receive index has gaps or invalid time/size')
        offset += size
        elapsed = when
        if offset > log_size or len(rows) == MAX_RECEIVE_RECORDS:
            raise ValueError('Receive index exceeds bounds')
        rows.append(row)
    if type(metadata.get('receive_records')) is not int or len(rows) != metadata['receive_records']:
        raise ValueError('Receive index count mismatch')
    truncated = metadata.get('receive_index_truncated')
    if type(truncated) is not bool:
        raise ValueError('Receive index truncation flag missing')
    if truncated:
        if len(rows) != MAX_RECEIVE_RECORDS or offset >= log_size:
            raise ValueError('Invalid truncated receive index')
    elif offset != log_size:
        raise ValueError('Receive index does not cover the saved log')
    return rows


NUMBER = r'([0-9]{1,8}(?:\.[0-9]{1,6})?)'
FALLBACK = re.compile(r'wsi/videoout: '+NUMBER+r' Hz accepted, but a vblank comes every '+NUMBER+r' ms; '+NUMBER+r' Hz restored\b')
FRAMEBUFFERS = re.compile(r'wsi/videoout: ([0-9]{1,5})x([0-9]{1,5}) framebuffers registered as set ([0-9]{1,5}), buffers ([0-9]{1,5})-([0-9]{1,5})\b')
PRESENT = re.compile(r'\[perf\]\[startup\] elapsed='+NUMBER+r'ms event=first-successful-presentation\b')
SPLASH = re.compile(r'\[boot\]\[ps5\] hide-splash rc=(0x[0-9a-fA-F]{8})\b')


def observations(data, receive_rows=()):
    """Record what the source says, without assigning anonymous WSI lines to a title."""
    result = []
    ends = [at + size for at, size, when in receive_rows]
    offset = 0
    for number, raw in enumerate(data.splitlines(keepends=True), 1):
        offset += len(raw)
        if len(raw) > 16384:
            continue
        line = raw.decode('utf-8', errors='replace')
        entry = None
        if match := FALLBACK.search(line):
            entry = dict(kind='refresh-fallback', requested_hz=float(match[1]),
                         observed_vblank_ms=float(match[2]), restored_hz=float(match[3]))
        elif match := FRAMEBUFFERS.search(line):
            entry = dict(kind='registered-framebuffers', width=int(match[1]), height=int(match[2]),
                         buffer_set=int(match[3]), first_buffer=int(match[4]), last_buffer=int(match[5]))
        elif match := PRESENT.search(line):
            entry = dict(kind='first-present-return', startup_elapsed_ms=float(match[1]))
        elif match := SPLASH.search(line):
            entry = dict(kind='hide-launch-image', return_code=match[1].lower())
        if entry is not None and len(result) < 4096:
            index = bisect.bisect_left(ends, offset)
            entry.update(line=number, last_byte_offset=offset-1, candidate_attributed=False,
                         pc_receive_elapsed_ns=receive_rows[index][2] if index < len(ends) else None)
            result.append(entry)
    return result


def build_report(session, manifest, elf, journals=(), runtime_logs=(), shadowmount_logs=()):
    metadata = json.loads(read_regular(session/'SESSION.json', 65536))
    if not isinstance(metadata, dict) or type(metadata.get('schema')) is not int or metadata['schema'] not in (1, 2):
        raise ValueError('Unsupported capture schema')
    data = read_regular(session/'kernel.log', MAX_LOG_BYTES)
    if (type(metadata.get('bytes')) is not int or len(data) != metadata['bytes'] or
            sha256(data) != metadata.get('sha256')):
        raise ValueError('Saved kernel log differs from its capture receipt')
    rows = []
    if metadata['schema'] == 2:
        rows = validate_receive_index(read_regular(session/'RECEIVE.jsonl', 8*1024*1024), metadata, len(data))
    sources = []

    def add_source(kind, payload, timings=()):
        text = payload.decode('utf-8', errors='replace')
        sources.append({'kind': kind, 'bytes': len(payload), 'sha256': sha256(payload),
                        'diagnostics': analyze(text, manifest, elf),
                        'runtime_observations': observations(payload, timings),
                        'performance': summarize(text)})

    add_source('kernel', data, rows)
    for kind, paths in (('own-journal', journals), ('runtime', runtime_logs), ('shadowmount', shadowmount_logs)):
        for path in paths:
            add_source(kind, read_regular(path, MAX_LOG_BYTES))
    return {'schema': 1, 'candidate_build_id': manifest['build_id'],
            'capture_build_label': metadata.get('build_id'),
            'capture_label_matches_candidate': metadata.get('build_id') == manifest['build_id'],
            'capture_label_is_runtime_proof': False,
            'end_reason': metadata.get('end_reason'),
            'crash_inferred_from_disconnect': False,
            'receive_index': {'available': metadata['schema'] == 2, 'records': len(rows),
                              'truncated': metadata.get('receive_index_truncated', False)},
            'sources': sources,
            'limitations': [
                'A capture build label is supplied by the operator; exact own records control symbol attribution.',
                'Only matching crash build IDs, verified private ELF hashes, entry/load bases and executable ranges are symbolized.',
                'PC receive times describe network arrival, not PS5 execution, frame time or HDMI scanout.',
                'Anonymous driver/loader observations remain unattributed, even when captured beside a matching journal.',
                'Separate source clocks are not synchronized or merged; receive timing is absent for old captures and added local logs.',
                'No crash is inferred from missing journal data or a closed socket; no stutter cause is inferred from nearby log entries.',
                'This report reads local files only and does not launch, stop, attach to or patch a console process.']}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('session', type=pathlib.Path)
    parser.add_argument('manifest', type=pathlib.Path)
    parser.add_argument('symbol_elf', type=pathlib.Path)
    parser.add_argument('--journal', type=pathlib.Path, action='append', default=[])
    parser.add_argument('--runtime-log', type=pathlib.Path, action='append', default=[])
    parser.add_argument('--shadowmount-log', type=pathlib.Path, action='append', default=[])
    args = parser.parse_args()
    try:
        manifest = json.loads(read_regular(args.manifest, 1024*1024))
        result = build_report(args.session, manifest, args.symbol_elf,
                              args.journal, args.runtime_log, args.shadowmount_log)
    except (ValueError, KeyError, OSError, subprocess.CalledProcessError) as error:
        parser.error(str(error))
    print(json.dumps(result, indent=2, allow_nan=False))


if __name__ == '__main__':
    main()
