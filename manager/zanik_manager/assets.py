from __future__ import annotations

import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile
import time
import zipfile
from pathlib import Path, PurePosixPath

from .cache import BundleCache, CachedBundle
from .lifecycle import PterodactylProvider, safe_archive_name, sha256


CRITICAL_REMOTE_ASSETS = {
    "asset-manifest.json",
    "world-map.jpg",
    "buildings/catalog.json",
    "npc-catalog.json",
    "npc-identities.tsv",
    "quest-catalog.json",
    "quest-runtime-catalog.tsv",
    "quest-step-locations.json",
    "recipe-unlocks.json",
    "recipe-runtime-catalog.tsv",
    "resource-catalog.json",
    "resource-identities.tsv",
}

GAME_EXECUTABLE = Path("RSDragonwilds/Binaries/Win64/RSDragonwilds-Win64-Shipping.exe")


def _tree_size(path: Path) -> int:
    try:
        return sum(item.stat().st_size for item in path.rglob("*") if item.is_file())
    except OSError:
        return 0


def compact_completed_asset_work(work_root: Path) -> int:
    """Drop reproducible output while preserving the expensive incremental exports."""
    marker = work_root / ".zanik-asset-work.json"
    journal = work_root / "extraction-journal.json"
    try:
        owner = json.loads(marker.read_text(encoding="utf-8-sig"))
        state = json.loads(journal.read_text(encoding="utf-8-sig"))
    except (OSError, UnicodeError, json.JSONDecodeError):
        return 0
    if owner.get("owner") != "Zanik asset extractor" or state.get("stage") != "complete":
        return 0
    pipeline = work_root / "pipeline"
    targets = [work_root / "generated-web", work_root / "generated-web.previous",
               pipeline / "web", pipeline / "internal"]
    targets.extend(pipeline.glob("*.log") if pipeline.is_dir() else ())
    reclaimed = 0
    for target in targets:
        try:
            if target.is_dir():
                reclaimed += _tree_size(target); shutil.rmtree(target)
            elif target.is_file():
                reclaimed += target.stat().st_size; target.unlink()
        except OSError:
            continue
    return reclaimed


def cleanup_extraction_output(output: Path) -> int:
    """Remove Manager-owned bundle copies and upload parts after cache insertion."""
    if not output.is_dir():
        return 0
    reclaimed = 0
    for target in output.iterdir():
        if target.is_file() and target.name.startswith("zanik-assets-") and target.suffix.lower() == ".zip":
            try:
                reclaimed += target.stat().st_size; target.unlink()
            except OSError:
                continue
        elif target.is_dir() and target.name.startswith("zanik-assets-") and target.name.endswith("-parts"):
            try:
                reclaimed += _tree_size(target); shutil.rmtree(target)
            except OSError:
                continue
    return reclaimed


def background_creation_flags(platform: str = os.name) -> int:
    """Hide console-only helpers when launched by the Windows GUI."""
    return int(getattr(subprocess, "CREATE_NO_WINDOW", 0)) if platform == "nt" else 0


def expected_asset_contract(tool_root: Path) -> dict:
    candidates = (tool_root / "asset-tools" / "extraction-recipes.json",
                  tool_root / "extraction-recipes.json")
    path = next((candidate for candidate in candidates if candidate.is_file()), None)
    if path is None:
        raise RuntimeError("The packaged extraction recipe is missing")
    value = json.loads(path.read_text(encoding="utf-8"))
    recipe = value.get("schemaVersion")
    profile = value.get("catalogProfile", {}).get("version")
    if not isinstance(recipe, int) or recipe < 1 or not isinstance(profile, str) or not profile:
        raise RuntimeError("The packaged extraction recipe contract is invalid")
    return {"extractionRecipeVersion": recipe, "compatibilityProfileVersion": profile,
            "extractionRecipeSha256": hashlib.sha256(path.read_bytes()).hexdigest(),
            "criticalFiles": sorted(CRITICAL_REMOTE_ASSETS - {"asset-manifest.json"})}


def asset_setup_mode(use_cached: bool, resume_status: dict | None) -> str:
    """Choose the cheapest safe source for a guided asset setup."""
    if use_cached:
        return "cached"
    if resume_status and resume_status.get("incremental"):
        return "incremental"
    if resume_status:
        return "resume"
    return "new"


def resume_activity_message(stage: str) -> str:
    """Describe exactly which expensive work a verified resume will repeat."""
    if stage == "mappings":
        return ("Resuming from the verified mapping; Dragonwilds will not reopen. "
                "UnrealAssetScout will continue with catalog and artwork export.")
    return (f"Resuming verified asset work from stage {stage}; Dragonwilds and completed "
            "UnrealAssetScout exports will not run again.")


def game_root_id(path: Path) -> str:
    return hashlib.sha256(str(path.resolve()).lower().encode()).hexdigest()[:16]


def discover_steam_game_roots() -> list[Path]:
    candidates: list[Path] = []
    try:
        import winreg
        for hive in (winreg.HKEY_CURRENT_USER, winreg.HKEY_LOCAL_MACHINE):
            for key_name in (r"Software\Valve\Steam", r"Software\WOW6432Node\Valve\Steam"):
                try:
                    with winreg.OpenKey(hive, key_name) as key:
                        root = Path(winreg.QueryValueEx(key, "SteamPath")[0])
                        candidates.append(root / "steamapps" / "common" / "RSDragonwilds")
                except OSError:
                    pass
    except ImportError:
        pass
    for drive in "CDEFG":
        candidates.append(Path(f"{drive}:/SteamLibrary/steamapps/common/RSDragonwilds"))
    unique = []
    for path in candidates:
        if path.is_dir() and path not in unique:
            unique.append(path)
    return unique


def dragonwilds_is_running(runner=subprocess.run, platform: str = os.name) -> bool:
    """Return whether a Windows Dragonwilds client process is currently running."""
    if platform != "nt":
        return False
    result = runner(
        ["powershell.exe", "-NoProfile", "-NonInteractive", "-Command",
         "@(Get-Process -Name 'RSDragonwilds*' -ErrorAction SilentlyContinue).Count"],
        capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=15,
        creationflags=background_creation_flags(platform),
    )
    try:
        return int((result.stdout or "0").strip().splitlines()[-1]) > 0
    except (ValueError, IndexError):
        return False


def request_dragonwilds_close(runner=subprocess.run, timeout: int = 120,
                              platform: str = os.name) -> bool:
    """Request a normal window close and wait; never terminates the process."""
    if platform != "nt":
        return True
    runner(
        ["powershell.exe", "-NoProfile", "-NonInteractive", "-Command",
         "Get-Process -Name 'RSDragonwilds*' -ErrorAction SilentlyContinue | "
         "ForEach-Object { [void]$_.CloseMainWindow() }"],
        capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=15,
        creationflags=background_creation_flags(platform),
    )
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if not dragonwilds_is_running(runner, platform):
            return True
        time.sleep(1)
    return False


class AssetExtractor:
    def __init__(self, tool_root: Path, cache: BundleCache):
        self.tool_root = tool_root
        self.cache = cache

    def _python(self) -> str:
        embedded = self.tool_root / "runtime" / "python.exe"
        return str(embedded if embedded.is_file() else Path(sys.executable))

    def toolchain_summary(self, game_root: Path) -> str:
        version = "unknown"
        payload = self.tool_root.parent / "payload" / "server-payload.json"
        recipe_path = self.tool_root / "asset-tools" / "extraction-recipes.json"
        lock_path = self.tool_root / "asset-tools" / "tools.lock.json"
        try:
            version = str(json.loads(payload.read_text(encoding="utf-8")).get("version", version))
        except (OSError, UnicodeError, json.JSONDecodeError):
            pass
        try:
            recipe = str(json.loads(recipe_path.read_text(encoding="utf-8")).get("schemaVersion", "unknown"))
        except (OSError, UnicodeError, json.JSONDecodeError):
            recipe = "unknown"
        try:
            tools = json.loads(lock_path.read_text(encoding="utf-8")).get("tools", {})
            uas = str(tools.get("unreal-asset-scout-windows-x86_64", {}).get("version", "unknown"))
            python_version = str(tools.get("python-embed-windows-x86_64", {}).get("version", "unknown"))
            ue4ss = str(tools.get("ue4ss-windows-x86_64", {}).get("version", "unknown"))
        except (OSError, UnicodeError, json.JSONDecodeError):
            uas = python_version = ue4ss = "unknown"
        game_build = "unknown"
        tool = self.tool_root / "tools" / "zanik_assets.py"
        try:
            completed = subprocess.run(
                [self._python(), str(tool), "show-build", "--game-root", str(game_root)],
                capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=30,
                creationflags=background_creation_flags(),
            )
            if completed.returncode == 0 and completed.stdout.strip():
                game_build = completed.stdout.strip()
        except (OSError, subprocess.SubprocessError):
            pass
        return (f"Zanik Manager/extractor {version} · recipe {recipe} · game {game_build} · "
                f"UnrealAssetScout {uas} · UE4SS {ue4ss} · Python {python_version}")

    def mapping_preflight(self, game_root: Path, recovery_root: Path) -> dict:
        tool = self.tool_root / "tools" / "zanik_assets.py"
        completed = subprocess.run(
            [self._python(), str(tool), "mapping-status", "--game-root", str(game_root),
             "--recovery-root", str(recovery_root)], capture_output=True, text=True,
            encoding="utf-8", errors="replace", timeout=60,
            creationflags=background_creation_flags(),
        )
        if completed.returncode:
            raise RuntimeError("Asset preflight failed: " + (completed.stderr or completed.stdout).strip())
        return json.loads(completed.stdout)

    def validate_bundle(self, bundle: Path, contract: dict | None = None) -> None:
        tool = self.tool_root / "tools" / "zanik_assets.py"
        completed = subprocess.run([self._python(), str(tool), "validate", "--bundle", str(bundle)],
                                   capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=300,
                                   creationflags=background_creation_flags())
        if completed.returncode:
            raise RuntimeError("Cached asset bundle validation failed")
        if contract:
            try:
                with zipfile.ZipFile(bundle) as archive:
                    manifest = json.loads(archive.read("asset-manifest.json"))
            except (OSError, KeyError, zipfile.BadZipFile, json.JSONDecodeError) as error:
                raise RuntimeError("Cached asset bundle manifest is unreadable") from error
            if (manifest.get("extractionRecipeVersion") != contract.get("extractionRecipeVersion")
                    or manifest.get("compatibilityProfile", {}).get("version") !=
                    contract.get("compatibilityProfileVersion")
                    or manifest.get("extractionRecipeSha256") !=
                    contract.get("extractionRecipeSha256")):
                raise RuntimeError(
                    "The selected asset bundle uses an older extraction recipe or compatibility profile. "
                    "Update the retained incremental cache or perform a new extraction before uploading it.")

    @staticmethod
    def _file_sha256(path: Path) -> str:
        value = hashlib.sha256()
        with path.open("rb") as stream:
            for chunk in iter(lambda: stream.read(1024 * 1024), b""):
                value.update(chunk)
        return value.hexdigest()

    def current_fingerprints(self, game_root: Path) -> dict[str, str]:
        paths = {
            "gameExecutableSha256": game_root / GAME_EXECUTABLE,
            "recipeSha256": self.tool_root / "asset-tools" / "extraction-recipes.json",
            "toolLockSha256": self.tool_root / "asset-tools" / "tools.lock.json",
        }
        missing = [str(path) for path in paths.values() if not path.is_file()]
        if missing:
            raise RuntimeError("Cannot validate resumable extraction; required file is missing: " + ", ".join(missing))
        return {name: self._file_sha256(path) for name, path in paths.items()}

    def resume_compatibility(self, work_root: Path, game_root: Path) -> dict | None:
        status = self.resumable(work_root)
        if not status:
            return None
        current = self.current_fingerprints(game_root)
        expected = status.get("fingerprints", {})
        mismatches = [name for name, value in current.items() if expected.get(name) != value]
        incremental = status.get("stage") == "complete"
        unsafe = [name for name in mismatches if name == "toolLockSha256"]
        return {**status, "compatible": not (unsafe if incremental else mismatches),
                "incremental": incremental, "mappingRequired": "gameExecutableSha256" in mismatches,
                "mismatches": mismatches, "unsafeMismatches": unsafe}

    def bundle_compatible_with_game(self, bundle: Path, game_root: Path,
                                    executable_sha256: str | None = None,
                                    contract: dict | None = None) -> bool:
        try:
            with zipfile.ZipFile(bundle) as archive:
                manifest = json.loads(archive.read("asset-manifest.json"))
            source = manifest.get("sourceExecutable") or {}
            current = executable_sha256 or self.current_fingerprints(game_root)["gameExecutableSha256"]
            if source.get("sha256") != current:
                return False
            if contract and (manifest.get("extractionRecipeVersion") != contract.get("extractionRecipeVersion")
                             or manifest.get("compatibilityProfile", {}).get("version") !=
                             contract.get("compatibilityProfileVersion")
                             or manifest.get("extractionRecipeSha256") !=
                             contract.get("extractionRecipeSha256")):
                return False
            return True
        except (OSError, KeyError, ValueError, zipfile.BadZipFile, json.JSONDecodeError, RuntimeError):
            return False

    def extract(self, game_root: Path, output: Path, event=lambda event: None,
                usmap: Path | None = None, keep_work: bool = False,
                preserve_existing_loader: bool = False,
                recovery_root: Path | None = None, work_root: Path | None = None,
                resume: bool = False, incremental: bool = False) -> CachedBundle:
        script = self.tool_root / "init-assets.ps1"
        if not script.exists():
            raise FileNotFoundError("The bundled Windows asset extractor is missing")
        if not resume and not usmap and dragonwilds_is_running():
            raise RuntimeError(
                "Dragonwilds is already running. Close the game before extraction, then try again. "
                "Zanik Manager will launch and close it automatically while generating mappings."
            )
        command = ["powershell.exe", "-NoProfile", "-NonInteractive", "-ExecutionPolicy", "Bypass", "-File",
                   str(script), "-GameRoot", str(game_root), "-OutputDirectory", str(output)]
        if usmap:
            command += ["-UsmapPath", str(usmap), "-SkipGameLaunch"]
        if keep_work:
            command.append("-KeepWork")
        if preserve_existing_loader:
            command.append("-PreserveExistingLoader")
        if recovery_root:
            command += ["-RecoveryDirectory", str(recovery_root)]
        if work_root:
            command += ["-WorkDirectory", str(work_root)]
        if resume:
            command.append("-Resume")
        if incremental:
            command.append("-Incremental")
        event({"phase": "extract", "message": self.toolchain_summary(game_root)})
        event({"phase": "extract", "message": "Starting the Windows client asset extractor"})
        process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
                                   encoding="utf-8", errors="replace",
                                   creationflags=background_creation_flags())
        assert process.stdout
        for line in process.stdout:
            line = line.rstrip()
            if line.startswith("ZANIK_EVENT "):
                try:
                    event(json.loads(line[12:]))
                    continue
                except json.JSONDecodeError:
                    pass
            event({"phase": "extract", "message": line})
        code = process.wait()
        if code:
            raise RuntimeError(f"Asset extractor failed with exit code {code}")
        bundles = sorted(output.glob("zanik-assets-*.zip"), key=lambda path: path.stat().st_mtime, reverse=True)
        bundles = [path for path in bundles if "-parts" not in path.name]
        if not bundles:
            raise RuntimeError("Asset extraction completed without producing a bundle")
        with zipfile.ZipFile(bundles[0]) as archive:
            manifest = json.loads(archive.read("asset-manifest.json"))
        item = self.cache.add(bundles[0], game_root_id(game_root), manifest)
        reclaimed = cleanup_extraction_output(output)
        if work_root:
            reclaimed += compact_completed_asset_work(work_root)
        if reclaimed:
            event({"phase": "cleanup", "message":
                   f"Cleaned {reclaimed / (1024 ** 3):.2f} GiB of reproducible extraction output; the incremental cache was retained"})
        return item

    def resumable(self, work_root: Path) -> dict | None:
        journal = work_root / "extraction-journal.json"
        marker = work_root / ".zanik-asset-work.json"
        if not journal.is_file() or not marker.is_file():
            return None
        try:
            ownership = json.loads(marker.read_text(encoding="utf-8-sig"))
            value = json.loads(journal.read_text(encoding="utf-8-sig"))
        except (OSError, UnicodeError, json.JSONDecodeError):
            return None
        if ownership.get("owner") != "Zanik asset extractor" or value.get("schemaVersion") != 1:
            return None
        fingerprints = value.get("fingerprints", {})
        return {"stage": str(value.get("stage", "unknown")), "updatedAt": str(value.get("updatedAt", "")),
                "fingerprints": fingerprints if isinstance(fingerprints, dict) else {}}

    def discard(self, work_root: Path) -> None:
        tool = self.tool_root / "tools" / "zanik_assets.py"
        pending = work_root.parent / f"pending-cleanup-{work_root.name}.json"
        completed = subprocess.run(
            [self._python(), str(tool), "cleanup-work", "--work-root", str(work_root),
             "--pending-journal", str(pending)],
            capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=180,
        )
        if completed.returncode:
            raise RuntimeError("Could not safely discard the extraction workspace: " +
                               (completed.stderr or completed.stdout).strip())

class AssetDeployer:
    def __init__(self, provider: PterodactylProvider):
        self.provider = provider

    def activate(self, profile_id: str, server_id: str, server_root: str, bundle: Path,
                 progress=lambda current, total, path: None,
                 status=lambda message: None) -> dict:
        self.provider.preflight(server_id, "assets")
        root = server_root.rstrip("/")
        with tempfile.TemporaryDirectory(prefix="zanik-manager-assets-") as temporary:
            tree = Path(temporary) / "tree"
            tree.mkdir()
            with zipfile.ZipFile(bundle) as archive:
                for item in archive.infolist():
                    if item.is_dir():
                        continue
                    name = safe_archive_name(item.filename)
                    target = tree / name
                    target.parent.mkdir(parents=True, exist_ok=True)
                    target.write_bytes(archive.read(item))
            manifest = json.loads((tree / "asset-manifest.json").read_text(encoding="utf-8"))
            if manifest.get("compatible") is False:
                raise RuntimeError("The selected asset bundle is marked incompatible")
            expected = {entry["path"]: entry["sha256"] for entry in manifest.get("files", [])}
            for name, digest in expected.items():
                target = tree / safe_archive_name(name)
                if not target.is_file() or sha256(target.read_bytes()) != digest:
                    raise RuntimeError(f"Bundle validation failed for {name}")
            journal = self.provider.journals
            from .journal import OperationJournal
            operation = OperationJournal.create(profile_id, "activate-assets")
            journal.save(operation)
            stage = f"{root}/.zanik-manager/staging/{operation.operation_id}/assets"
            self.provider._mkdir(server_id, stage)
            files = sorted(path for path in tree.rglob("*") if path.is_file())
            used_archives = False
            try:
                parts, index_data = self._make_parts(bundle, Path(temporary) / "parts")
                self.provider.client.write_file(server_id, f"{stage}/bundle-index.json", json.dumps(index_data, sort_keys=True).encode())
                for part in parts:
                    self.provider.client.upload(server_id, stage, part.name, part.read_bytes())
                    remote_hash = sha256(self.provider.client.download_file(server_id, f"{stage}/{part.name}"))
                    if remote_hash != sha256(part.read_bytes()):
                        raise RuntimeError(f"Remote part verification failed for {part.name}")
                    self.provider.client.decompress(server_id, stage, part.name)
                    self.provider.client.delete(server_id, stage, [part.name])
                self.provider.client.delete(server_id, stage, ["bundle-index.json"])
                representatives = set(CRITICAL_REMOTE_ASSETS)
                for record in index_data["parts"]:
                    names = record.get("files", [])
                    if names:
                        representatives.update((names[0], names[len(names) // 2], names[-1]))
                self._verify_remote_subset(server_id, stage, tree, representatives, progress)
                used_archives = True
            except Exception:
                # Some Pterodactyl versions acknowledge archive extraction without
                # doing it. Recreate the stage and use the slower verified path.
                operation.completed_steps.append("archive-upload-fallback")
                self.provider.client.delete(server_id, str(PurePosixPath(stage).parent), [PurePosixPath(stage).name])
                self.provider._mkdir(server_id, stage)
                for index, path in enumerate(files, 1):
                    relative = path.relative_to(tree).as_posix()
                    directory = str(PurePosixPath(stage) / PurePosixPath(relative).parent)
                    self.provider._mkdir(server_id, directory)
                    data = path.read_bytes()
                    self.provider.client.upload(server_id, directory, path.name, data)
                    progress(index, len(files), relative)
                self._verify_remote_subset(server_id, stage, tree, CRITICAL_REMOTE_ASSETS, progress)
            current = f"{root}/zanik/assets/current"
            previous = f"{root}/zanik/assets/previous"
            if self.provider._directory_exists(server_id, previous):
                self.provider.client.delete(server_id, f"{root}/zanik/assets", ["previous"])
            if self.provider._directory_exists(server_id, current):
                self.provider.client.rename(server_id, f"{root}/zanik/assets", [("current", "previous")])
                operation.rollback.append({"from": previous, "to": current})
            self.provider.client.rename(server_id, root, [(str(PurePosixPath(stage).relative_to(root)), "zanik/assets/current")])
            operation.state = "complete"; operation.completed_steps.append("assets-activated"); journal.save(operation)
            restart_context = self._restart_runtime_if_running(server_id, status)
            restarted = restart_context is not None
            if restarted:
                operation.completed_steps.append("runtime-restart-requested"); journal.save(operation)
            return {"operationId": operation.operation_id, "manifest": manifest,
                    "bundleSha256": sha256(bundle.read_bytes()), "fileCount": len(files),
                    "archiveUpload": used_archives, "restartRequested": restarted,
                    "restartPreviousUptime": (restart_context or {}).get("previousUptime")}

    def _restart_runtime_if_running(self, server_id: str, status=lambda message: None) -> dict | None:
        resources = self.provider.client.resources(server_id)
        if resources.get("current_state", "") != "running":
            return None
        previous_uptime = int(resources.get("resources", {}).get("uptime") or 0)
        status("Restarting the game server to load the new runtime catalogs")
        self.provider.client.power(server_id, "restart")
        return {"previousUptime": previous_uptime}

    @staticmethod
    def _make_parts(bundle: Path, output: Path, limit: int = 90 * 1024 * 1024) -> tuple[list[Path], dict]:
        output.mkdir(parents=True, exist_ok=True)
        with zipfile.ZipFile(bundle) as source:
            members = [item for item in source.infolist() if not item.is_dir()]
            groups=[]; current=[]; estimated=0
            for item in members:
                if item.compress_size > limit:
                    raise RuntimeError(f"A single asset exceeds the upload part limit: {item.filename}")
                safety=min(1024 * 1024, max(64, limit // 20))
                if current and estimated + item.compress_size > limit - safety:
                    groups.append(current); current=[]; estimated=0
                current.append(item); estimated += item.compress_size
            if current: groups.append(current)
            records=[]; parts=[]; queue=list(groups)
            while queue:
                group=queue.pop(0); candidate=output/"candidate.zip"
                with zipfile.ZipFile(candidate, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as target:
                    for item in group: target.writestr(item, source.read(item))
                if candidate.stat().st_size >= limit and len(group)>1:
                    candidate.unlink(); middle=len(group)//2; queue[0:0]=[group[:middle],group[middle:]]; continue
                if candidate.stat().st_size >= limit:
                    candidate.unlink(); raise RuntimeError(f"A single generated asset part exceeds the upload limit: {group[0].filename}")
                part=output/f"assets-{len(parts)+1:03d}.zip"; candidate.replace(part)
                parts.append(part); records.append({"name":part.name,"sha256":sha256(part.read_bytes()),
                                                    "size":part.stat().st_size,"files":[safe_archive_name(item.filename) for item in group]})
        return parts, {"schemaVersion":1,"bundle":bundle.name,"bundleSha256":sha256(bundle.read_bytes()),"parts":records}

    def _verify_remote_subset(self, server_id: str, stage: str, tree: Path,
                              selected: set[str], progress) -> None:
        files = sorted(
            tree / safe_archive_name(relative)
            for relative in selected
            if (tree / safe_archive_name(relative)).is_file()
        )
        for index, path in enumerate(files, 1):
            relative = path.relative_to(tree).as_posix()
            actual=sha256(self.provider.client.download_file(server_id, str(PurePosixPath(stage)/relative)))
            if actual != sha256(path.read_bytes()):
                raise RuntimeError(f"Remote asset verification failed for {relative}")
            progress(index, len(files), relative)
