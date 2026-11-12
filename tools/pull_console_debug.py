#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Read the debug evidence of the last runs from a console over FTP, read-only.

Only LIST, MDTM, SIZE and RETR are ever sent. Nothing is written to, deleted
from or executed on the console. The host is given explicitly; there is no
discovery. Output: <out>/<timestamp>/ with the files and REPORT.txt.
"""
import argparse
import datetime
import ftplib
import pathlib
import re
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from ps5_crash_report import report  # noqa: E402
from check_import_baseline import unexpected_missing  # noqa: E402
import json  # noqa: E402

PROFILES = ('profile-us-v77', 'profile-us-v80')
FILES = ('logs/runtime.log', 'logs/runtime-previous.log', 'logs/storage-trace.log', 'logs/selftest.json')
MAX_BYTES = 8 * 1024 * 1024


class ReadOnlyFTP(ftplib.FTP):
    ALLOWED = ('LIST', 'MDTM', 'SIZE', 'RETR', 'TYPE', 'PASV', 'EPSV', 'USER', 'PASS', 'QUIT', 'NLST', 'FEAT', 'SYST', 'PWD', 'CWD')

    def putline(self, line):
        verb = line.split(' ', 1)[0].upper()
        if verb not in self.ALLOWED:
            raise RuntimeError(f'Refusing to send {verb}: this tool is read-only')
        super().putline(line)


def fetch(ftp, remote, local):
    chunks, total = [], 0

    def take(block):
        nonlocal total
        total += len(block)
        if total > MAX_BYTES:
            raise RuntimeError(f'{remote} is larger than {MAX_BYTES} bytes')
        chunks.append(block)

    ftp.retrbinary('RETR ' + remote, take)
    local.write_bytes(b''.join(chunks))


def listing(ftp, directory):
    rows = []
    ftp.retrlines('LIST ' + directory, rows.append)
    return [row.split(None, 8)[-1] for row in rows if len(row.split(None, 8)) >= 9]


def stale_image_notes(target, journals, installed_at):
    """A run that started after the install but reports another build id ran a stale image."""
    manifest = target / 'dkrr-install.manifest'
    if not manifest.is_file() or installed_at is None:
        return []
    lines = manifest.read_text(errors='replace').splitlines()
    installed = lines[1] if len(lines) > 1 else ''
    notes = []
    for journal in journals:
        parts = journal.name.split('-')  # run-<epoch>-<usec>-<pid>.log
        try:
            started = int(parts[1])
        except (IndexError, ValueError):
            continue
        match = [row for row in journal.read_text(errors='replace').splitlines() if row.startswith('DKRR_SESSION ')]
        build = next(iter(re.findall(r'build=(\S+)', match[0])), '') if match else ''
        if build and installed and started > installed_at and build != installed:
            notes.append(f'STALE IMAGE: {journal.name} started after the install but ran build {build}; the installed build is '
                         f'{installed}. The console still loaded the old image: restart the console (full power cycle), then start the game.')
    return notes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('host', help="the console's LAN address")
    parser.add_argument('--port', type=int, default=2121)
    parser.add_argument('--root', default='/data/dkrr')
    parser.add_argument('--runs', type=int, default=3, help='how many of the newest journals to fetch')
    parser.add_argument('--out', type=pathlib.Path, default=pathlib.Path('debug-pulls'))
    parser.add_argument('--symbols', type=pathlib.Path, default=pathlib.Path('symbols'))
    args = parser.parse_args()
    target = args.out / datetime.datetime.now().strftime('%Y%m%d-%H%M%S')
    target.mkdir(parents=True)
    ftp = ReadOnlyFTP()
    ftp.connect(args.host, args.port, timeout=15)
    ftp.login()
    notes = []
    names = sorted(n for n in listing(ftp, args.root + '/diagnostics') if n.startswith('run-') and n.endswith('.log'))
    journals = []
    for name in names[-args.runs:]:
        local = target / name
        try:
            fetch(ftp, f'{args.root}/diagnostics/{name}', local)
            journals.append(local)
        except ftplib.error_perm as error:
            notes.append(f'{name}: {error} (the journal is not world-readable; builds before 1.2.8 write mode 600)')
    for profile in PROFILES:
        for relative in FILES:
            local = target / f'{profile}-{relative.replace("/", "-")}'
            try:
                fetch(ftp, f'{args.root}/{profile}/{relative}', local)
            except ftplib.error_perm:
                pass
    installed_at = None
    for manifest in ('/data/homebrew/PPSA94678/dkrr-install.manifest',):
        try:
            fetch(ftp, manifest, target / 'dkrr-install.manifest')
            installed_at = datetime.datetime.strptime(ftp.sendcmd('MDTM ' + manifest)[4:18], '%Y%m%d%H%M%S').replace(
                tzinfo=datetime.timezone.utc).timestamp()
        except ftplib.error_perm as error:
            notes.append(f'{manifest}: {error}')
    ftp.quit()
    out = []
    baseline_file = pathlib.Path(__file__).resolve().parent.parent / 'docs/IMPORTS_BASELINE.json'
    baseline = json.loads(baseline_file.read_text()) if baseline_file.is_file() else None
    for journal in journals:
        out.append(f'=== {journal.name} ===')
        text = journal.read_text(errors='replace')
        out.append(report(text, args.symbols)[0])
        if baseline:
            for name in unexpected_missing(text, baseline):
                out.append(f'NEW UNRESOLVED IMPORT on the console: {name} (a call jumps to address 0); classify it in docs/IMPORTS_BASELINE.json')
    out.extend(stale_image_notes(target, journals, installed_at))
    out.extend(notes)
    (target / 'REPORT.txt').write_text('\n'.join(out) + '\n')
    print('\n'.join(out))
    print(f'\nFiles in {target}')


if __name__ == '__main__':
    main()
