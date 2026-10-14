#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Apply our pinned SDL patch only to exact original or already patched files."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    if len(sys.argv) != 2:
        raise SystemExit("Usage: apply_sdl_patch.py EXTRACTED_SDL_SOURCE")
    source = Path(sys.argv[1]).resolve()
    root = Path(__file__).resolve().parent.parent
    patch = root / "patches/0002-sdl-ps5-sized-video-buffers.patch"
    manifest = json.loads((root / "patches/sdl-patch-manifest.json").read_text())
    if digest(patch) != manifest["patch_sha256"]:
        raise SystemExit("SDL patch checksum mismatch")
    files = manifest["files"]
    current = {name: digest(source / name) for name in files}
    if all(current[name] == checks["patched"] for name, checks in files.items()):
        print("SDL low-memory patch already applied; file checksums match")
        return
    if not all(current[name] == checks["original"] for name, checks in files.items()):
        raise SystemExit("SDL source differs from pinned originals/patch; refusing to overwrite edits")
    command = ["patch", "--batch", "--forward", "--fuzz=0", "-p1", "-d", str(source), "-i", str(patch)]
    subprocess.run(command + ["--dry-run"], check=True)
    subprocess.run(command, check=True)
    if not all(digest(source / name) == checks["patched"] for name, checks in files.items()):
        raise SystemExit("Patched SDL file checksum mismatch")


if __name__ == "__main__":
    main()
