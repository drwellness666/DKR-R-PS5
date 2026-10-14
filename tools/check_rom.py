#!/usr/bin/env python3
"""Validate user-supplied DKR ROMs read-only; never extract or copy game data."""
import argparse
import hashlib
import json
from pathlib import Path

KNOWN = {"0cb115d8716dbbc2922fda38e533b9fe63bb9670": "US v1.0 / v77",
         "6d96743d46f8c0cd0edb0ec5600b003c89b93755": "US Rev A v1.1 / v80"}
PAL_IDENTITIES = {"dd5d64dd140cb7aa28404fa35abdcaba33c29260": "EUR/PAL v1.0 / pal.v77",
                  "b7f628073237b3d211d40406aa0884ff8fdd70d5": "EUR/PAL v1.1 / pal.v80"}

def canonical(data):
    magic = data[:4]
    if magic == bytes.fromhex("80371240"):
        return data, "z64 big endian"
    if magic == bytes.fromhex("37804012") and len(data) % 2 == 0:
        result = bytearray(len(data))
        result[0::2], result[1::2] = data[1::2], data[0::2]
        return result, "v64 byte swapped"
    if magic == bytes.fromhex("40123780") and len(data) % 4 == 0:
        result = bytearray(len(data))
        for i in range(4):
            result[i::4] = data[3-i::4]
        return result, "n64 little endian"
    raise ValueError("Unrecognised N64 header or invalid alignment")

def check(path):
    path = Path(path)
    if path.stat().st_size != 12 * 1024 * 1024:
        raise ValueError("Expected a clean 12 MiB DKR image")
    data, order = canonical(path.read_bytes())
    digest = hashlib.sha1(data).hexdigest()
    result = {"name": path.name, "size": len(data), "byte_order": order,
            "canonical_sha1": digest, "revision": KNOWN.get(digest),
            "supported": digest in KNOWN}
    if digest in PAL_IDENTITIES:
        result.update(identified_revision=PAL_IDENTITIES[digest],
                      error="Known EUR/PAL dump; native PAL CPU payload and runtime support are not implemented yet")
    return result

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("rom", nargs="+", type=Path)
    args = p.parse_args()
    ok = True
    for path in args.rom:
        try:
            result = check(path)
        except (OSError, ValueError) as e:
            result = {"name": path.name, "supported": False, "error": str(e)}
        print(json.dumps(result, ensure_ascii=False))
        ok &= result["supported"]
    return 0 if ok else 1

if __name__ == "__main__":
    raise SystemExit(main())
