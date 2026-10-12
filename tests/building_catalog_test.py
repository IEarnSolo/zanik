#!/usr/bin/env python3
import json
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[1]
catalog = json.loads((root / "web/buildings/catalog.json").read_text(encoding="utf-8"))
assert catalog["schemaVersion"] == 1
assert catalog["counts"] == {"tabs": 8, "sections": 63, "references": 839, "pieces": 822}
assert len(catalog["tabs"]) == 8
assert sum(len(tab["sections"]) for tab in catalog["tabs"]) == 63
assert sum(len(section["pieces"]) for tab in catalog["tabs"] for section in tab["sections"]) == 839

pieces = {piece["key"]: piece for piece in catalog["pieces"]}
assert len(pieces) == 822
for tab in catalog["tabs"]:
    for section in tab["sections"]:
        for key in section["pieces"]:
            assert key in pieces, key
for piece in pieces.values():
    assert piece["name"] and piece["assetPath"] and piece["actor"]
    assert (root / "web/buildings" / piece["icon"]).stat().st_size > 0

labels = {(tab["label"], section["label"]): section for tab in catalog["tabs"] for section in tab["sections"]}
assert ("Decoration", "Companions") in labels
assert len(labels[("Decoration", "Companions")]["pieces"]) == 7
assert ("Walls", "Doors & Windows") in labels
assert ("Walls", "Baroque Castle Walls") in labels
assert ("Beams & Pillars", "Kharidian Beams & Pillars") in labels
assert len({key for tab in catalog["tabs"] for section in tab["sections"] for key in section["pieces"]}) < 839

header = (root / "web/buildings/catalog.tsv").read_text(encoding="utf-8").splitlines()[0].split("\t")
assert header[:6] == ["# zanik-building-catalog", "1", "8", "63", "839", "822"]
print("building catalogue invariants passed")
