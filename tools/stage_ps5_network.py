#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Stage the two pinned RTC wakeup consumers without editing dependencies."""
import argparse, hashlib, json
from pathlib import Path


def stage(source, output):
    records = []
    for relative, expected, variable in (
        ('src/impl/pollinterrupter.cpp', '2034f1ef0368557c29ad48a10e3cba42d7c53a86bf499f82cf7a6f129a8ab460', 'pipefd'),
        ('deps/libjuice/src/conn_poll.c', '0070d5366b2197a05cf5e7c2f25734c9fc24b5d4c38abb7e8bb50c35255dfce1', 'pipefds'),
    ):
        old = source / relative
        raw = old.read_bytes()
        if hashlib.sha256(raw).hexdigest() != expected:
            raise ValueError('Unexpected RTC wakeup source: ' + relative)
        text = raw.decode()
        create = ('::pipe(pipefd) != 0' if variable == 'pipefd' else 'pipe(pipefds)')
        replacement = 'dkrr_net_wakeup_pair(' + variable + ') != 0'
        setters = ('\t::fcntl(pipefd[0], F_SETFL, O_NONBLOCK);\n\t::fcntl(pipefd[1], F_SETFL, O_NONBLOCK);\n' if variable == 'pipefd' else
                   '\tfcntl(pipefds[0], F_SETFL, O_NONBLOCK);\n\tfcntl(pipefds[1], F_SETFL, O_NONBLOCK);\n')
        if text.count(create) != 1 or text.count(setters) != 1:
            raise ValueError('RTC wakeup shape changed: ' + relative)
        text = text.replace(create, replacement).replace(setters, '')
        # Keep the original MPL notice at the beginning of each derived file.
        include = '#include "pollinterrupter.hpp"' if variable == 'pipefd' else '#include "conn_poll.h"'
        text = text.replace(include, '#include "ps5_net.h"\n' + include, 1)
        pairs = [('::read(pfd.fd, &dummy, 1)', 'dkrr_net_recv(pfd.fd, &dummy, 1, 0)'),
                 ('::write(mPipeOut, &dummy, 1)', 'dkrr_net_send(mPipeOut, &dummy, 1, 0)')] if variable == 'pipefd' else [
                 ('read(interrupt_pfd->fd, &dummy, 1)', 'dkrr_net_recv(interrupt_pfd->fd, &dummy, 1, 0)'),
                 ('write(registry_impl->interrupt_pipe_out, &dummy, 1)', 'dkrr_net_send(registry_impl->interrupt_pipe_out, &dummy, 1, 0)')]
        for old_call, new_call in pairs:
            if text.count(old_call) != 1:
                raise ValueError('Unexpected RTC wakeup consumer: ' + old_call)
            text = text.replace(old_call, new_call)
        dest = output / relative
        dest.parent.mkdir(parents=True, exist_ok=True)
        if not dest.exists() or dest.read_text() != text:
            dest.write_text(text)
        records.append(dict(old=str(old), new=str(dest), input_sha256=expected,
                            output_sha256=hashlib.sha256(text.encode()).hexdigest()))
    (output / 'RECEIPT.json').write_text(json.dumps(records, indent=2) + '\n')
    return records


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('source', type=Path)
    p.add_argument('output', type=Path)
    a = p.parse_args()
    stage(a.source.resolve(), a.output.resolve())
