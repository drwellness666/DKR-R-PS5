#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Fetch pinned public sources into a dedicated project build directory."""
import hashlib
import json
from pathlib import Path
import shutil
import sys
import tarfile
import urllib.request

def main():
    if len(sys.argv) != 2:
        raise SystemExit("Usage: fetch_vulkan_sources.py ABSOLUTE_GPU_WORK")
    work = Path(sys.argv[1])
    if not work.is_absolute():
        raise SystemExit("GPU work directory must be absolute")
    root = Path(__file__).resolve().parent.parent
    (work / "downloads").mkdir(parents=True, exist_ok=True)
    sources = json.loads((root / "VULKAN_SOURCES.json").read_text())["sources"]
    for item in sources:
        archive = (root / "vendor" if item.get("bundled") else work / "downloads") / item["file"]
        if not archive.exists():
            print("Downloading pinned source:", item["name"], flush=True)
            temporary = archive.with_suffix(archive.suffix + ".part")
            with urllib.request.urlopen(item["url"], timeout=90) as response, temporary.open("wb") as output:
                shutil.copyfileobj(response, output)
            if hashlib.sha256(temporary.read_bytes()).hexdigest() != item["sha256"]:
                raise SystemExit("Downloaded source checksum mismatch: " + item["name"])
            temporary.rename(archive)
        if hashlib.sha256(archive.read_bytes()).hexdigest() != item["sha256"]:
            raise SystemExit("Source checksum mismatch: " + item["name"])
        target = work / item["name"]
        if item["name"] in ("volk", "VulkanMemoryAllocator", "Vulkan-Headers"):
            target = work / "Plume/contrib" / item["name"]
        stamp = target / ".dkrr-source-sha256"
        if stamp.exists():
            if stamp.read_text().strip() != item["sha256"]:
                raise SystemExit("Existing source directory has another pin: " + str(target))
            continue
        # An unstamped populated tree might contain a developer's edits.
        if target.exists() and any(target.iterdir()):
            raise SystemExit("Refusing to overwrite an unstamped source tree: " + str(target))
        target.mkdir(parents=True, exist_ok=True)
        with tarfile.open(archive) as source:
            for member in source.getmembers():
                if item["strip_components"]:
                    parts = member.name.split("/", 1)
                    if len(parts) < 2 or not parts[1]: continue
                    member.name = parts[1]
                source.extract(member, target, filter="data")
        stamp.write_text(item["sha256"] + "\n")
        print("Verified source:", item["name"], flush=True)

if __name__ == "__main__":
    main()
