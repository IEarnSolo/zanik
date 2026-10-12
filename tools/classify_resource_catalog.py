#!/usr/bin/env python3
"""Classify proven inaccessible resource placements without exposing world coordinates."""
import csv
import hashlib
import json
import math
import statistics
import sys
from collections import Counter, defaultdict
from pathlib import Path

from PIL import Image

WATER_RGB = (79, 107, 110)
WATER_DISTANCE = 5.0
GRID_SIZE = 0.02
BURIAL_DELTA = 10000.0


def water_distance(pixel):
    return math.dist(pixel, WATER_RGB)


def uniform_exterior_water(image, x, y):
    px = min(image.width - 1, max(0, int(x * image.width)))
    py = min(image.height - 1, max(0, int(y * image.height)))
    samples = []
    for oy in (-4, -2, 0, 2, 4):
        for ox in (-4, -2, 0, 2, 4):
            samples.append(water_distance(image.getpixel((min(image.width - 1, max(0, px + ox)),
                                                          min(image.height - 1, max(0, py + oy))))))
    return sum(value < WATER_DISTANCE for value in samples) >= 23


def main():
    if len(sys.argv) != 6:
        raise SystemExit("usage: classify_resource_catalog.py INPUT_JSON PRIVATE_TSV MAP_JPG OUTPUT_JSON REPORT_TSV")
    input_path, private_path, map_path, output_path, report_path = map(Path, sys.argv[1:])
    catalog = json.loads(input_path.read_text(encoding="utf-8"))
    image = Image.open(map_path).convert("RGB")
    private = {}
    with private_path.open(encoding="utf-8", newline="") as source:
        for row in csv.reader(source, delimiter="\t"):
            if not row or row[0].startswith("#") or len(row) < 9:
                continue
            private[row[1]] = {
                "stateKey": row[0], "type": row[2], "worldX": float(row[5]),
                "worldY": float(row[6]), "worldZ": float(row[7]) if row[7] else None,
                "sourceFile": row[8]
            }

    grid = defaultdict(list)
    for node in catalog["nodes"]:
        detail = private.get(node["id"])
        if detail and detail["worldZ"] is not None and not uniform_exterior_water(image, node["x"], node["y"]):
            grid[(int(node["x"] / GRID_SIZE), int(node["y"] / GRID_SIZE))].append(detail["worldZ"])

    report = []
    reasons = Counter()
    by_type = Counter()
    for node in catalog["nodes"]:
        detail = private.get(node["id"])
        reason = ""
        evidence = ""
        if uniform_exterior_water(image, node["x"], node["y"]):
            reason = "outside-playable-map"
            evidence = "uniform exterior water in authoritative map artwork"
        elif detail and detail["worldZ"] is not None:
            cell_x, cell_y = int(node["x"] / GRID_SIZE), int(node["y"] / GRID_SIZE)
            nearby = []
            for oy in (-1, 0, 1):
                for ox in (-1, 0, 1):
                    nearby.extend(grid.get((cell_x + ox, cell_y + oy), ()))
            if len(nearby) >= 8:
                local_reference = statistics.quantiles(nearby, n=4, method="inclusive")[2]
                delta = local_reference - detail["worldZ"]
                if delta > BURIAL_DELTA:
                    reason = "buried-below-local-surface"
                    evidence = f"altitude {delta:.0f} below local upper-quartile placement level"
        if reason:
            node["excluded"] = True
            node["exclusion"] = reason
            reasons[reason] += 1
            by_type[node["type"]] += 1
            report.append({"id": node["id"], "type": node["type"], "reason": reason,
                           "evidence": evidence, **(detail or {})})

    catalog["classification"] = {
        "version": "conservative-placement-v1",
        "excluded": len(report),
        "reasons": dict(sorted(reasons.items())),
        "policy": "Only uniform exterior-water placements and extreme local altitude outliers are excluded."
    }
    classification_hash = hashlib.sha256("\n".join(sorted(item["id"] for item in report)).encode()).hexdigest()
    catalog["version"] = f"{catalog['version']}-placement-{classification_hash[:8]}"
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(json.dumps(catalog, separators=(",", ":")), encoding="utf-8")
    report_path.parent.mkdir(parents=True, exist_ok=True)
    fields = ["id", "type", "reason", "evidence", "stateKey", "worldX", "worldY", "worldZ", "sourceFile"]
    with report_path.open("w", encoding="utf-8", newline="") as target:
        writer = csv.DictWriter(target, fieldnames=fields, delimiter="\t", extrasaction="ignore")
        writer.writeheader()
        writer.writerows(report)
    print(json.dumps({"excluded": len(report), "reasons": reasons, "types": by_type}, default=dict, sort_keys=True))


if __name__ == "__main__":
    main()
