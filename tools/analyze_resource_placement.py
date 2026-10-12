#!/usr/bin/env python3
import json
import math
import sys
import csv
from collections import defaultdict
from pathlib import Path

from PIL import Image


def main():
    catalog_path = Path(sys.argv[1] if len(sys.argv) > 1 else "dist/resource-classification-catalog.json")
    map_path = Path(sys.argv[2] if len(sys.argv) > 2 else "web/world-map.jpg")
    catalog = json.loads(catalog_path.read_text(encoding="utf-8"))
    image = Image.open(map_path).convert("RGB")
    water = (79, 107, 110)
    by_type = defaultdict(list)
    for node in catalog["nodes"]:
        px = min(image.width - 1, max(0, int(node["x"] * image.width)))
        py = min(image.height - 1, max(0, int(node["y"] * image.height)))
        colour = image.getpixel((px, py))
        distance = math.sqrt(sum((colour[i] - water[i]) ** 2 for i in range(3)))
        by_type[node["type"]].append(distance)
    for resource in catalog["types"]:
        distances = by_type[resource["id"]]
        counts = [sum(value < threshold for value in distances) for threshold in (5, 12, 25)]
        print(f"{resource['id']}\t{len(distances)}\t{counts[0]}\t{counts[1]}\t{counts[2]}")
    private_path = catalog_path.with_name("resource-classification-private.tsv")
    if private_path.exists():
        heights = defaultdict(list)
        with private_path.open(encoding="utf-8", newline="") as source:
            for row in csv.reader(source, delimiter="\t"):
                if not row or row[0].startswith("#") or len(row) < 8 or not row[7]:
                    continue
                heights[row[2]].append(float(row[7]))
        print("\nheight distribution: type count min p01 median p99 max below0 below-10000")
        for resource, values in sorted(heights.items()):
            values.sort()
            print(resource, len(values), values[0], values[int(len(values) * .01)],
                  values[len(values) // 2], values[int(len(values) * .99)], values[-1],
                  sum(value < 0 for value in values), sum(value < -10000 for value in values))


if __name__ == "__main__":
    main()
