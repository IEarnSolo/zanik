"""Find matching function addresses in Unreal's compact Linux .sym file."""

import argparse
import mmap
import struct


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("sym")
    parser.add_argument("terms", nargs="+")
    args = parser.parse_args()
    needles = tuple(term.lower().encode() for term in args.terms)
    with open(args.sym, "rb") as file, mmap.mmap(file.fileno(), 0, access=mmap.ACCESS_READ) as data:
        count = struct.unpack_from("<I", data, 0)[0]
        names_base = 4 + count * 20
        matches = {}
        cursor = names_base
        while cursor < len(data):
            end = data.find(b"\n", cursor)
            if end < 0:
                end = len(data)
            name = data[cursor:end]
            if any(needle in name.lower() for needle in needles):
                matches[cursor - names_base] = name.decode("utf-8", "replace")
            cursor = end + 1
        previous_name = None
        for record in range(count):
            address, _, name_offset = struct.unpack_from("<QQI", data, 4 + record * 20)
            if name_offset in matches and name_offset != previous_name:
                print(f"0x{address:016x} {matches[name_offset]}")
            previous_name = name_offset


if __name__ == "__main__":
    main()
