"""Find x86-64 direct CALL references to a .sym function.

Dragonwilds .sym addresses are ELF virtual addresses minus 0x200000.
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
    args = parser.parse_args()

    with open(args.sym, "rb") as f, mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ) as data:
        count = struct.unpack_from("<I", data, 0)[0]
        names_base = 4 + count * 20
        symbols = []
        for i in range(count):
            address, _, name_offset = struct.unpack_from("<QQI", data, 4 + i * 20)
            end = data.find(b"\n", names_base + name_offset)
            name = data[names_base + name_offset:end].decode("utf-8", "replace")
            symbols.append((address, name))

    symbols.sort()
    unique = []
    for item in symbols:
        if not unique or unique[-1][0] != item[0]:
            unique.append(item)
    addresses = [item[0] for item in unique]
    exact_targets = [(address, name) for address, name in symbols if name == args.name]
    targets = exact_targets or [(address, name) for address, name in symbols if args.name in name]
    if not targets:
        raise SystemExit("target not found")
    target_sym, target_name = min(targets)

    # Exact section values for the matched server build. File offset is VMA - 0x201000.
    text_sym_start = 0x4BA2000 - 0x200000
    text_size = 0x908BD8E
    text_file_offset = text_sym_start - 0x1000
    with open(args.binary, "rb") as f:
        f.seek(text_file_offset)
        code = f.read(text_size)

    print(f"target 0x{target_sym:x} {target_name}")
    for i in range(len(code) - 4):
        if code[i] != 0xE8:
            continue
        caller_address = text_sym_start + i
        delta = struct.unpack_from("<i", code, i + 1)[0]
        destination = caller_address + 5 + delta
        if destination != target_sym:
            continue
        position = bisect.bisect_right(addresses, caller_address) - 1
        symbol_address, symbol_name = unique[position]
        print(f"0x{caller_address:x} +0x{caller_address-symbol_address:x} {symbol_name}")


if __name__ == "__main__":
    main()
