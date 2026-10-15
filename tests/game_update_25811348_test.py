#!/usr/bin/env python3
from pathlib import Path
import json


root = Path(__file__).resolve().parents[1]
server_sha = "49512c0ee933c3ee4dda5fb5c2f0a25e66d7f26e34795c45b91b5c1d12b61b0c"

rows = [
    line.split("\t")
    for line in (root / "deploy" / "compatibility.tsv").read_text().splitlines()
    if line and not line.startswith("#")
]
candidate = [row for row in rows if row[0] == server_sha]
assert len(candidate) == 1
assert len(candidate[0]) == 49
assert candidate[0][10:14] == ["-11075", "-117685", "420000", "world-map-1.0.0.2"]

header = (root / "src" / "engine" / "dom_engine.h").read_text()
assert "GObjects_25811348 = 0x0DC96510" in header
assert "GNames_25811348   = 0x0DBDAFF8" in header
assert "GWorld_25811348   = 0x0DDB25F8" in header

dispatcher = (root / "src" / "engine" / "admin_storage.cpp").read_text()
assert f'ExpectedSha(hash)||hash=="{server_sha}"' in dispatcher
assert f'ExpectedSha(hash){{return hash=="{server_sha}"' not in dispatcher

resources = (root / "src" / "engine" / "resources.cpp").read_text()
assert f'hash=="{server_sha}"' in resources

profile = json.loads((root / "asset-tools" / "extraction-recipes.json").read_text())["catalogProfile"]
assert profile["version"] == "client-25812671-reviewed-4"
assert profile["clientBuild"] == "25812671"
assert profile["catalogCounts"]["buildings/catalog.json:pieces"] == 827
assert profile["catalogCounts"]["npc-catalog.json:nodes"] == 2964
assert profile["catalogCounts"]["recipe-unlocks.json:entries"] == 1576

print("game update 25811348 compatibility checks passed")
