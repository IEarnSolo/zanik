"""Inspect SPWN chunks and resolve their class IDs through a SPUD level META."""
import argparse
import json
import struct
from pathlib import Path
from inspect_spud_resources import Reader, chunks, parse_metadata

data = Path(argparse.ArgumentParser().parse_known_args()[1][0]).read_bytes()
root = Reader(data)
magic, level, _ = root.chunk()
if magic != "LEVL":
    raise SystemExit("not LEVL")
level.fstring(); level.take(8)
meta = None; lats = None; sats = None
for tag, child in chunks(level):
    if tag == "META": meta = parse_metadata(child)
    elif tag == "LATS": lats = child
    elif tag == "SATS": sats = child
rows=[]
for tag, child in chunks(sats):
    if tag != "SPWN": continue
    start=child.pos
    class_id=child.u32()
    rows.append({"classId":class_id,"class":meta["classes"][class_id],"payloadHex":data[child.pos:child.end].hex()})
print(json.dumps(rows,indent=2))
