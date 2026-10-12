#!/usr/bin/env python3
import hashlib
import pathlib

root = pathlib.Path(__file__).resolve().parents[1]
lines = (root / "web" / "plan-items.tsv").read_text(encoding="utf-8").splitlines()
header = lines[0].split("\t")
assert header[:2] == ["# zanik-plan-items", "1"]
assert int(header[2]) == 123 == len(lines) - 1
records = [tuple(line.split("\t")) for line in lines[1:]]
assert all(len(row) == 4 for row in records)
assert len({row[0] for row in records}) == len(records)
assert all(row[0].startswith("DA_Consumable_Plan") for row in records)
assert all(row[1].startswith("PLAN: ") for row in records)
assert all((root / "web" / "items" / row[3]).stat().st_size > 5000 for row in records)
fingerprint = hashlib.sha256("\n".join("\t".join(row) for row in records).encode()).hexdigest()
assert header[3] == fingerprint
by_name = {row[1]: row for row in records}
for name in ("PLAN: Open Wooden Barrel", "PLAN: Wooden Barrel", "PLAN: Keg Barrel", "PLAN: Sealed Wooden Crate"):
    assert name in by_name

source = (root / "src" / "engine" / "admin_storage.cpp").read_text(encoding="utf-8")
assert 'set("Plans",85)' in source
assert "PlanItemCatalog" in source
