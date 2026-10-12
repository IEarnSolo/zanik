#!/usr/bin/env python3
import importlib.util, json, tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("windows_mapping", ROOT / "tools/windows_mapping.py")
mapping = importlib.util.module_from_spec(spec); spec.loader.exec_module(mapping)

with tempfile.TemporaryDirectory() as name:
    root = Path(name); stage = root / "stage"; mods = stage / "ue4ss/Mods"
    (mods / "OldMod/Scripts").mkdir(parents=True)
    (mods / "OldMod/Scripts/main.lua").write_text("old")
    (mods / "mods.txt").write_text("OldMod : 1\n")
    (mods / "mods.json").write_text("[]\n")
    (stage / "ue4ss/UE4SS-settings.ini").write_text(
        "GuiConsoleEnabled = 1\nGuiConsoleVisible = 1\n"
        "[EngineVersionOverride]\nMajorVersion = \nMinorVersion = \n"
    )
    mapping.configure_staging(stage)
    configured = json.loads((mods / "mods.json").read_text())
    assert configured == [{"mod_enabled": True, "mod_name": "ZanikMappings"}]
    assert not (mods / "OldMod").exists()
    mapping_script = (mods / "ZanikMappings/Scripts/main.lua").read_text()
    assert "ExecuteWithDelay(15000" in mapping_script
    assert "DumpUSMAP()" in mapping_script
    configured_settings = (stage / "ue4ss/UE4SS-settings.ini").read_text()
    assert "GuiConsoleEnabled = 1" in configured_settings
    assert "GuiConsoleVisible = 1" in configured_settings
    assert "MajorVersion = 5" in configured_settings
    assert "MinorVersion = 6" in configured_settings

    binary = root / "game/RSDragonwilds/Binaries/Win64"; binary.mkdir(parents=True)
    executable = binary / "RSDragonwilds-Win64-Shipping.exe"; executable.write_bytes(b"elf-not-needed")
    proxy = binary / "dwmapi.dll"; proxy.write_bytes(b"owned-proxy")
    owned = binary / "ue4ss"; owned.mkdir(); (owned / ".zanik-mapping-owner.json").write_text("{}")
    journal = root / "mapping-recovery.json"
    installed = mapping.snapshot_tree(owned)
    journal.write_text(json.dumps({"schemaVersion": 2, "state": "installed", "binaryDirectory": str(binary),
                                   "gameExecutable": str(executable), "gameExecutableSha256": mapping.sha256(executable),
                                   "proxySha256": mapping.sha256(proxy), "ownedFilesInstalled": installed}))
    (owned / "UE4SS.log").write_text("runtime log")
    (owned / "imgui.ini").write_text("runtime UI state")
    assert mapping.recover(journal)
    assert not proxy.exists() and not owned.exists()
    assert json.loads(journal.read_text())["state"] == "restored"

    proxy.write_bytes(b"changed")
    owned.mkdir(); (owned / ".zanik-mapping-owner.json").write_text("{}")
    journal.write_text(json.dumps({"schemaVersion": 2, "state": "installed", "binaryDirectory": str(binary),
                                   "gameExecutable": str(executable), "proxySha256": "0" * 64,
                                   "ownedFilesInstalled": mapping.snapshot_tree(owned)}))
    try:
        mapping.recover(journal)
        raise AssertionError("changed proxy was removed")
    except RuntimeError as error:
        assert "changed" in str(error)
    assert proxy.exists() and owned.exists()

    proxy.unlink(); (owned / "unexpected.dll").write_bytes(b"not-zanik")
    journal.write_text(json.dumps({"schemaVersion": 2, "state": "installed", "binaryDirectory": str(binary),
                                   "gameExecutable": str(executable), "proxySha256": "",
                                   "ownedFilesInstalled": {".zanik-mapping-owner.json": mapping.sha256(owned / ".zanik-mapping-owner.json")}}))
    try:
        mapping.recover(journal)
        raise AssertionError("unknown UE4SS file was removed")
    except RuntimeError as error:
        assert "unknown or changed" in str(error)
    assert (owned / "unexpected.dll").exists()
print("windows mapping tests passed")
