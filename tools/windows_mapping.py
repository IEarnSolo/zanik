#!/usr/bin/env python3
"""Generate Unreal mappings locally while leaving the licensed client unchanged."""

from __future__ import annotations

import ctypes
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import time
from typing import Any


GAME_EXE = Path("RSDragonwilds/Binaries/Win64/RSDragonwilds-Win64-Shipping.exe")
PROXY_NAME = "dwmapi.dll"
OWNED_DIRECTORY = "ue4ss"
JOURNAL_SCHEMA = 2
PROXY_STAGING_NAME = ".zanik-mapping-dwmapi-installing"
DIRECTORY_STAGING_NAME = ".zanik-mapping-ue4ss-installing"


def sha256(path: Path) -> str:
    value = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(block)
    return value.hexdigest()


def inside(path: Path, parent: Path) -> bool:
    try:
        path.resolve().relative_to(parent.resolve())
        return True
    except ValueError:
        return False


def read_journal(path: Path) -> dict[str, Any]:
    value = json.loads(path.read_text(encoding="utf-8"))
    if value.get("schemaVersion") != JOURNAL_SCHEMA:
        raise ValueError("unsupported mapping recovery journal")
    return value


def write_journal(path: Path, value: dict[str, Any]) -> None:
    temporary = path.with_name(path.name + ".new")
    temporary.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    temporary.replace(path)


def snapshot_tree(root: Path) -> dict[str, str]:
    result: dict[str, str] = {}
    for path in sorted(root.rglob("*")):
        if path.is_symlink():
            raise RuntimeError(f"temporary UE4SS tree contains a symbolic link: {path}")
        if path.is_file():
            result[path.relative_to(root).as_posix()] = sha256(path)
    return result


def game_processes(executable: Path) -> list[int]:
    if os.name != "nt":
        return []
    command = ["powershell.exe", "-NoProfile", "-NonInteractive", "-Command",
               "$p=Get-Process -Name 'RSDragonwilds-Win64-Shipping' -ErrorAction SilentlyContinue;"
               "$p|ForEach-Object{$path='';try{$path=$_.Path}catch{};\"$($_.Id)`t$path\"}"]
    completed = subprocess.run(command, capture_output=True, text=True, check=False)
    result: list[int] = []
    for line in completed.stdout.splitlines():
        fields = line.split("\t", 1)
        if len(fields) != 2:
            continue
        try:
            process_id = int(fields[0])
        except ValueError:
            continue
        # Get-Process is available to ordinary desktop users.  Path may still be
        # withheld by Windows, but the exact executable name uniquely identifies
        # the client and is safer than treating a permission error as "not running".
        if not fields[1] or Path(fields[1]).resolve() == executable.resolve():
            result.append(process_id)
    return result


def recover(journal_path: Path) -> bool:
    if not journal_path.is_file():
        return False
    journal = read_journal(journal_path)
    binary_dir = Path(journal["binaryDirectory"]).resolve()
    executable = Path(journal["gameExecutable"]).resolve()
    if not inside(executable, binary_dir) or executable.parent != binary_dir:
        raise ValueError("mapping journal paths are outside the recorded game binary directory")
    if game_processes(executable):
        raise RuntimeError("Dragonwilds is still running; close it before mapping cleanup can continue")
    proxy = binary_dir / PROXY_NAME
    owned = binary_dir / OWNED_DIRECTORY
    proxy_staging = binary_dir / PROXY_STAGING_NAME
    directory_staging = binary_dir / DIRECTORY_STAGING_NAME
    installed_files = journal.get("ownedFilesInstalled")
    if not isinstance(installed_files, dict):
        raise ValueError("mapping recovery journal has no installed-file inventory")
    for partial in (proxy_staging, directory_staging):
        if not partial.exists():
            continue
        if not inside(partial, binary_dir):
            raise RuntimeError("temporary mapping installation resolves outside the game directory")
        if partial == proxy_staging:
            if not partial.is_file() or sha256(partial) != journal.get("proxySha256"):
                raise RuntimeError("partial mapping proxy changed; refusing automatic cleanup")
            partial.unlink()
        else:
            if not partial.is_dir() or partial.is_symlink():
                raise RuntimeError("partial UE4SS installation is not a normal directory")
            current = snapshot_tree(partial)
            if any(installed_files.get(path) != value for path, value in current.items()):
                raise RuntimeError("partial UE4SS installation contains an unknown or changed file")
            shutil.rmtree(partial)
    expected_proxy = journal.get("proxySha256", "")
    if proxy.exists():
        if not proxy.is_file() or sha256(proxy) != expected_proxy:
            raise RuntimeError("temporary mapping proxy changed; refusing to remove it automatically")
        proxy.unlink()
    if owned.exists():
        if not owned.is_dir() or owned.is_symlink() or not (owned / ".zanik-mapping-owner.json").is_file():
            raise RuntimeError("temporary UE4SS directory lost its Zanik ownership marker")
        if not inside(owned, binary_dir):
            raise RuntimeError("temporary UE4SS directory resolves outside the game directory")
        cleanup_snapshot = snapshot_tree(owned)
        unexpected = []
        for path, value in cleanup_snapshot.items():
            if installed_files.get(path) == value:
                continue
            lowered = path.casefold()
            runtime_generated = {
                "imgui.ini",
                "liveview/filters.meta.json",
                "watches/watches.meta.json",
            }
            if path not in installed_files and (
                    lowered.endswith(".log") or lowered.endswith(".usmap")
                    or lowered in runtime_generated):
                continue
            unexpected.append(path)
        if unexpected:
            raise RuntimeError("temporary UE4SS tree contains unknown or changed files: " + ", ".join(unexpected[:5]))
        journal["ownedFilesAtCleanup"] = cleanup_snapshot
        journal["state"] = "cleanup-snapshotted"
        write_journal(journal_path, journal)
        if snapshot_tree(owned) != cleanup_snapshot:
            raise RuntimeError("temporary UE4SS files changed during cleanup; refusing to remove them")
        shutil.rmtree(owned)
    generated = journal.get("generatedMapping")
    if isinstance(generated, dict):
        generated_path = Path(generated.get("path", "")).resolve()
        if not inside(generated_path, binary_dir):
            raise RuntimeError("generated mapping path resolves outside the game directory")
        if generated_path.exists():
            if not generated_path.is_file() or sha256(generated_path) != generated.get("sha256"):
                raise RuntimeError("generated mapping changed; refusing automatic cleanup")
            generated_path.unlink()
    expected_executable = journal.get("gameExecutableSha256")
    if expected_executable and sha256(executable) != expected_executable:
        raise RuntimeError("Dragonwilds executable changed during mapping generation")
    journal["state"] = "restored"
    journal["restoredAt"] = int(time.time())
    write_journal(journal_path, journal)
    return True


def configure_staging(staging: Path) -> None:
    settings = staging / OWNED_DIRECTORY / "UE4SS-settings.ini"
    text = settings.read_text(encoding="utf-8", errors="strict")
    replacements = {
        # The Dragonwilds-compatible UE4SS build requires its normal console and
        # hot-reload initialization path.  The console is temporary and closes
        # with the client after the mapping is copied.
        "EnableHotReloadSystem = 0": "EnableHotReloadSystem = 1",
        "GuiConsoleEnabled = 1": "GuiConsoleEnabled = 1",
        "GuiConsoleEnabled = 0": "GuiConsoleEnabled = 1",
        "GuiConsoleVisible = 1": "GuiConsoleVisible = 1",
        "GuiConsoleVisible = 0": "GuiConsoleVisible = 1",
        "EnableDumping = 1": "EnableDumping = 0",
    }
    for old, new in replacements.items():
        text = text.replace(old, new)
    if "[EngineVersionOverride]" not in text:
        raise RuntimeError("the pinned UE4SS settings have no engine-version override section")
    text, major_count = re.subn(r"(?m)^MajorVersion\s*=.*$", "MajorVersion = 5", text)
    text, minor_count = re.subn(r"(?m)^MinorVersion\s*=.*$", "MinorVersion = 6", text)
    if major_count != 1 or minor_count != 1:
        raise RuntimeError("the pinned UE4SS settings have ambiguous engine-version fields")
    settings.write_text(text, encoding="utf-8")
    # UE4SS rewrites this UI state while running.  Exclude the archive's copy so
    # any exact-name replacement is clearly runtime-generated and can be removed
    # without weakening verification for installed binaries or scripts.
    imgui_state = staging / OWNED_DIRECTORY / "imgui.ini"
    if imgui_state.exists():
        imgui_state.unlink()
    mods = staging / OWNED_DIRECTORY / "Mods"
    for child in list(mods.iterdir()):
        if child.name not in {"mods.txt", "mods.json"}:
            if child.is_dir():
                shutil.rmtree(child)
            else:
                child.unlink()
    zanik = mods / "ZanikMappings" / "Scripts"
    zanik.mkdir(parents=True)
    (zanik / "main.lua").write_text(
        "ExecuteWithDelay(15000, function()\n"
        "    DumpUSMAP()\n"
        "end)\n", encoding="utf-8")
    (mods / "mods.txt").write_text("ZanikMappings : 1\n", encoding="utf-8")
    if (mods / "mods.json").exists():
        (mods / "mods.json").write_text(json.dumps([
            {"mod_name": "ZanikMappings", "mod_enabled": True}
        ], indent=2) + "\n", encoding="utf-8")
    (staging / OWNED_DIRECTORY / ".zanik-mapping-owner.json").write_text(
        json.dumps({"schemaVersion": 1, "purpose": "temporary local mapping generation"}, sort_keys=True) + "\n",
        encoding="utf-8")


def close_windows(pid: int) -> None:
    if os.name != "nt":
        return
    user32 = ctypes.windll.user32
    WM_CLOSE = 0x0010
    callback_type = ctypes.WINFUNCTYPE(ctypes.c_bool, ctypes.c_void_p, ctypes.c_void_p)

    def callback(window: int, _: int) -> bool:
        process_id = ctypes.c_ulong()
        user32.GetWindowThreadProcessId(window, ctypes.byref(process_id))
        if process_id.value == pid:
            user32.PostMessageW(window, WM_CLOSE, 0, 0)
        return True

    user32.EnumWindows(callback_type(callback), 0)


def steam_launch(game_root: Path) -> tuple[Path, str]:
    steamapps = next((parent for parent in game_root.parents if parent.name.casefold() == "steamapps"), None)
    if steamapps is None:
        raise ValueError("the selected client is not inside a discoverable Steam library")
    for manifest in steamapps.glob("appmanifest_*.acf"):
        contents = manifest.read_text(encoding="utf-8", errors="ignore")
        install = re.search(r'"installdir"\s+"([^"]+)"', contents, re.I)
        name = re.search(r'"name"\s+"([^"]+)"', contents, re.I)
        if (not install or install.group(1).casefold() != game_root.name.casefold()
                or not name or name.group(1).casefold() != "runescape: dragonwilds"):
            continue
        app_id = re.search(r'"appid"\s+"(\d+)"', contents, re.I)
        if not app_id:
            continue
        candidates: list[Path] = []
        launcher = re.search(r'"LauncherPath"\s+"([^"]+)"', contents, re.I)
        if launcher:
            candidates.append(Path(launcher.group(1).replace("\\\\", "\\")))
        try:
            import winreg
            with winreg.OpenKey(winreg.HKEY_CURRENT_USER, r"Software\Valve\Steam") as key:
                candidates.append(Path(winreg.QueryValueEx(key, "SteamExe")[0]))
        except (ImportError, FileNotFoundError, OSError):
            pass
        program_files = os.environ.get("ProgramFiles(x86)")
        if program_files:
            candidates.append(Path(program_files) / "Steam/steam.exe")
        candidates.append(steamapps.parent / "steam.exe")
        for steam in candidates:
            if steam.is_file():
                return steam.resolve(), app_id.group(1)
    raise ValueError("could not discover the Dragonwilds Steam launcher and app id")


def launch_steam_app(steam: Path, app_id: str) -> None:
    """Ask the running Steam client to launch the app through its URL protocol.

    Starting a second steam.exe process with ``-applaunch`` is not reliably
    forwarded to an already-running client on all Windows configurations.  The
    registered URL protocol is the supported desktop handoff and avoids that
    silent no-op.
    """
    if os.name != "nt":
        raise RuntimeError("Steam client launch is supported only on Windows")
    os.startfile(f"steam://rungameid/{app_id}")


def stable_mapping(binary_dir: Path, executable: Path, timeout_seconds: int) -> Path:
    deadline = time.monotonic() + timeout_seconds
    sizes: dict[Path, tuple[int, float]] = {}
    while time.monotonic() < deadline:
        candidates = list((binary_dir / OWNED_DIRECTORY).rglob("*.usmap")) + list(binary_dir.glob("*.usmap"))
        for candidate in candidates:
            size = candidate.stat().st_size
            previous = sizes.get(candidate)
            if size > 1024 and previous and previous[0] == size and time.monotonic() - previous[1] >= 2:
                return candidate
            sizes[candidate] = (size, previous[1] if previous and previous[0] == size else time.monotonic())
        if not game_processes(executable):
            raise RuntimeError("Dragonwilds exited before producing mappings")
        time.sleep(1)
    raise TimeoutError("timed out waiting for UE4SS to generate Mappings.usmap")


def prepare(game_root: Path, ue4ss_root: Path, output: Path, work_root: Path,
            timeout_seconds: int = 600) -> dict[str, Any]:
    if os.name != "nt":
        raise RuntimeError("automatic mapping generation is supported only on Windows")
    root = game_root.resolve()
    executable = root / GAME_EXE
    binary_dir = executable.parent
    if not executable.is_file():
        raise ValueError(f"Dragonwilds client executable was not found: {executable}")
    paks = root / "RSDragonwilds/Content/Paks"
    if not paks.is_dir() or not any(paks.glob("*Windows.utoc")):
        raise ValueError("the selected root does not contain the expected Windows client containers")
    steam, app_id = steam_launch(root)
    if game_processes(executable):
        raise RuntimeError("Dragonwilds is already running; close it before generating mappings")
    journal_path = work_root.resolve() / "mapping-recovery.json"
    work_root.mkdir(parents=True, exist_ok=True)
    if journal_path.exists() and read_journal(journal_path).get("state") != "restored":
        recover(journal_path)
    proxy_source = ue4ss_root.resolve() / PROXY_NAME
    owned_source = ue4ss_root.resolve() / OWNED_DIRECTORY
    if not proxy_source.is_file() or not owned_source.is_dir():
        raise ValueError("the pinned UE4SS tool directory is incomplete")
    proxy = binary_dir / PROXY_NAME
    owned = binary_dir / OWNED_DIRECTORY
    proxy_staging = binary_dir / PROXY_STAGING_NAME
    directory_staging = binary_dir / DIRECTORY_STAGING_NAME
    collisions = [str(path) for path in (proxy, owned, proxy_staging, directory_staging) if path.exists()]
    if collisions:
        raise RuntimeError("refusing to overwrite an existing loader: " + ", ".join(collisions) + "; use --usmap-path instead")
    staging = work_root / "ue4ss-staging"
    if staging.exists():
        shutil.rmtree(staging)
    staging.mkdir()
    shutil.copy2(proxy_source, staging / PROXY_NAME)
    shutil.copytree(owned_source, staging / OWNED_DIRECTORY)
    configure_staging(staging)
    journal = {
        "schemaVersion": JOURNAL_SCHEMA,
        "state": "prepared",
        "createdAt": int(time.time()),
        "gameExecutable": str(executable),
        "gameExecutableSha256": sha256(executable),
        "steamAppId": app_id,
        "binaryDirectory": str(binary_dir),
        "proxySha256": sha256(staging / PROXY_NAME),
        "ownedFilesInstalled": snapshot_tree(staging / OWNED_DIRECTORY),
        "pathsAbsentBefore": [str(proxy), str(owned), str(proxy_staging), str(directory_staging)],
    }
    write_journal(journal_path, journal)
    launched = False
    mapping_copy: Path | None = None
    try:
        shutil.copy2(staging / PROXY_NAME, proxy_staging)
        proxy_staging.replace(proxy)
        shutil.copytree(staging / OWNED_DIRECTORY, directory_staging)
        directory_staging.replace(owned)
        journal["state"] = "installed"
        write_journal(journal_path, journal)
        launch_steam_app(steam, app_id)
        deadline = time.monotonic() + 120
        process_ids: list[int] = []
        while time.monotonic() < deadline and not process_ids:
            time.sleep(1)
            process_ids = game_processes(executable)
        if not process_ids:
            raise RuntimeError("Steam did not start Dragonwilds within two minutes")
        launched = True
        journal["state"] = "running"
        journal["processId"] = process_ids[0]
        write_journal(journal_path, journal)
        generated = stable_mapping(binary_dir, executable, timeout_seconds)
        journal["generatedMapping"] = {"path": str(generated.resolve()), "sha256": sha256(generated)}
        write_journal(journal_path, journal)
        output.parent.mkdir(parents=True, exist_ok=True)
        mapping_copy = output.resolve()
        shutil.copy2(generated, mapping_copy)
        journal["mappingSha256"] = sha256(mapping_copy)
        journal["state"] = "mapping-copied"
        write_journal(journal_path, journal)
        close_windows(process_ids[0])
        deadline = time.monotonic() + 120
        while time.monotonic() < deadline and game_processes(executable):
            time.sleep(1)
        if game_processes(executable):
            raise RuntimeError("mappings were generated, but Dragonwilds did not close gracefully; close it and rerun recovery")
    finally:
        if not launched or not game_processes(executable):
            recover(journal_path)
    if mapping_copy is None:
        raise RuntimeError("mapping generation did not produce an output file")
    return {"mapping": str(mapping_copy), "sha256": sha256(mapping_copy), "journal": str(journal_path), "restored": True}
