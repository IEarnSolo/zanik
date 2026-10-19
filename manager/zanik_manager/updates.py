from __future__ import annotations

import datetime as dt
import hashlib
import json
import os
import re
import shutil
import subprocess
import urllib.error
import urllib.request
import uuid
import zipfile
from pathlib import Path, PurePosixPath


RELEASE_API = "https://api.github.com/repos/IEarnSolo/zanik/releases/latest"
STRICT_TAG = re.compile(r"^v(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)$")
INTERVAL_SECONDS = 10 * 60
USER_AGENT = "Zanik-Manager-Update-Checker"
REQUIRED_MANAGER_FILES = {
    "VERSION", "ZanikManager.exe", "ZanikManagerUpdater.exe", "payload/payloads.json",
    "runtime/pythonw.exe", "app/zanik_manager/app.py",
}


def utc_now() -> str:
    return dt.datetime.now(dt.timezone.utc).isoformat().replace("+00:00", "Z")


def parse_version(value: str) -> tuple[int, int, int]:
    match = STRICT_TAG.fullmatch(value if value.startswith("v") else "v" + value)
    if not match:
        raise ValueError("Release tag is not a stable vMAJOR.MINOR.PATCH version")
    return tuple(int(part) for part in match.groups())


def release_update_components(result: dict, current_server_contract: dict | None,
                              current_asset_contract: dict | None) -> list[str]:
    """Classify a validated Manager release without conflating independent contracts."""
    components = []
    latest_server = result.get("serverPayload") or {}
    if (latest_server.get("contractSha256") and current_server_contract
            and latest_server["contractSha256"] != current_server_contract.get("contractSha256")):
        components.append("Zanik server")
    latest_assets = result.get("assetContract") or {}
    keys = ("extractionRecipeVersion", "compatibilityProfileVersion", "extractionRecipeSha256")
    if (latest_assets.get("extractionRecipeSha256") and current_asset_contract
            and any(latest_assets.get(key) != current_asset_contract.get(key) for key in keys)):
        components.append("website assets")
    return components or ["Manager only"]


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def atomic_json(path: Path, value: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + ".tmp")
    temporary.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    os.replace(temporary, path)


def read_json(path: Path) -> dict:
    try:
        return json.loads(path.read_text(encoding="utf-8-sig"))
    except (OSError, ValueError, TypeError):
        return {}


class GitHubReleaseChecker:
    def __init__(self, state_root: Path, opener=None):
        self.root = state_root
        self.state_path = state_root / "manager-update.json"
        self.opener = opener or urllib.request.urlopen

    def _request(self, url: str, *, etag: str = "") -> tuple[int, bytes, dict]:
        headers = {"Accept": "application/vnd.github+json", "User-Agent": USER_AGENT,
                   "X-GitHub-Api-Version": "2022-11-28"}
        if etag:
            headers["If-None-Match"] = etag
        request = urllib.request.Request(url, headers=headers)
        try:
            with self.opener(request, timeout=30) as response:
                return getattr(response, "status", 200), response.read(), dict(response.headers)
        except urllib.error.HTTPError as error:
            if error.code == 304:
                return 304, b"", dict(error.headers)
            raise RuntimeError(f"GitHub update check returned HTTP {error.code}") from error
        except (urllib.error.URLError, TimeoutError, OSError) as error:
            raise RuntimeError(f"GitHub update check failed: {getattr(error, 'reason', error)}") from error

    def check(self, current_version: str, *, force: bool = False) -> dict:
        cached = read_json(self.state_path)
        if not force and cached.get("validated"):
            try:
                checked = dt.datetime.fromisoformat(cached["checkedAt"].replace("Z", "+00:00"))
                if (dt.datetime.now(dt.timezone.utc) - checked).total_seconds() < INTERVAL_SECONDS:
                    cached.update({"currentVersion": current_version,
                                   "updateAvailable": parse_version(cached["latestVersion"]) > parse_version(current_version),
                                   "fromCache": True})
                    return cached
            except (KeyError, ValueError, TypeError):
                pass
        status, body, headers = self._request(RELEASE_API, etag=cached.get("etag", ""))
        if status == 304:
            if not cached.get("validated"):
                raise RuntimeError("GitHub returned an unchanged release without a validated local result")
            cached.update({"checkedAt": utc_now(), "currentVersion": current_version,
                           "updateAvailable": parse_version(cached["latestVersion"]) > parse_version(current_version),
                           "fromCache": True})
            atomic_json(self.state_path, cached)
            return cached

        release = json.loads(body.decode("utf-8"))
        if release.get("draft") or release.get("prerelease"):
            raise RuntimeError("GitHub's latest release is not a stable public release")
        tag = str(release.get("tag_name", ""))
        latest = parse_version(tag)
        current = parse_version(current_version)
        version = tag[1:]
        expected_zip = f"zanik-manager-windows-x86_64-{version}.zip"
        assets = {item.get("name"): item for item in release.get("assets", [])}
        if expected_zip not in assets or "release-manifest.json" not in assets:
            raise RuntimeError("GitHub release is missing the Manager ZIP or release manifest")
        manifest_asset = assets["release-manifest.json"]
        _, manifest_body, _ = self._request(manifest_asset["browser_download_url"])
        manifest = json.loads(manifest_body.decode("utf-8"))
        if manifest.get("schemaVersion") != 2 or manifest.get("tag") != tag or manifest.get("version") != version:
            raise RuntimeError("GitHub release manifest does not match the release tag")
        server_payload = manifest.get("serverPayload")
        if server_payload is not None:
            digest = str(server_payload.get("contractSha256", "")).lower()
            if (server_payload.get("contractSchemaVersion") != 1 or
                    server_payload.get("releaseVersion") != version or len(digest) != 64 or
                    any(character not in "0123456789abcdef" for character in digest)):
                raise RuntimeError("GitHub release manifest has invalid server payload metadata")
        game_compatibility = manifest.get("gameCompatibility")
        if game_compatibility is not None:
            hashes = game_compatibility.get("linuxExecutableSha256")
            if (game_compatibility.get("schemaVersion") != 1 or not isinstance(hashes, list) or not hashes or
                    any(not isinstance(value, str) or len(value) != 64 or
                        any(character not in "0123456789abcdef" for character in value) for value in hashes)):
                raise RuntimeError("GitHub release manifest has invalid game compatibility metadata")
        asset_contract = manifest.get("assetContract")
        if asset_contract is not None:
            recipe_digest = str(asset_contract.get("extractionRecipeSha256", "")).lower()
            if (not isinstance(asset_contract.get("extractionRecipeVersion"), int)
                    or asset_contract["extractionRecipeVersion"] < 1
                    or not isinstance(asset_contract.get("compatibilityProfileVersion"), str)
                    or not asset_contract["compatibilityProfileVersion"]
                    or len(recipe_digest) != 64
                    or any(character not in "0123456789abcdef" for character in recipe_digest)):
                raise RuntimeError("GitHub release manifest has invalid asset contract metadata")
        records = [entry for entry in manifest.get("files", []) if entry.get("name") == expected_zip]
        if len(records) != 1:
            raise RuntimeError("GitHub release manifest has no unique Manager archive record")
        record = records[0]
        zip_asset = assets[expected_zip]
        if int(zip_asset.get("size", -1)) != int(record.get("size", -2)):
            raise RuntimeError("GitHub Manager asset size does not match the release manifest")
        api_digest = zip_asset.get("digest")
        if api_digest and api_digest != "sha256:" + record.get("sha256", ""):
            raise RuntimeError("GitHub Manager asset digest does not match the release manifest")
        result = {
            "schemaVersion": 1, "validated": True, "checkedAt": utc_now(),
            "currentVersion": current_version, "latestVersion": version, "tag": tag,
            "updateAvailable": latest > current, "releaseUrl": release.get("html_url", ""),
            "etag": headers.get("ETag") or headers.get("Etag") or "",
            "serverPayload": server_payload,
            "gameCompatibility": game_compatibility,
            "assetContract": asset_contract,
            "manager": {"name": expected_zip, "url": zip_asset["browser_download_url"],
                        "size": int(record["size"]), "sha256": record["sha256"]},
        }
        atomic_json(self.state_path, result)
        return result

    def cached(self) -> dict:
        return read_json(self.state_path)

    def stage(self, result: dict, install_root: Path) -> dict:
        if not result.get("validated") or not result.get("updateAvailable"):
            raise RuntimeError("No validated Manager update is available")
        manager = result["manager"]
        update_root = self.root / result["latestVersion"]
        update_root.mkdir(parents=True, exist_ok=True)
        archive = update_root / manager["name"]
        request = urllib.request.Request(manager["url"], headers={"User-Agent": USER_AGENT})
        try:
            with self.opener(request, timeout=300) as response, archive.open("wb") as output:
                shutil.copyfileobj(response, output, 1024 * 1024)
        except (urllib.error.URLError, TimeoutError, OSError) as error:
            raise RuntimeError(f"Manager update download failed: {getattr(error, 'reason', error)}") from error
        if archive.stat().st_size != manager["size"] or sha256_file(archive) != manager["sha256"]:
            raise RuntimeError("Downloaded Manager update failed size or SHA-256 verification")

        extracted = update_root / "verified"
        if extracted.exists():
            shutil.rmtree(extracted)
        extracted.mkdir()
        seen: set[str] = set(); total_size = 0
        reserved = {"con", "prn", "aux", "nul", *(f"com{i}" for i in range(1, 10)), *(f"lpt{i}" for i in range(1, 10))}
        with zipfile.ZipFile(archive) as package:
            for item in package.infolist():
                name = item.filename.replace("\\", "/")
                while name.startswith("./"):
                    name = name[2:]
                path = PurePosixPath(name)
                mode = (item.external_attr >> 16) & 0o170000
                windows_key = name.casefold()
                unsafe_part = any(part.rstrip(". ").split(".", 1)[0].casefold() in reserved or part.endswith((".", " ")) for part in path.parts)
                total_size += item.file_size
                if (not name or path.is_absolute() or ".." in path.parts or ":" in name or
                        windows_key in seen or mode == 0o120000 or item.flag_bits & 1 or unsafe_part or
                        item.file_size > 1024 * 1024 * 1024 or total_size > 2 * 1024 * 1024 * 1024):
                    raise RuntimeError(f"Manager update archive contains an unsafe path: {item.filename}")
                seen.add(windows_key)
                destination = extracted.joinpath(*path.parts)
                if item.is_dir():
                    destination.mkdir(parents=True, exist_ok=True)
                else:
                    destination.parent.mkdir(parents=True, exist_ok=True)
                    with package.open(item) as source, destination.open("wb") as output:
                        shutil.copyfileobj(source, output)
        missing = {name for name in REQUIRED_MANAGER_FILES if name.casefold() not in seen}
        if missing:
            raise RuntimeError("Manager update archive is incomplete: " + ", ".join(sorted(missing)))
        if (extracted / "VERSION").read_text(encoding="utf-8-sig").strip() != result["latestVersion"]:
            raise RuntimeError("Extracted Manager VERSION does not match the release")
        payloads = read_json(extracted / "payload" / "payloads.json")
        if payloads.get("version") != result["latestVersion"]:
            raise RuntimeError("Extracted Manager payload version does not match the release")

        target = install_root.resolve()
        if not (target / "ZanikManager.exe").is_file() or not (target / "payload" / "payloads.json").is_file():
            raise RuntimeError("The running directory is not a recognized portable Zanik Manager installation")
        sibling = target.parent / f".zanik-manager-update-{result['latestVersion']}-{uuid.uuid4().hex[:8]}"
        try:
            shutil.copytree(extracted, sibling)
        except OSError as error:
            return {"ready": False, "staged": str(extracted),
                    "message": "The verified update is ready, but the current Manager directory cannot be replaced automatically.",
                    "error": str(error)}
        if sha256_file(sibling / "ZanikManager.exe") != sha256_file(extracted / "ZanikManager.exe"):
            shutil.rmtree(sibling, ignore_errors=True)
            raise RuntimeError("The prepared Manager update did not verify")
        backup = target.parent / f"{target.name}.backup-{result['currentVersion']}-{uuid.uuid4().hex[:8]}"
        handshake = self.root / "startup-handshake.json"
        journal = self.root / "self-update-journal.json"
        request_data = {"schemaVersion": 1, "state": "ready", "createdAt": utc_now(),
                        "target": str(target), "source": str(sibling), "backup": str(backup),
                        "handshake": str(handshake), "newVersion": result["latestVersion"]}
        atomic_json(journal, request_data)
        updater_copy = self.root / "ZanikManagerUpdater.exe"
        shutil.copy2(extracted / "ZanikManagerUpdater.exe", updater_copy)
        return {"ready": True, "journal": str(journal), "updater": str(updater_copy),
                "staged": str(sibling), "version": result["latestVersion"]}

    @staticmethod
    def launch(staged: dict) -> None:
        if not staged.get("ready"):
            raise RuntimeError("The Manager update is not ready to install")
        journal = read_json(Path(staged["journal"]))
        updater = Path(staged["updater"]).resolve()
        subprocess.Popen([str(updater), "--pid", str(os.getpid()),
                          "--source", journal["source"], "--target", journal["target"],
                          "--backup", journal["backup"], "--handshake", journal["handshake"],
                          "--journal", staged["journal"]], close_fds=True,
                         cwd=str(updater.parent))


def current_manager_version(distribution_root: Path) -> str:
    version = distribution_root / "VERSION"
    if version.is_file():
        return version.read_text(encoding="utf-8-sig").strip()
    payloads = read_json(distribution_root / "payload" / "payloads.json")
    return str(payloads.get("version") or "0.0.0")


def write_startup_handshake(path: Path, version: str) -> None:
    atomic_json(path, {"schemaVersion": 1, "version": version, "startedAt": utc_now()})


def prune_updater_backups(install_root: Path) -> None:
    """Retain the newest recognized updater backup and remove only older recognized copies."""
    candidates = []
    for path in install_root.parent.glob(install_root.name + ".backup-*"):
        if (path / "ZanikManager.exe").is_file() and (path / "VERSION").is_file() and (path / "payload" / "payloads.json").is_file():
            candidates.append(path)
    candidates.sort(key=lambda value: value.stat().st_mtime, reverse=True)
    for old in candidates[1:]:
        shutil.rmtree(old, ignore_errors=True)


def prune_update_staging(update_root: Path) -> tuple[int, int]:
    """Remove recognized version staging only after the native updater completed."""
    status_path = update_root / "self-update-journal.json.status"
    try:
        status = json.loads(status_path.read_text(encoding="utf-8-sig"))
    except (OSError, UnicodeError, json.JSONDecodeError):
        return 0, 0
    if status.get("state") != "complete":
        return 0, 0
    removed = 0; reclaimed = 0
    for path in update_root.iterdir() if update_root.is_dir() else ():
        if not path.is_dir():
            continue
        try:
            parse_version(path.name)
        except ValueError:
            continue
        verified = path / "verified"
        version_file = verified / "VERSION"
        if not ((verified / "ZanikManager.exe").is_file()
                and (verified / "payload" / "payloads.json").is_file()
                and version_file.is_file()):
            continue
        try:
            if version_file.read_text(encoding="utf-8-sig").strip() != path.name:
                continue
            reclaimed += sum(item.stat().st_size for item in path.rglob("*") if item.is_file())
            shutil.rmtree(path)
            removed += 1
        except (OSError, UnicodeError):
            continue
    return removed, reclaimed


def update_staging_cleanup_pending(update_root: Path) -> bool:
    """Report a recognized staged update that is still awaiting updater completion."""
    try:
        candidates = []
        for path in update_root.iterdir() if update_root.is_dir() else ():
            if not path.is_dir():
                continue
            try:
                parse_version(path.name)
            except ValueError:
                continue
            verified = path / "verified"
            if ((verified / "ZanikManager.exe").is_file()
                    and (verified / "VERSION").is_file()
                    and (verified / "payload" / "payloads.json").is_file()):
                candidates.append(path)
        if not candidates:
            return False
        status = json.loads((update_root / "self-update-journal.json.status").read_text(encoding="utf-8-sig"))
        return status.get("state") != "complete"
    except (OSError, UnicodeError, json.JSONDecodeError):
        return bool(candidates) if 'candidates' in locals() else False
