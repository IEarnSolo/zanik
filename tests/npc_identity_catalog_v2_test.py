#!/usr/bin/env python3
import argparse
import importlib.util
import json
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
spec = importlib.util.spec_from_file_location(
    "generate_world_catalogs", ROOT / "tools/generate_world_catalogs.py")
world = importlib.util.module_from_spec(spec)
spec.loader.exec_module(world)


def write(path: Path, value: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value), encoding="utf-8")


with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    characters = root / "characters"
    spawns = root / "spawns"
    cells = root / "cells"
    placed = root / "placed"
    placed.mkdir()

    write(characters / "BP_AI_Test.json", [
        {"Type": "BlueprintGeneratedClass", "Name": "BP_AI_Test_C"},
        {"Name": "Default__BP_AI_Test_C", "Properties": {
            "AIName": {"SourceString": "Test Beast"},
        }},
    ])
    write(spawns / "BP_SpawnPoint_Test.json", [
        {"Type": "BlueprintGeneratedClass", "Name": "BP_SpawnPoint_Test_C",
         "SuperStruct": {"ObjectName": "AISpawnPoint"}},
        {"Name": "Default__BP_SpawnPoint_Test_C", "Properties": {
            "AIClass": {"AssetPathName": "/Game/Test/BP_AI_Test.BP_AI_Test"},
            "RespawnDuration": {"Seconds": 120},
        }},
    ])
    native_guid = "12345678-90AB-CDEF-1234-567890ABCDEF"
    write(cells / "Maps/World/L_World/_Generated_/Cell.json", [
        {"Type": "BP_SpawnPoint_Test_C", "Name": "SpawnPoint_Test_1", "Properties": {
            "Guid": native_guid,
            "RootComponent": {"ObjectPath": "/Game/Maps/World/L_World.Cell.1"},
        }},
        {"Type": "SceneComponent", "Properties": {
            "RelativeLocation": {"X": 0, "Y": 0, "Z": 50},
        }},
    ])

    public = root / "npc-catalog.json"
    identities = root / "npc-identities.tsv"
    exclusions = root / "npc-exclusions.tsv"
    result = world.npc_catalog(argparse.Namespace(
        character_root=characters, spawn_root=spawns, cells_root=cells,
        placed_root=placed, public_output=public,
        identities_output=identities, exclusions_output=exclusions,
    ))
    assert result == 0
    lines = identities.read_text(encoding="utf-8").splitlines()
    header = lines[0].split("\t")
    row = lines[1].split("\t")
    assert header[0:2] == ["# zanik-npc-identities", "2"]
    assert header[13:15] == ["nativeGuid", "sourceFile"]
    assert len(row) == 11
    assert row[9] == native_guid
    assert row[10] == "Maps/World/L_World/_Generated_/Cell.json"
    assert json.loads(public.read_text(encoding="utf-8"))["nodes"][0]["respawnGameSeconds"] == 120
