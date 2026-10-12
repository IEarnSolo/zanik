"""Find little-endian double coordinates in files."""
import argparse
import struct
from pathlib import Path

ap = argparse.ArgumentParser()
ap.add_argument("x")
ap.add_argument("y")
ap.add_argument("paths", nargs="+")
ap.add_argument("--hex", action="store_true")
args = ap.parse_args()
needles = ([bytes.fromhex(args.x), bytes.fromhex(args.y)] if args.hex else
           [struct.pack("<d", float(args.x)), struct.pack("<d", float(args.y))])
for raw in args.paths:
    path = Path(raw)
    for file in ([path] if path.is_file() else path.rglob("*")):
        if not file.is_file():
            continue
        data = file.read_bytes()
        hits = [[i for i in range(len(data)) if data.startswith(needle, i)][:20] for needle in needles]
        if any(hits):
            print(file, hits)
