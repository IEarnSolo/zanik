#!/usr/bin/env python3
"""Build the storage plan-item manifest and per-item inventory icons."""

import argparse
import hashlib
import json
import shutil
from pathlib import Path

try:
    from asset_filesystem import copy_file, read_bytes, read_json
except ModuleNotFoundError:  # Imported as tools.* by repository tests.
    from tools.asset_filesystem import copy_file, read_bytes, read_json


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--assets", type=Path, default=Path("dist/recipe-unlock-all-json-current"))
    parser.add_argument("--icons", type=Path, default=Path("dist/storage-all-icons-current"))
    parser.add_argument("--web", type=Path, default=Path("web"))
    args = parser.parse_args()

    icon_matches = list(args.icons.rglob("T_Icon_Plans.png"))
    icon_hashes = {hashlib.sha256(read_bytes(path)).hexdigest() for path in icon_matches}
    if not icon_matches or len(icon_hashes) != 1:
        raise SystemExit(f"Expected one unique T_Icon_Plans.png, found {len(icon_matches)} exports with {len(icon_hashes)} hashes")

    records = []
    for path in sorted(args.assets.rglob("DA_Consumable_Plan*.json")):
        exports = read_json(path)
        matches = [value for value in exports if value.get("Name") == path.stem]
        if len(matches) != 1:
            raise ValueError(f"{path}: expected one matching export")
        props = matches[0].get("Properties", {})
        name = props.get("Name", {}).get("SourceString", "")
        persistence = props.get("PersistenceID", "")
        icon_asset = props.get("Icon", {}).get("AssetPathName", "")
        if not name.startswith("PLAN: ") or len(persistence) != 22 or not icon_asset.endswith("/T_Icon_Plans.T_Icon_Plans"):
            raise ValueError(f"{path}: invalid plan metadata")
        records.append((path.stem, name, persistence, f"{path.stem}.png"))

    if not records:
        raise SystemExit("No plan item records found")
    fingerprint = hashlib.sha256("\n".join("\t".join(row) for row in records).encode()).hexdigest()
    item_dir = args.web / "items"
    item_dir.mkdir(parents=True, exist_ok=True)
    for key, _, _, filename in records:
        copy_file(icon_matches[0], item_dir / filename)

    lines = [f"# zanik-plan-items\t1\t{len(records)}\t{fingerprint}"]
    lines.extend("\t".join(row) for row in records)
    (args.web / "plan-items.tsv").write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(json.dumps({"plans": len(records), "fingerprint": fingerprint}))


if __name__ == "__main__":
    main()
