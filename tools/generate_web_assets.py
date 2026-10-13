#!/usr/bin/env python3
"""Copy only visual assets reachable from Zanik's web catalogs."""

from __future__ import annotations

import argparse
import binascii
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import zlib

try:
    from asset_filesystem import copy_file, extended_path, read_bytes, read_json
except ModuleNotFoundError:  # Imported as tools.* by repository tests.
    from tools.asset_filesystem import copy_file, extended_path, read_bytes, read_json


SKILLS = ("agility", "artisan", "attack", "construction", "cooking", "farming", "fishing", "magic", "mining", "ranged", "runecrafting", "woodcutting")
MAP_SIZE = 4096
# The exported landmass uses transparency for ocean pixels. Some transparent
# texels contain white RGB values, so saving it directly as JPEG creates white
# seams. This color matches the ocean used by Zanik's original web map.
MAP_OCEAN_RGB = (79, 107, 110)


def png_chunk(kind: bytes, data: bytes) -> bytes:
    return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", binascii.crc32(kind + data) & 0xffffffff)


def placeholder(path: Path, color: tuple[int, int, int, int]) -> None:
    width = height = 32
    row = bytes([0]) + bytes(color) * width
    data = b"\x89PNG\r\n\x1a\n" + png_chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0))
    data += png_chunk(b"IDAT", zlib.compress(row * height, 9)) + png_chunk(b"IEND", b"")
    path.parent.mkdir(parents=True, exist_ok=True); path.write_bytes(data)


def index_pngs(root: Path) -> dict[str, Path]:
    result: dict[str, Path] = {}
    for path in root.rglob("*.png"):
        result.setdefault(path.stem.casefold(), path)
    return result


def records(root: Path):
    for path in root.rglob("*.json"):
        if path.name == ".uas-manifest.json": continue
        try: value = read_json(path)
        except (OSError, UnicodeError, json.JSONDecodeError) as error:
            raise RuntimeError(f"cannot read exported JSON {path.name}: {error}") from error
        for record in value if isinstance(value, list) else []:
            if isinstance(record, dict): yield record


def asset_base(value) -> str:
    raw = str((value or {}).get("AssetPathName") or (value or {}).get("ObjectPath") or "") if isinstance(value, dict) else ""
    return raw.rsplit("/", 1)[-1].split(".", 1)[0]


def copy(index: dict[str, Path], name: str, destination: Path) -> bool:
    source = index.get(name.casefold())
    if not source: return False
    destination.parent.mkdir(parents=True, exist_ok=True); copy_file(source, destination); return True


def svg(path: Path, main: bool) -> None:
    color = "#f1c75b" if main else "#9bc7ab"
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32"><path fill="{color}" stroke="#17241f" stroke-width="2" d="M16 2 29 16 16 30 3 16z"/><circle cx="16" cy="16" r="4" fill="#17241f"/></svg>\n', encoding="utf-8")


def convert_map(source: Path, destination: Path) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    command = (
        "Add-Type -AssemblyName System.Drawing; "
        "$source=[System.Drawing.Image]::FromFile($env:ZANIK_MAP_SOURCE); "
        f"$output=[System.Drawing.Bitmap]::new({MAP_SIZE},{MAP_SIZE},[System.Drawing.Imaging.PixelFormat]::Format24bppRgb); "
        "$graphics=[System.Drawing.Graphics]::FromImage($output); "
        "try{"
        f"$graphics.Clear([System.Drawing.Color]::FromArgb({MAP_OCEAN_RGB[0]},{MAP_OCEAN_RGB[1]},{MAP_OCEAN_RGB[2]})); "
        "$graphics.CompositingMode=[System.Drawing.Drawing2D.CompositingMode]::SourceOver; "
        "$graphics.CompositingQuality=[System.Drawing.Drawing2D.CompositingQuality]::HighQuality; "
        "$graphics.InterpolationMode=[System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic; "
        "$graphics.PixelOffsetMode=[System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality; "
        f"$graphics.DrawImage($source,[System.Drawing.Rectangle]::new(0,0,{MAP_SIZE},{MAP_SIZE}),0,0,$source.Width,$source.Height,[System.Drawing.GraphicsUnit]::Pixel); "
        "$output.Save($env:ZANIK_MAP_DESTINATION,[System.Drawing.Imaging.ImageFormat]::Jpeg)"
        "}finally{$graphics.Dispose();$output.Dispose();$source.Dispose()}"
    )
    environment = os.environ.copy(); environment["ZANIK_MAP_SOURCE"] = str(extended_path(source)); environment["ZANIK_MAP_DESTINATION"] = str(destination.resolve())
    completed = subprocess.run(["powershell.exe", "-NoProfile", "-NonInteractive", "-Command", command],
                               check=False, capture_output=True, text=True, env=environment)
    if completed.returncode:
        raise RuntimeError("could not convert the extracted world map to JPEG: " + completed.stderr.strip())
    if not destination.is_file() or destination.stat().st_size < 1024:
        raise RuntimeError("world map conversion returned without producing a valid JPEG")


def extract_font(font_root: Path, destination: Path) -> None:
    matches = list(font_root.rglob("Jancient.ufont"))
    if len(matches) != 1:
        raise RuntimeError(f"expected one extracted Jancient.ufont, found {len(matches)}")
    encoded = read_bytes(matches[0])
    if len(encoded) < 12:
        raise RuntimeError("the extracted Jancient font is truncated")
    size = int.from_bytes(encoded[:4], "little")
    payload = encoded[4:4 + size]
    if len(payload) != size or payload[:4] not in {b"\x00\x01\x00\x00", b"OTTO", b"true", b"ttcf"}:
        raise RuntimeError("the extracted Jancient font has an invalid Unreal bulk-font payload")
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_bytes(payload)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--json-root", type=Path, required=True)
    parser.add_argument("--texture-root", type=Path, required=True)
    parser.add_argument("--font-root", type=Path, required=True)
    parser.add_argument("--web-root", type=Path, required=True)
    args = parser.parse_args(); web = args.web_root; images = index_pngs(args.texture_root)
    missing: list[str] = []; copied = 0
    item_dir = web / "items"
    for record in records(args.json_root):
        p = record.get("Properties") or {}; key = str(record.get("Name", "")); icon = asset_base(p.get("Icon") or p.get("DisplayIcon"))
        if key and icon and (record.get("Type") in {"ItemData", "EquippableItemData", "ConsumableItemData"} or key.startswith(("ITEM_", "DA_Consumable_", "SM_"))):
            copied += int(copy(images, icon, item_dir / f"{key}.png"))
    npc_path = web / "npc-catalog.json"
    if npc_path.is_file():
        catalog = json.loads(npc_path.read_text(encoding="utf-8"))
        for item in catalog.get("types", []):
            icon = str(item.get("icon", "")).rsplit("/", 1)[-1].removesuffix(".png")
            if icon and not copy(images, icon, web / "npcs" / f"{icon}.png"): missing.append(f"npc:{icon}")
    resource_path = web / "resource-catalog.json"; identities = web / "resource-identities.tsv"
    resource_icons: dict[str, str] = {}
    if identities.is_file():
        for line in identities.read_text(encoding="utf-8").splitlines():
            fields = line.split("\t")
            if line and not line.startswith("#") and len(fields) >= 5 and fields[4]: resource_icons.setdefault(fields[2], fields[4])
    if resource_path.is_file():
        catalog = json.loads(resource_path.read_text(encoding="utf-8"))
        for item in catalog.get("types", []):
            key = str(item.get("id", "")); icon = resource_icons.get(key, "")
            if not icon or not copy(images, icon, web / "resources" / f"{key}.png"):
                placeholder(web / "resources" / f"{key}.png", (91, 157, 91, 255)); missing.append(f"resource:{key}")
    for skill in SKILLS:
        # Player cards use the clean skill glyphs from the tag artwork.  The
        # SkillTree textures are glowing progression-node badges and are not
        # interchangeable even though their names look more specific.
        candidates = [f"T_Icon_Tag_Skill_{skill}", f"T_Icon_{skill}", f"T_Icon_SkillTree_{skill}"]
        if not next((copy(images, candidate, web / "skills" / f"{skill}.png") for candidate in candidates), False):
            placeholder(web / "skills" / f"{skill}.png", (193, 166, 77, 255)); missing.append(f"skill:{skill}")
    for target, candidates, color in (
        ("lodestone-icon.png", ("T_Icon_Lodestone",), (67, 155, 219, 255)),
        ("nodestone-icon.png", ("T_Icon_Nodestone", "T_Icon_NodeStone"), (126, 196, 223, 255)),
        ("gravestone-icon.png", ("T_Icon_Gravestone",), (180, 180, 180, 255)),
    ):
        if not next((copy(images, candidate, web / target) for candidate in candidates), False):
            placeholder(web / target, color); missing.append(target)
    map_source = images.get("t_worldmap_landmass")
    if not map_source: raise RuntimeError("the required T_WorldMap_Landmass texture was not exported")
    convert_map(map_source, web / "world-map.jpg")
    extract_font(args.font_root, web / "Jancient-Regular.ttf")
    svg(web / "quests" / "main-quest.svg", True); svg(web / "quests" / "side-quest.svg", False)
    print(json.dumps({"itemIcons": copied, "fallbacks": sorted(missing), "textureFiles": len(images)}, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
