#!/usr/bin/env python3
from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from tools.release_archive_policy import prohibited_archive_path


for path in (
    "AGENTS.md", "agents.MD", "docs/.agents/local.md", ".codex/settings.json",
    "payload/admins.txt", "assets/raw.usmap", "save/world.SAV", "../escape",
):
    assert prohibited_archive_path(path), path

for path in (
    "README.md", "asset-tools/artwork/nodestone-icon-map.png",
    "payload/server.zip", "docs/installation.md",
):
    assert not prohibited_archive_path(path), path

print("release archive policy tests passed")
