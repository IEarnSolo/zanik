"""Inspect direct relative calls near a named server function.

This is an exploration aid, not a disassembler: E8 bytes inside other
instructions may appear as false positives. Confirm candidates in a real
disassembler before using them for a hook.
"""

import argparse
import bisect
import mmap
import struct


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("binary")
    parser.add_argument("sym")
    parser.add_argument("name")
    parser.add_argument("--bytes", type=int, default=512)
    args = parser.parse_args()
    with open(args.sym, "rb") as file, mmap.mmap(file.fileno(), 0, access=mmap.ACCESS_READ) as symbols:
        count = struct.unpack_from("<I", symbols, 0)[0]
        names_base = 4 + count * 20
        addresses = []
        names = []
        for i in range(count):
            address, _, name_offset = struct.unpack_from("<QQI", symbols, 4 + i * 20)
            if addresses and addresses[-1] == address:
                continue
            end = symbols.find(b"\n", names_base + name_offset)
            name = symbols[names_base + name_offset:end].decode("utf-8", "replace")
            addresses.append(address)
            names.append(name)
        targets = [(a, n) for a, n in zip(addresses, names) if args.name in n]
        for address, name in targets[:10]:
            print(f"\n0x{address:x} {name}")
            # This build's .sym addresses are ELF virtual addresses minus
            # 0x200000. The ELF .text file offset is virtual address -0x201000.
            with open(args.binary, "rb") as file:
                file.seek(address - 0x1000)
                code = file.read(args.bytes)
            for i in range(len(code) - 4):
                if code[i] != 0xE8:
                    continue
                delta = struct.unpack_from("<i", code, i + 1)[0]
                destination = address + i + 5 + delta
                position = bisect.bisect_right(addresses, destination) - 1
                if position >= 0 and destination - addresses[position] < 0x10000:
                    print(f"  +0x{i:x} -> 0x{destination:x} {names[position]}")


if __name__ == "__main__":
    main()
