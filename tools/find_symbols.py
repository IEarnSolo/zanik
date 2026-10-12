"""Print exact or substring matches from a Dragonwilds binary .sym table."""

import argparse
import mmap
import struct


parser = argparse.ArgumentParser()
parser.add_argument("sym")
parser.add_argument("queries", nargs="+")
args = parser.parse_args()

with open(args.sym, "rb") as source, mmap.mmap(
    source.fileno(), 0, access=mmap.ACCESS_READ
) as data:
    count = struct.unpack_from("<I", data, 0)[0]
    names_base = 4 + count * 20
    for index in range(count):
        address, size, name_offset = struct.unpack_from("<QQI", data, 4 + index * 20)
        start = names_base + name_offset
        end = data.find(b"\n", start)
        if end < 0:
            end = len(data)
        name = data[start:end].decode("utf-8", "replace")
        if any(query in name for query in args.queries):
            print(f"0x{address:x}\t0x{size:x}\t{name}")
