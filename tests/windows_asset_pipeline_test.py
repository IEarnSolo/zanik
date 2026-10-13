#!/usr/bin/env python3
import importlib.util
import json
import re
import tempfile
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
spec = importlib.util.spec_from_file_location("windows_asset_pipeline", ROOT / "tools/windows_asset_pipeline.py")
pipeline = importlib.util.module_from_spec(spec); spec.loader.exec_module(pipeline)
building_spec = importlib.util.spec_from_file_location("generate_building_catalog", ROOT / "tools/generate_building_catalog.py")
buildings = importlib.util.module_from_spec(building_spec); building_spec.loader.exec_module(buildings)

recipe = json.loads((ROOT / "asset-tools/extraction-recipes.json").read_text(encoding="utf-8"))
pattern = re.compile(recipe["jsonFilter"], re.I)
assert pattern.search("/Game/Maps/World/L_World.umap")
assert pattern.search("/Game/Maps/World/L_World/_Generated_/ABC.umap")
assert pattern.search("/Game/Gameplay/Items/ITEM_Log.uasset")
assert pattern.search("/Game/Gameplay/Progress/DT_Progression_Core.uasset")
assert pattern.search("/Game/Gameplay/Progress/DT_Progression_BuildPieces.uasset")
assert pattern.search("/Game/Gameplay/NPCs/BP_NPC_GhostCathan_Quest.uasset")
assert pattern.search("/GameFeatures/DowdunReach/Content/World/Spawners/AI/SpawnPoints/BP_SpawnPoint_MagicalRaven.uasset")
assert not pattern.search("/Game/Art/Audio/Music/Unrelated.uasset")
assert not pattern.search("/Game/Art/Characters/Models/Unrelated.uasset")
assert buildings.EXPECTED_ICON_SOURCES == {
    "authored-texture": 753,
    "embedded-build-ui-thumbnail": 68,
    "procedural-thumbnail": 1,
}
assert buildings.icon_source("/Game/Art/UI/Icons/GeneratedIcons/SM_Flag.SM_Flag") == "embedded-build-ui-thumbnail"
assert buildings.icon_source("/Game/Art/UI/Icons/ProceduralThumbnails/T_Tree.T_Tree") == "procedural-thumbnail"
assert buildings.cooked_package_path("/Game/Art/UI/Icons/GeneratedIcons/SM_Flag.SM_Flag") == (
    "RSDragonwilds/Content/Art/UI/Icons/GeneratedIcons/SM_Flag"
)

with tempfile.TemporaryDirectory() as name:
    root = Path(name)
    exact = root / "exact.png"; first = root / "first.png"; second = root / "second.png"
    exact.write_bytes(b"exact"); first.write_bytes(b"first"); second.write_bytes(b"second")
    icon_asset = "/Game/Art/UI/Icons/GeneratedIcons/SM_Flag.SM_Flag"
    assert buildings.find_icon(
        {"rsdragonwilds/content/art/ui/icons/generatedicons/sm_flag": exact},
        {"sm_flag.png": [first, second]}, icon_asset,
    ) == exact
    try:
        buildings.find_icon({}, {"sm_flag.png": [first, second]}, icon_asset)
        raise AssertionError("ambiguous duplicate icon basenames were accepted")
    except RuntimeError as error:
        assert "ambiguous icon basename" in str(error)
    (root / "item.json").write_text(json.dumps([
        {"Properties": {"Icon": {"AssetPathName": "/Game/UI/T_Icon_Log.T_Icon_Log"}}},
        {"Properties": {"Image": {"ObjectPath": "/Game/UI/T_Icon_Wolf.T_Icon_Wolf"}}},
        {"Properties": {"DisplayIcon": {"AssetPathName": "/Game/Art/UI/Icons/GeneratedIcons/SM_Flag.SM_Flag"}}},
        {"Properties": {"DisplayIcon": {"AssetPathName": "/Game/Art/UI/Icons/ProceduralThumbnails/T_Tree.T_Tree"}}},
    ]), encoding="utf-8")
    names = pipeline.referenced_textures(root, ["T_WorldMap_Landmass"])
    assert names == [
        "basename:T_WorldMap_Landmass",
        "package:RSDragonwilds/Content/Art/UI/Icons/GeneratedIcons/SM_Flag",
        "package:RSDragonwilds/Content/Art/UI/Icons/ProceduralThumbnails/T_Tree",
        "package:RSDragonwilds/Content/UI/T_Icon_Log",
        "package:RSDragonwilds/Content/UI/T_Icon_Wolf",
    ]
    duplicate_required = pipeline.referenced_textures(root, ["SM_Flag"])
    assert "basename:SM_Flag" not in duplicate_required
    chunks = pipeline.filter_chunks(names, maximum=55)
    assert len(chunks) > 1
    assert any(re.search(chunk, "RSDragonwilds/Content/Art/UI/Icons/GeneratedIcons/SM_Flag.uasset", re.I) for chunk in chunks)
    assert any(re.search(chunk, "/x/T_WorldMap_Landmass.uasset", re.I) for chunk in chunks)
    assert pipeline.cooked_package_path("/UmbralSands/UI/T_Icon.T_Icon") == (
        "RSDragonwilds/Plugins/GameFeatures/UmbralSands/Content/UI/T_Icon"
    )
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
    retry = root / "retry"; retry.mkdir(); attempts = []
    def busy(path, onerror=None):
        attempts.append(path)
        if len(attempts) == 1: raise OSError("directory not empty")
        original(path, onerror=onerror)
    pipeline.shutil.rmtree = busy
    try: pipeline.remove_tree(retry)
    finally: pipeline.shutil.rmtree = original
    assert len(attempts) == 2 and not retry.exists()
    incomplete = root / "incomplete"; incomplete.mkdir()
    (incomplete / "DA_BuildPieceCatalogue_Default.json").write_text(json.dumps([{
        "Properties": {"Pages": [{"Collection": [{"Collection": [
            {"AssetPathName": "/Game/DA_RequiredPiece.DA_RequiredPiece"}
        ]}]}]}
    }]))
    try:
        pipeline.validate_catalog_export(incomplete)
        raise AssertionError("incomplete targeted export was accepted")
    except RuntimeError as error:
        assert "omitted 1 building records" in str(error)
    try:
        pipeline.validate_reviewed_counts({"jsonFiles": 10, "worldPartitionCells": 5},
                                          {"jsonFiles": 11, "worldPartitionCells": 5})
        raise AssertionError("reviewed export-count drift was accepted")
    except RuntimeError as error:
        assert "jsonFiles expected 11, got 10" in str(error)

print("windows asset pipeline tests passed")
