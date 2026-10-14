#!/usr/bin/env python3
"""Verify player skill artwork selection without requiring game assets."""

import json
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import generate_web_assets as generator  # noqa: E402


with tempfile.TemporaryDirectory(prefix="zanik-web-assets-") as temporary:
    root = Path(temporary)
    json_root = root / "json"
    textures = root / "textures"
    fonts = root / "fonts"
    web = root / "web"
    for directory in (json_root, textures, fonts):
        directory.mkdir(parents=True)

    for skill in generator.SKILLS:
        (textures / f"T_Icon_SkillTree_{skill}.png").write_bytes(b"skill-tree:" + skill.encode())
        (textures / f"T_Icon_Tag_Skill_{skill}.png").write_bytes(b"tag-skill:" + skill.encode())
    (textures / "T_WorldMap_Landmass.png").write_bytes(b"map")
    (textures / "T_Arrows_Bone.png").write_bytes(b"fang-arrow")
    (textures / "T_Icon_Cactus_Steak_Raw.png").write_bytes(b"cactus")
    (json_root / "ammo.json").write_text(json.dumps([
        {
            "Name": "ITEM_Ammo_Arrows_Bone_Bodkin", "Type": "RangedAmmoData",
            "Properties": {"Icon": {"AssetPathName": "/Game/UI/T_Arrows_Bone.T_Arrows_Bone"}},
        },
        {
            "Name": "ITEM_Ammo_Arrows_Bone_Poison", "Type": "RangedAmmoData",
            "Properties": {"Icon": {"AssetPathName": "/Game/UI/T_Arrows_Bone.T_Arrows_Bone"}},
        },
    ]), encoding="utf-8")
    (web / "resource-catalog.json").parent.mkdir(parents=True, exist_ok=True)
    (web / "resource-catalog.json").write_text(json.dumps({
        "types": [{"id": "barrel-cactus", "name": "Barrel Cactus"}],
    }), encoding="utf-8")
    (web / "resource-identities.tsv").write_text(
        "# stateKey\topaqueId\ttypeId\trespawnProfile\ticonAsset\n"
        "state\tid\tbarrel-cactus\tprofile\tT_Icon_Cactus_Steak_Raw\n",
        encoding="utf-8",
    )

    generator.convert_map = lambda _source, destination: destination.write_bytes(b"jpeg")
    generator.extract_font = lambda _root, destination: destination.write_bytes(b"font")
    sys.argv = [
        "generate_web_assets.py",
        "--json-root", str(json_root),
        "--texture-root", str(textures),
        "--font-root", str(fonts),
        "--web-root", str(web),
    ]
    assert generator.main() == 0
    for skill in generator.SKILLS:
        assert (web / "skills" / f"{skill}.png").read_bytes() == b"tag-skill:" + skill.encode()
    assert (web / "items" / "ITEM_Ammo_Arrows_Bone_Bodkin.png").read_bytes() == b"fang-arrow"
    assert (web / "items" / "ITEM_Ammo_Arrows_Bone_Poison.png").read_bytes() == b"fang-arrow"
    assert (web / "resources" / "barrel-cactus.png").read_bytes() == b"cactus"

print("web asset selection tests passed")
