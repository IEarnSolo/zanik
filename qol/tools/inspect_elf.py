"""Print ELF64 sections and relevant dynamic symbols without external packages."""

import argparse
import struct


def read_at(file, offset, size):
    file.seek(offset)
    return file.read(size)


def cstring(data, offset):
    end = data.find(b"\0", offset)
    return data[offset:end if end >= 0 else None].decode("utf-8", "replace")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("binary")
    parser.add_argument("--match", default="Compost|Equip|Inventory|Bucket")
    args = parser.parse_args()
    needles = args.match.lower().split("|")
    with open(args.binary, "rb") as file:
        hdr = read_at(file, 0, 64)
        if hdr[:5] != b"\x7fELF\x02" or hdr[5] != 1:
            raise SystemExit("expected little-endian ELF64")
        shoff = struct.unpack_from("<Q", hdr, 40)[0]
        shentsize, shnum, shstrndx = struct.unpack_from("<HHH", hdr, 58)
        sections = []
        for i in range(shnum):
            raw = read_at(file, shoff + i * shentsize, 64)
            sections.append(struct.unpack("<IIQQQQIIQQ", raw))
        names_section = sections[shstrndx]
        names = read_at(file, names_section[4], names_section[5])
        for i, sec in enumerate(sections):
            name = cstring(names, sec[0])
            print(f"section {i}: {name}, size={sec[5]}, offset={sec[4]}")
        for sec in sections:
            if sec[1] not in (2, 11):
                continue
            strings_section = sections[sec[6]]
            strings = read_at(file, strings_section[4], strings_section[5])
            symbols = read_at(file, sec[4], sec[5])
            for offset in range(0, len(symbols), sec[9]):
                name_offset, info, other, index, address, size = struct.unpack_from(
                    "<IBBHQQ", symbols, offset
                )
                if name_offset < len(strings):
                    name = cstring(strings, name_offset)
                    if any(needle in name.lower() for needle in needles):
                        print(f"symbol 0x{address:x} {name}")


if __name__ == "__main__":
    main()
