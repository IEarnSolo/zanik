#!/usr/bin/env python3
import importlib.util
import json
import re
import tempfile
import sys
from pathlib import Path
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
spec = importlib.util.spec_from_file_location("windows_asset_pipeline", ROOT / "tools/windows_asset_pipeline.py")
pipeline = importlib.util.module_from_spec(spec); spec.loader.exec_module(pipeline)
filesystem_spec = importlib.util.spec_from_file_location("asset_filesystem", ROOT / "tools/asset_filesystem.py")
asset_filesystem = importlib.util.module_from_spec(filesystem_spec); filesystem_spec.loader.exec_module(asset_filesystem)
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
assert str(asset_filesystem.extended_path(
    r"C:\Users\Bill\AppData\Local\Zanik Manager\asset-work\hash\pipeline\json", "nt"
)).startswith("\\\\?\\C:\\")

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
    with patch.object(pipeline, "read_json_file", side_effect=OSError("path too long")):
        try:
            pipeline.referenced_textures(root, [])
            raise AssertionError("an unreadable exported JSON file was silently skipped")
        except RuntimeError as error:
            assert "cannot read exported JSON" in str(error)
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

    journal = root / "extraction-journal.json"
    original_journal = {
        "schemaVersion": 1, "stage": "mappings", "updatedAt": "old",
        "fingerprints": {"gameExecutableSha256": "game"},
        "futureField": {"preserved": True},
    }
    journal.write_text(json.dumps(original_journal), encoding="utf-8-sig")
    pipeline.update_stage(journal, "exports")
    assert not journal.read_bytes().startswith(b"\xef\xbb\xbf")
    updated = json.loads(journal.read_text(encoding="utf-8"))
    assert updated["stage"] == "exports" and updated["futureField"] == {"preserved": True}

    workspace = root / "resume"; json_root = workspace / "json"; textures = workspace / "textures"
    font = workspace / "font"; json_root.mkdir(parents=True); textures.mkdir(); font.mkdir()
    (json_root / ".uas-manifest.json").write_text(json.dumps({"schema": 2, "mode": "json"}))
    batch = textures / "batch-01"; batch.mkdir()
    (batch / ".uas-manifest.json").write_text(json.dumps({"schema": 2, "mode": "textures"}))
    (batch / "icon.png").write_bytes(b"png")
    (font / ".uas-manifest.json").write_text(json.dumps({"schema": 2, "mode": "raw"}))
    (font / "Jancient.ufont").write_bytes(b"font")
    mapping = root / "Mappings.usmap"; mapping.write_bytes(b"mapping")
    recipe_path = root / "recipe.json"; recipe_path.write_text("{}")
    checkpoint_recipe = {"requiredTextureAssets": ["required"], "_path": str(recipe_path)}
    journal.write_text(json.dumps(original_journal), encoding="utf-8-sig")
    with patch.object(pipeline, "validate_catalog_export"), \
         patch.object(pipeline, "referenced_textures", return_value=["basename:required"]), \
         patch.object(pipeline, "filter_chunks", return_value=["one"]):
        checkpoint = pipeline.recover_or_create_export_checkpoint(
            workspace, checkpoint_recipe, {"jsonFiles": 1}, mapping, journal)
        assert checkpoint["textureBatches"] == [{
            "name": "batch-01",
            "manifestSha256": pipeline.sha256(batch / ".uas-manifest.json"),
            "pngFiles": 1,
        }]
        assert checkpoint["fingerprints"] == original_journal["fingerprints"]
        pipeline.recover_or_create_export_checkpoint(
            workspace, checkpoint_recipe, {"jsonFiles": 1}, mapping, journal)
        json_manifest = json_root / ".uas-manifest.json"
        original_json_manifest = json_manifest.read_text()
        json_manifest.write_text("not json")
        try:
            pipeline.verified_export_checkpoint(
                workspace, checkpoint_recipe, {"jsonFiles": 1}, mapping, journal)
            raise AssertionError("corrupt JSON manifest was accepted")
        except RuntimeError as error:
            assert "invalid UnrealAssetScout manifest" in str(error)
        json_manifest.write_text(original_json_manifest)
        exported_font = font / "Jancient.ufont"
        exported_font.unlink()
        try:
            pipeline.verified_export_checkpoint(
                workspace, checkpoint_recipe, {"jsonFiles": 1}, mapping, journal)
            raise AssertionError("missing font was accepted")
        except RuntimeError as error:
            assert "exactly one Jancient.ufont" in str(error)
        exported_font.write_bytes(b"font")
        mapping.unlink()
        try:
            pipeline.verified_export_checkpoint(
                workspace, checkpoint_recipe, {"jsonFiles": 1}, mapping, journal)
            raise AssertionError("missing mapping was accepted")
        except RuntimeError as error:
            assert "missing its generated mapping" in str(error)
        mapping.write_bytes(b"mapping")
        extra = textures / "batch-02"; extra.mkdir()
        try:
            pipeline.verified_export_checkpoint(
                workspace, checkpoint_recipe, {"jsonFiles": 1}, mapping, journal)
            raise AssertionError("unexpected texture batch was accepted")
        except RuntimeError as error:
            assert "extra=['batch-02']" in str(error)
        extra.rmdir()
        moved_batch = textures / "held-batch"
        batch.rename(moved_batch)
        try:
            pipeline.verified_export_checkpoint(
                workspace, checkpoint_recipe, {"jsonFiles": 1}, mapping, journal)
            raise AssertionError("missing texture batch was accepted")
        except RuntimeError as error:
            assert "missing=['batch-01']" in str(error)
        moved_batch.rename(batch)
        (batch / "icon.png").unlink()
        try:
            pipeline.verified_export_checkpoint(
                workspace, checkpoint_recipe, {"jsonFiles": 1}, mapping, journal)
            raise AssertionError("empty texture batch was accepted")
        except RuntimeError as error:
            assert "texture export batch is empty" in str(error)
        (batch / "icon.png").write_bytes(b"png")
        mapping.write_bytes(b"changed mapping")
        try:
            pipeline.recover_or_create_export_checkpoint(
                workspace, checkpoint_recipe, {"jsonFiles": 1}, mapping, journal)
            raise AssertionError("changed mapping was accepted by the recorded checkpoint")
        except RuntimeError as error:
            assert "checkpoint no longer matches" in str(error)
        mapping.write_bytes(b"mapping")
        (batch / ".uas-manifest.json").write_text("not json")
        try:
            pipeline.recover_or_create_export_checkpoint(
                workspace, checkpoint_recipe, {"jsonFiles": 1}, mapping, journal)
            raise AssertionError("corrupt UAS manifest was accepted")
        except RuntimeError as error:
            assert "invalid UnrealAssetScout manifest" in str(error)

    raw = workspace / "json" / "keep.json"; raw.write_text("{}")
    generated = workspace / "web"; generated.mkdir(); (generated / "partial.txt").write_text("partial")
    private = workspace / "internal"; private.mkdir(); (private / "partial.txt").write_text("partial")
    pipeline.reset_partial_generation(workspace)
    assert raw.is_file() and not generated.exists()
    assert private.is_dir() and not any(private.iterdir())

    powershell = (ROOT / "scripts" / "init-assets.ps1").read_text(encoding="utf-8")
    writer = powershell[powershell.index("function Write-JsonAtomic"):powershell.index("function Set-ExtractionStage")]
    assert "UTF8Encoding($false)" in writer and "WriteAllText" in writer
    assert "Set-Content" not in writer
    assert "$Resume -and (Test-Path -LiteralPath $PipelineWorkspace" in powershell
    assert "game launch and UnrealAssetScout extraction are skipped" in powershell

print("windows asset pipeline tests passed")
