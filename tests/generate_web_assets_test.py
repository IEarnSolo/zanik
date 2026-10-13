#!/usr/bin/env python3
"""Verify player skill artwork selection without requiring game assets."""

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

print("web asset selection tests passed")
