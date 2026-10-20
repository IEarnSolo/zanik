#!/usr/bin/env python3
import importlib.util, json, tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("windows_mapping", ROOT / "tools/windows_mapping.py")
mapping = importlib.util.module_from_spec(spec); spec.loader.exec_module(mapping)


def simulated_launch(url_starts=False, direct_starts=False, timeout=120):
    now = [0.0]; launched = [False]; events = []; recorded = []
    def lookup(_executable): return [4242] if launched[0] else []
    def sleep(seconds): now[0] += seconds
    def url(_steam, _app_id):
        events.append("url")
        if url_starts: launched[0] = True
    def direct(_steam, _app_id):
        events.append("direct")
        if direct_starts: launched[0] = True
    result, attempts = mapping.wait_for_game_launch(
        Path("game.exe"), Path("steam.exe"), "1374490", timeout,
        process_lookup=lookup, clock=lambda: now[0], sleeper=sleep,
        url_launcher=url, direct_launcher=direct, on_attempt=recorded.append)
    return result, attempts, events, recorded


result, attempts, events, recorded = simulated_launch(url_starts=True)
assert result == [4242] and events == ["url"]
assert attempts == recorded and attempts[0]["accepted"]

result, attempts, events, recorded = simulated_launch(direct_starts=True)
assert result == [4242] and events == ["url", "direct"]
assert [item["method"] for item in attempts] == ["steam-url", "steam-executable"]
assert attempts == recorded

result, attempts, events, _recorded = simulated_launch(timeout=95)
assert result == [] and events == ["url", "direct", "url"]
assert [item["method"] for item in attempts] == ["steam-url", "steam-executable", "steam-url-retry"]

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

with tempfile.TemporaryDirectory() as name:
    root = Path(name); binary = root / "game/RSDragonwilds/Binaries/Win64"; binary.mkdir(parents=True)
    executable = binary / "RSDragonwilds-Win64-Shipping.exe"; executable.write_bytes(b"game")
    proxy = binary / "dwmapi.dll"; proxy.write_bytes(b"user proxy")
    owned = binary / "ue4ss"; (owned / "Mods/RSDWTools").mkdir(parents=True)
    (owned / "UE4SS.dll").write_bytes(b"user ue4ss")
    (owned / "Mods/RSDWTools/main.lua").write_text("user mod")
    original_proxy = mapping.checked_snapshot(proxy); original_tree = mapping.checked_snapshot(owned)
    recovery = root / "recovery"; backup = recovery / "backup"; recovery.mkdir()
    preserved = mapping._copy_preserved_loader(binary, backup)
    assert preserved == {"proxy": original_proxy, "ue4ss": original_tree}
    mapping._remove_preserved_loader(binary, preserved)
    proxy.write_bytes(b"zanik proxy")
    owned.mkdir(); (owned / ".zanik-mapping-owner.json").write_text("{}")
    installed = mapping.snapshot_tree(owned)
    journal = recovery / "mapping-recovery.json"
    journal.write_text(json.dumps({
        "schemaVersion": 3, "state": "installed", "binaryDirectory": str(binary),
        "gameExecutable": str(executable), "gameExecutableSha256": mapping.sha256(executable),
        "proxySha256": mapping.sha256(proxy), "ownedFilesInstalled": installed,
        "preservedInventory": preserved, "preservedBackupVerified": True,
        "backupDirectory": str(backup),
    }))
    assert mapping.recover(journal)
    assert mapping.checked_snapshot(proxy) == original_proxy
    assert mapping.checked_snapshot(owned) == original_tree
    assert not backup.exists()
    assert json.loads(journal.read_text())["state"] == "restored"

    # A late client launch may leave the journal at running even though the
    # verified user loader was restored and its backup was already removed.
    # Runtime logs may change, but every operational loader file must match.
    (owned / "UE4SS.log").write_text("new runtime log")
    stale = json.loads(journal.read_text())
    stale.update({"state": "running", "preservedBackupVerified": True,
                  "backupDirectory": str(backup), "preservedInventory": preserved})
    journal.write_text(json.dumps(stale))
    assert mapping.recover(journal)
    recovered = json.loads(journal.read_text())
    assert recovered["state"] == "restored" and recovered["recoveredFromStaleState"]
    assert mapping.checked_snapshot(proxy) == original_proxy
    assert (owned / "Mods/RSDWTools/main.lua").read_text() == "user mod"

    stale["state"] = "running"
    journal.write_text(json.dumps(stale))
    (owned / "unexpected.dll").write_bytes(b"changed user loader")
    try:
        mapping.recover(journal)
        raise AssertionError("stale recovery accepted a changed operational loader")
    except RuntimeError as error:
        assert "ownership marker" in str(error)
    (owned / "unexpected.dll").unlink()

with tempfile.TemporaryDirectory() as name:
    root = Path(name); binary = root / "game/RSDragonwilds/Binaries/Win64"; binary.mkdir(parents=True)
    executable = binary / "RSDragonwilds-Win64-Shipping.exe"; executable.write_bytes(b"game")
    proxy = binary / "dwmapi.dll"; proxy.write_bytes(b"user proxy")
    recovery = root / "recovery"; backup = recovery / "backup"; backup.mkdir(parents=True)
    (backup / "partial").write_text("partial")
    journal = recovery / "mapping-recovery.json"
    journal.write_text(json.dumps({
        "schemaVersion": 3, "state": "preserving-existing-loader", "binaryDirectory": str(binary),
        "gameExecutable": str(executable), "gameExecutableSha256": mapping.sha256(executable),
        "proxySha256": "0" * 64, "ownedFilesInstalled": {},
        "preservedInventory": {"proxy": mapping.checked_snapshot(proxy), "ue4ss": {}},
        "preservedBackupVerified": False, "backupDirectory": str(backup),
    }))
    assert mapping.recover(journal)
    assert proxy.read_bytes() == b"user proxy" and not backup.exists()

with tempfile.TemporaryDirectory() as name:
    root = Path(name); loader = root / "dwmapi.dll"; loader.write_bytes(b"loader")
    backup = root / "backup"
    inventory = mapping._copy_preserved_loader(root, backup)
    loader.write_bytes(b"changed")
    try:
        mapping._remove_preserved_loader(root, inventory)
        raise AssertionError("changed user loader was removed")
    except RuntimeError as error:
        assert "changed" in str(error)
    assert loader.read_bytes() == b"changed"
print("windows mapping tests passed")
