"""Find exact address ranges for symbol names containing any requested text."""
import argparse
import mmap
import struct

parser = argparse.ArgumentParser()
parser.add_argument("sym")
parser.add_argument("patterns", nargs="+")
args = parser.parse_args()
patterns = [pattern.encode().lower() for pattern in args.patterns]

with open(args.sym, "rb") as source, mmap.mmap(source.fileno(), 0, access=mmap.ACCESS_READ) as data:
    count = struct.unpack_from("<I", data, 0)[0]
    names_base = 4 + count * 20
    matching_offsets = {}
    cursor = names_base
    while cursor < len(data):
        end = data.find(b"\n", cursor)
        if end < 0:
            end = len(data)
        raw = data[cursor:end]
        if any(pattern in raw.lower() for pattern in patterns):
            matching_offsets[cursor - names_base] = raw.decode("utf-8", "replace")
        cursor = end + 1

    ranges = {}
    unpack = struct.Struct("<QQI").unpack_from
    for index in range(count):
        address, _, name_offset = unpack(data, 4 + index * 20)
        name = matching_offsets.get(name_offset)
        if name is None:
            continue
        span = ranges.setdefault(name, [address, address])
        span[0] = min(span[0], address)
        span[1] = max(span[1], address)

    for name, span in sorted(ranges.items(), key=lambda item: item[1][0]):
        print(f"{span[0]:09x}-{span[1]:09x} {name}")
