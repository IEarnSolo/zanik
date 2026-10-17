#!/usr/bin/env python3
from pathlib import Path


root = Path(__file__).resolve().parents[1]
server_sha = "b5017cf67bc6127dbec443e00fed1f818e82afd5d6d17f912d959a1dd87d5687"

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
assert "GObjects_25991736 = 0x0DC97510" in header
assert "GNames_25991736   = 0x0DBDBFF8" in header
assert "GWorld_25991736   = 0x0DDB35F8" in header

dispatcher = (root / "src" / "engine" / "admin_storage.cpp").read_text()
assert server_sha in dispatcher
assert f'ExpectedSha(hash){{return hash=="{server_sha}"' not in dispatcher
assert "currentDispatcher?0x07ABFE00" in dispatcher

resources = (root / "src" / "engine" / "resources.cpp").read_text()
assert f'hash=="{server_sha}"' in resources
assert "getRespawnIntervalOffset=0x0AACD130" in resources

storage = (root / "src" / "engine" / "uobject.cpp").read_text()
assert f'latestSha="{server_sha}"' in storage
assert "0x0A65A8B0" in storage
assert "0x0A65D330" in storage
assert "displaySignature" in storage
assert "emptySignature" in storage
assert "filledSignature" in storage

print("game update 25991736 compatibility checks passed")
