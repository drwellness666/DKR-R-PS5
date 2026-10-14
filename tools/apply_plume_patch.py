#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Apply the PS5 adapter only to the exact pinned Plume sources."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    if len(sys.argv) != 2:
        raise SystemExit("Usage: apply_plume_patch.py EXTRACTED_PLUME_SOURCE")
    source = Path(sys.argv[1]).resolve()
    root = Path(__file__).resolve().parent.parent
    patch = root / "patches/0003-plume-ps5-radv-display.patch"
    manifest = json.loads((root / "patches/plume-patch-manifest.json").read_text())
    if digest(patch) != manifest["patch_sha256"]:
        raise SystemExit("Plume patch checksum mismatch")
    files = manifest["files"]
    current = {name: digest(source / name) for name in files}
    if all(current[name] == checks["patched_sha256"] for name, checks in files.items()):
        print("Plume PS5 patch already applied; hashes match")
    else:
        if not all(current[name] == checks["original_sha256"] for name, checks in files.items()):
            raise SystemExit("Plume differs from pinned sources; refusing to overwrite edits")
        command = ["patch", "--batch", "--forward", "--fuzz=0", "-p1", "-d", str(source), "-i", str(patch)]
        subprocess.run(command + ["--dry-run"], check=True)
        subprocess.run(command, check=True)
        if not all(digest(source / name) == checks["patched_sha256"] for name, checks in files.items()):
            raise SystemExit("Patched Plume checksum mismatch")
    helper = source / "plume_ps5_display.h"
    expected = root / "src/plume_ps5_display.h"
    if helper.exists() and helper.read_bytes() != expected.read_bytes():
        raise SystemExit("Existing display adapter differs; refusing to overwrite edits")
    helper.write_bytes(expected.read_bytes())

if __name__ == "__main__":
    main()
