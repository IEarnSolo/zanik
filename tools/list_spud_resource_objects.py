"""List SPUD objects whose class or object name relates to resource state."""
import argparse
import json
from pathlib import Path
from inspect_spud_resources import Reader, chunks, parse_level, actor_properties

ap = argparse.ArgumentParser()
ap.add_argument("save")
args = ap.parse_args()
data = Path(args.save).read_bytes()
root = Reader(data)
magic, save, _ = root.chunk()
version = 8
rows = []
for magic, c in chunks(save):
    if magic == "INFO":
        version = c.u16()
    elif magic == "LVLS":
        for lm, level in chunks(c):
            if lm != "LEVL":
                continue
            level_name, meta, actors = parse_level(level, version)
            if not meta:
                continue
            for actor in actors:
                class_name = meta["classes"][actor["class_id"]]
                if any(term in class_name + actor["name"] for term in ("Resource", "Foliage", "Spawner_")):
                    rows.append({
                        "level": level_name,
                        "class": class_name,
                        "name": actor["name"],
                        "properties": actor_properties(actor, meta),
                        "custom": len(actor["custom"] or b""),
                        "core": len(actor["core"] or b""),
                    })
print(json.dumps(rows, indent=2))
