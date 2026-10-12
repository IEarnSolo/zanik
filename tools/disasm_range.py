"""Disassemble a Dragonwilds Linux module offset range from the matching ELF."""
import argparse
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_64

p = argparse.ArgumentParser()
p.add_argument("elf")
p.add_argument("start", type=lambda x: int(x, 0))
p.add_argument("end", type=lambda x: int(x, 0))
args = p.parse_args()

# The shipping ELF's executable PT_LOAD has p_vaddr - p_offset == 0x1000.
file_offset = args.start - 0x1000
code = Path(args.elf).read_bytes()[file_offset:file_offset + args.end - args.start]
md = Cs(CS_ARCH_X86, CS_MODE_64)
md.detail = False
for insn in md.disasm(code, args.start):
    print(f"{insn.address:09x}: {insn.mnemonic:8} {insn.op_str}")
