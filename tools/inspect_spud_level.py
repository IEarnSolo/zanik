"""Decode every actor in one SPUD .lvl cache file."""
import argparse
import json
from pathlib import Path
from inspect_spud_resources import Reader, parse_level, actor_properties

path = Path(argparse.ArgumentParser().parse_known_args()[1][0])
reader = Reader(path.read_bytes())
magic, payload, _ = reader.chunk()
if magic != "LEVL":
    raise SystemExit(f"expected LEVL, got {magic}")
level_name, meta, actors = parse_level(payload, 8)
rows = []
for actor in actors:
    rows.append({
        "level": level_name,
        "class": meta["classes"][actor["class_id"]],
        "name": actor["name"],
        "properties": actor_properties(actor, meta),
        "customHex": (actor["custom"] or b"").hex(),
        "coreHex": (actor["core"] or b"").hex(),
    })
print(json.dumps(rows, indent=2))
