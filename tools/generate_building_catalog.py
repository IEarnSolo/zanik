#!/usr/bin/env python3
"""Generate the authoritative Zanik building catalogue from cooked asset JSON."""

import argparse
import hashlib
import json
import shutil
import struct
from pathlib import Path

try:
    from asset_filesystem import copy_file, read_bytes, read_json
except ModuleNotFoundError:  # Imported as tools.* by repository tests.
    from tools.asset_filesystem import copy_file, read_bytes, read_json

EXPECTED = {"tabs": 8, "sections": 63, "references": 844, "pieces": 827}
EXPECTED_ICON_SOURCES = {
    "authored-texture": 758,
    "embedded-build-ui-thumbnail": 68,
    "procedural-thumbnail": 1,
}
REMOVED_RUNTIME_PIECES = {
    "DA_BaseBuilding_Building_Protection_Totem",
    "DA_T4_Floor_Large02",
}


def clean(value):
    return str(value or "").replace("\t", " ").replace("\r", " ").replace("\n", " ").strip()


def asset_key(path):
    return path.rsplit(".", 1)[-1]


def scan_assets(root):
    assets = {}
    for path in root.rglob("*.json"):
        try:
            records = read_json(path)
        except (OSError, UnicodeError, json.JSONDecodeError) as error:
            raise RuntimeError(f"cannot read exported JSON {path.name}: {error}") from error
        for record in records if isinstance(records, list) else []:
            if record.get("Type") != "BuildingPieceData":
                continue
            props = record.get("Properties", {})
            key = record.get("Name", "")
            requirements = []
            for req in props.get("Requirements", []):
                item = req.get("ItemData") or {}
                item_key = item.get("ObjectPath", "").split(".", 1)[0].rsplit("/", 1)[-1] if item.get("ObjectPath") else ""
                if not item_key and item.get("ObjectName"):
                    item_key = item["ObjectName"].split("'", 1)[-1].rstrip("'")
                if item_key:
                    requirements.append({"item": item_key, "quantity": int(req.get("Amount", 0))})
            icon_path = props.get("DisplayIcon", {}).get("AssetPathName", "")
            actor_path = props.get("BuildableActor", {}).get("AssetPathName", "")
            assets[key] = {
                "key": key,
                "assetPath": record.get("Package", "") + "." + key,
                "name": props.get("DisplayName", {}).get("LocalizedString") or props.get("DisplayName", {}).get("SourceString") or key,
                "description": props.get("Description", {}).get("LocalizedString") or props.get("Description", {}).get("SourceString") or "",
                "iconAsset": icon_path,
                "icon": key + ".png",
                "actor": asset_key(actor_path),
                "requirements": requirements,
            }
    return assets


def cooked_package_path(value):
    package = str(value or "").split(".", 1)[0]
    if package.startswith("/Game/"):
        return "RSDragonwilds/Content/" + package[len("/Game/"):]
    parts = package.strip("/").split("/", 1)
    if len(parts) == 2:
        return f"RSDragonwilds/Plugins/GameFeatures/{parts[0]}/Content/{parts[1]}"
    return ""


def icon_source(icon_asset):
    normalized = str(icon_asset or "").replace("\\", "/")
    if "/GeneratedIcons/SM_" in normalized:
        return "embedded-build-ui-thumbnail"
    if "/ProceduralThumbnails/" in normalized:
        return "procedural-thumbnail"
    return "authored-texture"


def find_icon(icon_index, basename_index, icon_asset):
    if not icon_asset:
        return None
    package = cooked_package_path(icon_asset).casefold()
    if package and package in icon_index:
        return icon_index[package]
    candidates = basename_index.get((asset_key(icon_asset) + ".png").casefold(), [])
    if len(candidates) == 1:
        return candidates[0]
    if len(candidates) > 1:
        raise RuntimeError(f"ambiguous icon basename for {icon_asset}: {len(candidates)} exports")
    return None


def png_dimensions(path):
    value = read_bytes(path)[:24]
    if len(value) != 24 or value[:8] != b"\x89PNG\r\n\x1a\n" or value[12:16] != b"IHDR":
        raise RuntimeError(f"building icon is not a valid PNG: {path}")
    return struct.unpack(">II", value[16:24])


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("assets", type=Path, help="Root containing exported building-piece JSON")
    parser.add_argument("icons", type=Path, help="Root containing exported PNG textures")
    parser.add_argument("output", type=Path, help="web/buildings output directory")
    parser.add_argument("--catalogue-root", type=Path, help="Separate root containing DA_BuildPieceCatalogue_Default.json")
    parser.add_argument("--extra-assets", type=Path, action="append", default=[], help="Additional exported building-piece JSON root")
    parser.add_argument("--extra-icons", type=Path, action="append", default=[], help="Additional exported PNG root")
    args = parser.parse_args()

    catalogue_path = next((args.catalogue_root or args.assets).rglob("DA_BuildPieceCatalogue_Default.json"))
    catalogue_record = read_json(catalogue_path)[0]
    props = catalogue_record["Properties"]
    assets = scan_assets(args.assets)
    for extra in args.extra_assets:
        assets.update(scan_assets(extra))
    tabs, references, unique = [], [], set()
    for tab_index, page in enumerate(props["Pages"]):
        tab = {"label": page["Label"]["LocalizedString"], "key": page["Label"]["Key"], "order": tab_index, "sections": []}
        for section_index, source_section in enumerate(page["Collection"]):
            section = {"label": source_section["Label"]["LocalizedString"], "key": source_section["Label"]["Key"], "order": section_index,
                       "favourites": bool(source_section.get("bFavouritesCategory")), "pieces": []}
            for piece_index, ref in enumerate(source_section["Collection"]):
                key = asset_key(ref["AssetPathName"])
                if key in REMOVED_RUNTIME_PIECES:
                    continue
                section["pieces"].append(key)
                references.append((tab_index, section_index, piece_index, key))
                unique.add(key)
            tab["sections"].append(section)
        tabs.append(tab)

    counts = {"tabs": len(tabs), "sections": sum(len(t["sections"]) for t in tabs),
              "references": len(references), "pieces": len(unique)}
    if counts != EXPECTED:
        raise SystemExit(f"catalogue shape changed: expected {EXPECTED}, got {counts}")
    missing = sorted(unique - assets.keys())
    if missing:
        raise SystemExit(f"{len(missing)} catalogue assets were not exported; first: {missing[0]}")

    args.output.mkdir(parents=True, exist_ok=True)
    icon_index = {}
    basename_index = {}
    for icon_root in [args.icons, *args.extra_icons]:
        for source in icon_root.rglob("*.png"):
            parts = source.as_posix().split("/")
            try:
                start = parts.index("RSDragonwilds")
            except ValueError:
                start = -1
            if start >= 0:
                package = "/".join(parts[start:])[:-4].casefold()
                prior = icon_index.get(package)
                if prior and read_bytes(prior) != read_bytes(source):
                    raise RuntimeError(f"conflicting exports for cooked icon package {package}")
                icon_index[package] = source
            basename_index.setdefault(source.name.casefold(), []).append(source)
    missing_icons = []
    source_counts = {key: 0 for key in EXPECTED_ICON_SOURCES}
    for key in sorted(unique):
        source_kind = icon_source(assets[key]["iconAsset"])
        source = find_icon(icon_index, basename_index, assets[key]["iconAsset"])
        if source:
            width, height = png_dimensions(source)
            if width < 256 or height < 256:
                raise RuntimeError(f"building icon is undersized ({width}x{height}): {key}")
            copy_file(source, args.output / assets[key]["icon"])
            assets[key]["iconSource"] = source_kind
            source_counts[source_kind] += 1
        else:
            missing_icons.append(key)
    if missing_icons:
        raise SystemExit(f"{len(missing_icons)} building icons were not exported; first: {missing_icons[0]}")
    if source_counts != EXPECTED_ICON_SOURCES:
        raise SystemExit(f"building icon coverage changed: expected {EXPECTED_ICON_SOURCES}, got {source_counts}")

    pieces = [assets[key] for key in sorted(unique)]
    canonical = {"schemaVersion": 1, "sourceAsset": catalogue_record["Package"], "counts": counts,
                 "iconSources": source_counts,
                 "tabs": tabs, "pieces": pieces}
    encoded = json.dumps(canonical, ensure_ascii=False, separators=(",", ":")).encode()
    canonical["contentHash"] = hashlib.sha256(encoded).hexdigest()
    (args.output / "catalog.json").write_text(json.dumps(canonical, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    with (args.output / "catalog.tsv").open("w", encoding="utf-8", newline="\n") as out:
        out.write(f"# zanik-building-catalog\t1\t{counts['tabs']}\t{counts['sections']}\t{counts['references']}\t{counts['pieces']}\t{canonical['contentHash']}\n")
        for tab in tabs:
            out.write("T\t{}\t{}\t{}\n".format(tab["order"], clean(tab["key"]), clean(tab["label"])))
            for section in tab["sections"]:
                out.write("S\t{}\t{}\t{}\t{}\t{}\n".format(tab["order"], section["order"], int(section["favourites"]), clean(section["key"]), clean(section["label"])))
                for order, key in enumerate(section["pieces"]):
                    out.write(f"M\t{tab['order']}\t{section['order']}\t{order}\t{clean(key)}\n")
        for piece in pieces:
            out.write("P\t{}\t{}\t{}\t{}\t{}\t{}\t{}\n".format(clean(piece["key"]), clean(piece["name"]), clean(piece["icon"]),
                      clean(piece["actor"]), clean(piece["assetPath"]), clean(piece["description"]), len(piece["requirements"])))

    with (args.output / "metadata.tsv").open("w", encoding="utf-8", newline="\n") as out:
        out.write("# Generated from DA_BuildPieceCatalogue_Default. key<TAB>localized name<TAB>web icon filename<TAB>buildable actor class\n")
        for piece in pieces:
            out.write("{}\t{}\t{}\t{}\n".format(clean(piece["key"]), clean(piece["name"]), clean(piece["icon"]), clean(piece["actor"])))
    print(json.dumps({**counts, "iconSources": source_counts, "placeholderIcons": 0}, sort_keys=True))


if __name__ == "__main__":
    main()
