#!/usr/bin/env python3
import importlib.util
import json
import re
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("windows_asset_pipeline", ROOT / "tools/windows_asset_pipeline.py")
pipeline = importlib.util.module_from_spec(spec); spec.loader.exec_module(pipeline)

recipe = json.loads((ROOT / "asset-tools/extraction-recipes.json").read_text(encoding="utf-8"))
pattern = re.compile(recipe["jsonFilter"], re.I)
assert pattern.search("/Game/Maps/World/L_World.umap")
assert pattern.search("/Game/Maps/World/L_World/_Generated_/ABC.umap")
assert pattern.search("/Game/Gameplay/Items/ITEM_Log.uasset")
assert pattern.search("/Game/Gameplay/NPCs/BP_NPC_GhostCathan_Quest.uasset")
assert pattern.search("/GameFeatures/DowdunReach/Content/World/Spawners/AI/SpawnPoints/BP_SpawnPoint_MagicalRaven.uasset")
assert not pattern.search("/Game/Art/Audio/Music/Unrelated.uasset")
assert not pattern.search("/Game/Art/Characters/Models/Unrelated.uasset")

with tempfile.TemporaryDirectory() as name:
    root = Path(name)
    (root / "item.json").write_text(json.dumps([
        {"Properties": {"Icon": {"AssetPathName": "/Game/UI/T_Icon_Log.T_Icon_Log"}}},
        {"Properties": {"Image": {"ObjectPath": "/Game/UI/T_Icon_Wolf.T_Icon_Wolf"}}},
    ]), encoding="utf-8")
    names = pipeline.referenced_textures(root, ["T_WorldMap_Landmass"])
    assert names == ["T_Icon_Log", "T_Icon_Wolf", "T_WorldMap_Landmass"]
    chunks = pipeline.filter_chunks(names, maximum=55)
    assert len(chunks) > 1
    assert all(any(re.search(chunk, f"/x/{item}.uasset", re.I) for chunk in chunks) for item in names)
    transient = root / "transient"; transient.mkdir(); (transient / "gone.txt").write_text("gone")
    original = pipeline.shutil.rmtree
    def raced(path, onerror=None):
        (Path(path) / "gone.txt").unlink()
        if onerror: onerror(None, str(Path(path) / "gone.txt"), (FileNotFoundError, FileNotFoundError(), None))
        Path(path).rmdir()
    pipeline.shutil.rmtree = raced
    try: pipeline.remove_tree(transient)
    finally: pipeline.shutil.rmtree = original
    assert not transient.exists()

print("windows asset pipeline tests passed")
