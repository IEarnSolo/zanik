"""Print a bounded SPUD chunk tree with offsets and magic counts."""
import argparse
import collections
import struct
from pathlib import Path

data = Path(argparse.ArgumentParser().parse_known_args()[1][0]).read_bytes()

def walk(start, end, depth=0):
    counts = collections.Counter()
    pos = start
    samples = []
    while pos + 8 <= end:
        magic = data[pos:pos+4].decode("ascii", "replace")
        size = struct.unpack_from("<I", data, pos + 4)[0]
        child_start, child_end = pos + 8, pos + 8 + size
        if child_end > end:
            break
        counts[magic] += 1
        if len(samples) < 20:
            samples.append((magic, pos, size))
        pos = child_end
    print("  " * depth + f"{start:#x}-{end:#x} counts={dict(counts)} samples=" + ", ".join(f"{m}@{p:#x}:{s}" for m,p,s in samples))

# Known chunks can contain a short header before nested chunks, so locate and
# summarize every chunk magic of interest directly as a structural aid.
for marker in (b"SAVE", b"LVLS", b"LEVL", b"META", b"LATS", b"NOBJ", b"PROP", b"CUST", b"CORA"):
    positions=[]; pos=0
    while len(positions)<20:
        pos=data.find(marker,pos)
        if pos<0: break
        if pos+8<=len(data): positions.append(pos)
        pos+=4
    print(marker.decode(), len(positions), [hex(x) for x in positions])
