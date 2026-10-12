#!/usr/bin/env python3
"""Print a deterministic semantic diff between recipe unlock catalogues."""

import json
import sys
from pathlib import Path


def load(path):
    data = json.loads(Path(path).read_text(encoding="utf-8"))
    return {(row["kind"], row["internalName"]): row for row in data["recipes"]}


before, after = load(sys.argv[1]), load(sys.argv[2])
for key in sorted(after.keys() - before.keys()):
    print("ADDED\t" + "\t".join(key))
for key in sorted(before.keys() - after.keys()):
    print("REMOVED\t" + "\t".join(key))
for key in sorted(before.keys() & after.keys()):
    old = {k: v for k, v in before[key].items() if k != "evidenceAssets"}
    new = {k: v for k, v in after[key].items() if k != "evidenceAssets"}
    if old != new:
        print("CHANGED\t" + "\t".join(key))

