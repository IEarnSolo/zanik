"""List SPUD actors filtered by case-insensitive class/object-name terms."""
import argparse
import json
from pathlib import Path
from inspect_spud_resources import Reader, chunks, parse_level, actor_properties

ap = argparse.ArgumentParser()
ap.add_argument("save")
ap.add_argument("terms", nargs="+")
args = ap.parse_args()
terms = [term.casefold() for term in args.terms]
root = Reader(Path(args.save).read_bytes())
_, save, _ = root.chunk()
version = 8
rows = []
for magic, chunk in chunks(save):
    if magic == "INFO":
        version = chunk.u16()
    elif magic == "LVLS":
        for level_magic, level in chunks(chunk):
            if level_magic != "LEVL":
                continue
            level_name, meta, actors = parse_level(level, version)
            if not meta:
                continue
            for actor in actors:
                class_name = meta["classes"][actor["class_id"]]
                haystack = (class_name + " " + actor["name"]).casefold()
                if not any(term in haystack for term in terms):
                    continue
                rows.append({
                    "level": level_name,
                    "class": class_name,
                    "name": actor["name"],
                    "properties": actor_properties(actor, meta),
                    "customHex": (actor["custom"] or b"").hex(),
                    "coreHex": (actor["core"] or b"").hex(),
                })
print(json.dumps(rows, indent=2))
