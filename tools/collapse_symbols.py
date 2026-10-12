"""Collapse Breakpad-style line records into one address range per symbol."""
import argparse
import mmap
import re
import struct

p = argparse.ArgumentParser()
p.add_argument("sym")
p.add_argument("patterns", nargs="+")
p.add_argument("--min-address", type=lambda value: int(value, 0), default=0x08000000)
p.add_argument("--max-address", type=lambda value: int(value, 0), default=0x0C000000)
args = p.parse_args()
rx = re.compile("|".join(f"(?:{x})" for x in args.patterns), re.I)

with open(args.sym, "rb") as source, mmap.mmap(source.fileno(), 0, access=mmap.ACCESS_READ) as data:
    count = struct.unpack_from("<I", data, 0)[0]
    names_base = 4 + count * 20
    found = {}
    low, high = 0, count
    while low < high:
        mid = (low + high) // 2
        address = struct.unpack_from("<Q", data, 4 + mid * 20)[0]
        if address < args.min_address:
            low = mid + 1
        else:
            high = mid
    for index in range(low, count):
        address, _, name_offset = struct.unpack_from("<QQI", data, 4 + index * 20)
        # Dominion gameplay code in this build is below 0x0c000000. Skipping
        # the much larger engine tail keeps targeted reverse engineering fast.
        if address >= args.max_address:
            break
        start = names_base + name_offset
        end = data.find(b"\n", start)
        name = data[start:end].decode("utf-8", "replace")
        if not rx.search(name):
            continue
        span = found.setdefault(name, [address, address])
        span[0] = min(span[0], address)
        span[1] = max(span[1], address)
    for name, span in sorted(found.items(), key=lambda item: item[1][0]):
        print(f"{span[0]:09x}-{span[1]:09x} {name}")
