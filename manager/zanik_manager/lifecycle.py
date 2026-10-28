from __future__ import annotations

import datetime as dt
import hashlib
import json
import os
import re
import secrets
import stat
import time
import zipfile
from dataclasses import asdict, dataclass
from pathlib import Path, PurePosixPath
from typing import Callable

from .config import content_hash
from .game_updates import INTERVAL_SECONDS, PublicSteamBuildChecker, installed_build
from .journal import JournalStore, OperationJournal
from .payload_contract import CONTRACT_SCHEMA_VERSION, validate_contract
from .pterodactyl import PterodactylClient, PterodactylError, api_file_path
from .provider import ServerProvider

GAME_RELATIVE = "RSDragonwilds/Binaries/Linux/RSDragonwildsServer-Linux-Shipping"
LAUNCHER_MARKER = b"ZANIK_COMPAT_LAUNCHER=1"
BACKUP_MANIFEST = ".zanik-backup.json"
BACKUP_FAMILIES = {"payload", "game-update", "configuration", "uninstall", "cli-update", "migration"}


def safe_archive_name(name: str) -> str:
    normalized = name.replace("\\", "/")
    while normalized.startswith("./"):
        normalized = normalized[2:]
    path = PurePosixPath(normalized)
    if not normalized or path.is_absolute() or ".." in path.parts:
        raise ValueError(f"Unsafe archive path: {name}")
    return str(path)


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def validate_server_root(server_root: str) -> str:
    root = server_root.rstrip("/")
    path = PurePosixPath(root)
    if not root.startswith("/") or ".." in path.parts or root in {"", "/", "/home"}:
        raise ValueError("Server root must be a specific absolute path")
    if str(path) != root:
        raise ValueError("Server root must be normalized before use")
    return root


def is_exact_legacy_launcher(content: bytes, server_root: str) -> bool:
    """Recognize only the two-line launcher shipped by pre-0.1 Zanik."""
    expected = f'#!/bin/sh\nexec /bin/sh {server_root}/zanik/zanik-bootstrap.sh "$@"\n'.encode()
    return len(content) <= 4096 and content.replace(b"\r\n", b"\n") == expected


@dataclass(frozen=True)
class PayloadEntry:
    path: str
    size: int
    sha256: str
    mode: int


class ReleasePayload:
    def __init__(self, archive: Path):
        self.archive = archive
        self.entries: dict[str, PayloadEntry] = {}
        with zipfile.ZipFile(archive) as package:
            for item in package.infolist():
                if item.is_dir():
                    continue
                name = safe_archive_name(item.filename)
                data = package.read(item)
                mode = (item.external_attr >> 16) & 0o777 or 0o644
                self.entries[name] = PayloadEntry(name, len(data), sha256(data), mode)
        required = {"bin/libzanik.so", "bin/zanik-server", "scripts/compat-launcher.sh",
                    "scripts/verify-assets.sh", "scripts/check-game-update.sh",
                    "scripts/check-compatible-release.sh", "scripts/update-game.sh",
                    "scripts/apply-asset-delta.sh",
                    "config/zanik.conf.example", "compatibility/compatibility.tsv", "VERSION",
                    "payload-contract.json"}
        missing = required - set(self.entries)
        if missing:
            raise ValueError(f"Server payload is missing {', '.join(sorted(missing))}")
        try:
            self.contract = json.loads(self.read("payload-contract.json"))
            validate_contract(self.contract, self.entries.values())
        except (KeyError, json.JSONDecodeError, ValueError) as error:
            raise ValueError(f"Server payload contract is invalid: {error}") from error

    def read(self, name: str) -> bytes:
        with zipfile.ZipFile(self.archive) as package:
            return package.read(name)

    def manifest(self) -> dict:
        return {"schemaVersion": 2, "archive": self.archive.name, "archiveSha256": content_hash(self.archive.read_bytes()),
                "payloadContractSchemaVersion": self.contract["schemaVersion"],
                "payloadContractSha256": self.contract["contractSha256"],
                "files": [asdict(self.entries[name]) for name in sorted(self.entries)]}


class PterodactylProvider(ServerProvider):
    def __init__(self, client: PterodactylClient, journals: JournalStore, steam_checker=None):
        self.client = client
        self.journals = journals
        self.steam_checker = steam_checker or PublicSteamBuildChecker()

    def list_servers(self) -> list[dict]:
        return self.client.servers()

    def _status(self, message: str) -> None:
        callback = getattr(self.client, "status", None)
        if callback:
            callback(message)

    def preflight(self, server_id: str, operation: str, server: dict | None = None) -> None:
        required={
            "doctor": {"file.read", "file.read-content", "startup.read"},
            "files": {"file.read"},
            "install": {"control.start", "control.stop", "file.read", "file.read-content", "file.create", "file.update", "file.delete"},
            "replacement-install": {"control.start", "control.stop", "file.read", "file.read-content", "file.create", "file.update", "file.delete", "startup.read", "startup.update"},
            "game-update": {"control.start", "control.stop", "file.read", "file.read-content", "file.create", "file.update", "file.delete", "startup.read", "startup.update"},
            "assets": {"control.start", "control.stop", "file.read", "file.read-content", "file.create", "file.update", "file.delete"},
            "config": {"file.read", "file.read-content", "file.create", "file.update", "file.delete"},
        }.get(operation, set())
        details=server or self.client.server(server_id)
        permissions=set(details.get("_permissions", details.get("permissions", [])) or [])
        if details.get("server_owner") or not permissions:
            return
        missing=required-permissions
        if missing:
            raise RuntimeError("The panel account is missing required permissions: " + ", ".join(sorted(missing)))

    def _auto_update(self, server_id: str) -> dict:
        variable = self.client.startup_variable(server_id, "AUTO_UPDATE")
        if not variable:
            raise RuntimeError("This egg does not expose the required AUTO_UPDATE startup variable")
        if variable["value"] not in {"0", "1"}:
            raise RuntimeError("AUTO_UPDATE must contain the numeric value 0 or 1")
        return variable

    def _enforce_replacement_auto_update(self, server_id: str) -> dict:
        variable = self._auto_update(server_id)
        if not variable["editable"]:
            raise RuntimeError(
                "Replacement mode requires an editable AUTO_UPDATE variable. Ask the panel administrator to make it editable."
            )
        previous = variable["value"]
        if previous != "0":
            self.client.set_startup_variable(server_id, "AUTO_UPDATE", "0")
        verified = self._auto_update(server_id)
        if not verified["editable"] or verified["value"] != "0":
            raise RuntimeError("Manager could not set and verify AUTO_UPDATE=0; replacement installation was blocked")
        return {"variable": "AUTO_UPDATE", "previousValue": previous, "managedValue": "0",
                "verified": True, "managedByZanik": True}

    def _restore_managed_auto_update(self, server_id: str, metadata: dict | None) -> bool:
        if not metadata or not metadata.get("managedByZanik"):
            return False
        current = self._auto_update(server_id)
        if current["value"] != str(metadata.get("managedValue", "0")):
            return False
        previous = str(metadata.get("previousValue", "0"))
        if previous not in {"0", "1"}:
            return False
        self.client.set_startup_variable(server_id, "AUTO_UPDATE", previous)
        return self._auto_update(server_id)["value"] == previous

    def _wrapper_startup_metadata(self, server_id: str, adapter: dict,
                                  previous_install: dict | None) -> dict:
        """Describe and verify a wrapper variable without losing its pre-Zanik value on updates."""
        key = str(adapter.get("key", ""))
        managed = str(adapter.get("value", ""))
        current = str(adapter.get("previous", ""))
        if key not in {"SERVER_BINARY", "EXECUTABLE", "BINARY"} or not managed or len(managed) > 1024:
            raise RuntimeError("The detected wrapper startup adapter is invalid")
        prior = (previous_install or {}).get("startupAdapter") or {}
        previous = current
        has_prior = (prior.get("managedByZanik") and prior.get("variable") == key
                     and prior.get("managedValue") == managed)
        if has_prior:
            candidate = prior.get("previousValue")
            if isinstance(candidate, str) and len(candidate) <= 1024:
                previous = candidate
            else:
                has_prior = False
        if current == managed and not has_prior:
            # Older Manager versions changed this value without recording its
            # predecessor. Never invent a restoration target from the managed
            # value itself; uninstall must leave this case stopped for review.
            return {"variable": key, "managedValue": managed, "verified": True,
                    "managedByZanik": False, "reason": "previous-value-unavailable"}
        if current != managed:
            self.client.set_startup_variable(server_id, key, managed)
        verified = self.client.startup_variable(server_id, key)
        if not verified or not verified.get("editable") or str(verified.get("value", "")) != managed:
            raise RuntimeError(f"Manager could not set and verify {key} for wrapper mode")
        return {"variable": key, "previousValue": previous, "managedValue": managed,
                "verified": True, "managedByZanik": True}

    def _restore_managed_wrapper_startup(self, server_id: str, metadata: dict | None) -> str:
        """Restore a wrapper variable only while it still contains Zanik's managed value."""
        if not metadata or not metadata.get("managedByZanik"):
            return "unmanaged"
        key = str(metadata.get("variable", ""))
        managed = metadata.get("managedValue")
        previous = metadata.get("previousValue")
        if (key not in {"SERVER_BINARY", "EXECUTABLE", "BINARY"}
                or not isinstance(managed, str) or not managed or len(managed) > 1024
                or not isinstance(previous, str) or len(previous) > 1024):
            raise RuntimeError("Stored wrapper startup metadata is invalid; refusing to alter panel settings")
        current = self.client.startup_variable(server_id, key)
        if not current:
            raise RuntimeError(f"The managed {key} startup variable is no longer available")
        if str(current.get("value", "")) != managed:
            return "operator-managed"
        if not current.get("editable"):
            raise RuntimeError(f"The managed {key} startup variable is no longer editable")
        self.client.set_startup_variable(server_id, key, previous)
        verified = self.client.startup_variable(server_id, key)
        if not verified or str(verified.get("value", "")) != previous:
            raise RuntimeError(f"Manager could not restore the pre-Zanik {key} startup value")
        return "restored"

    def _path(self, root: str, relative: str) -> str:
        return root.rstrip("/") + "/" + relative.lstrip("/")

    def _mkdir(self, server_id: str, path: str) -> None:
        current = "/"
        for component in PurePosixPath(api_file_path(path)).parts:
            if component == "/":
                continue
            parent = current
            gateway_failures = 0
            while True:
                try:
                    self.client.create_folder(server_id, parent, component)
                    break
                except PterodactylError as error:
                    if error.status in {409, 422}:
                        break
                    if error.status == 522 or (error.status in {500, 502, 503, 504} and gateway_failures < 5):
                        if self.client.cancelled():
                            raise PterodactylError("Operation cancelled while waiting for the panel", error.status)
                        gateway_failures += 1
                        self._status(
                            f"The server node is temporarily unavailable (HTTP {error.status}). Retrying in 60 seconds…"
                        )
                        time.sleep(self.client.retry_seconds)
                        continue
                    raise
            current = str(PurePosixPath(parent) / component)

    def _entry(self, server_id: str, path: str) -> dict | None:
        posix = PurePosixPath(path)
        failures = 0
        while True:
            try:
                for item in self.client.list_files(server_id, str(posix.parent)):
                    if item.get("name") == posix.name:
                        return item
                return None
            except PterodactylError as error:
                if error.status == 404:
                    return None
                if error.status in {500, 502, 503, 504} and self._directory_definitely_missing(server_id, posix.parent):
                    return None
                if error.status == 522 or (error.status in {500, 502, 503, 504} and failures < 5):
                    failures += 1
                    self._status(
                        f"The server node is temporarily unavailable (HTTP {error.status}). Retrying in 60 seconds…"
                    )
                    time.sleep(self.client.retry_seconds)
                    continue
                raise

    def _directory_definitely_missing(self, server_id: str, directory: PurePosixPath) -> bool:
        """Disambiguate Wings versions that return 500 when listing an absent directory."""
        current = directory
        container = PurePosixPath("/home/container")
        while current != container and str(current) not in {"/", "."}:
            parent = current.parent
            try:
                rows = self.client.list_files(server_id, str(parent))
            except PterodactylError as error:
                if error.status in {500, 502, 503, 504}:
                    current = parent
                    continue
                return False
            return not any(row.get("name") == current.name for row in rows)
        return False

    def _exists(self, server_id: str, path: str) -> bool:
        return self._entry(server_id, path) is not None

    @staticmethod
    def _backup_parents(root: str) -> tuple[str, ...]:
        return (f"{root}/zanik-backups/manager", f"{root}/zanik-backups/game-update",
                f"{root}/zanik-data/update-history")

    def _write_backup_manifest(self, server_id: str, path: str, family: str, operation_id: str,
                               state: str, created_at: str | None = None,
                               managed_paths: list[str] | None = None, known_bytes: int | None = None) -> dict:
        if family not in BACKUP_FAMILIES or state not in {"preparing", "complete", "failed", "superseded"}:
            raise ValueError("Invalid managed backup metadata")
        existing = None
        try:
            existing = json.loads(self.client.read_file(server_id, f"{path}/{BACKUP_MANIFEST}"))
        except Exception:
            pass
        manifest = {
            "schemaVersion": 1,
            "owner": "Zanik Manager",
            "family": family,
            "operationId": operation_id,
            "createdAt": (existing or {}).get("createdAt") or created_at or dt.datetime.now(dt.timezone.utc).isoformat(),
            "updatedAt": dt.datetime.now(dt.timezone.utc).isoformat(),
            "state": state,
            "managedPaths": sorted(set(managed_paths or (existing or {}).get("managedPaths") or [])),
            "knownBytes": known_bytes if known_bytes is not None else (existing or {}).get("knownBytes"),
        }
        data = (json.dumps(manifest, sort_keys=True, separators=(",", ":")) + "\n").encode()
        self.client.write_file(server_id, f"{path}/{BACKUP_MANIFEST}", data)
        self.client.chmod(server_id, path, [(BACKUP_MANIFEST, 0o600)])
        return manifest

    @staticmethod
    def _legacy_backup_family(parent: str, name: str) -> str | None:
        timestamp = r"\d{8}T\d{6}Z"
        if parent.endswith("/zanik-backups/manager"):
            if re.fullmatch(timestamp + r"-config", name): return "configuration"
            if re.fullmatch(timestamp + r"-[0-9a-f]{8}-uninstall", name): return "uninstall"
            if re.fullmatch(timestamp + r"-[0-9a-f]{8}", name): return "payload"
        if parent.endswith("/zanik-backups/game-update") and re.fullmatch(timestamp + r"(?:-[0-9a-f]{8})?", name):
            return "game-update"
        if parent.endswith("/zanik-data/update-history"):
            if re.fullmatch(timestamp + r"-legacy-migration", name): return "migration"
            if re.fullmatch(timestamp, name): return "cli-update"
        return None

    def backup_storage_status(self, server_id: str, server_root: str) -> dict:
        """Inventory only shallow, recognized Zanik backup roots; never scan user data."""
        root = validate_server_root(server_root)
        records: list[dict] = []
        for parent in self._backup_parents(root):
            try:
                rows = self.client.list_files(server_id, parent)
            except PterodactylError as error:
                if error.status in {404, 500, 502, 503, 504}: continue
                raise
            for row in rows:
                if row.get("is_file", False): continue
                name = str(row.get("name", ""))
                path = f"{parent}/{name}"
                manifest = None
                try:
                    candidate = json.loads(self.client.read_file(server_id, f"{path}/{BACKUP_MANIFEST}"))
                    if (candidate.get("schemaVersion") == 1
                            and candidate.get("owner") in {"Zanik Manager", "Zanik shell lifecycle"}
                            and candidate.get("family") in BACKUP_FAMILIES):
                        manifest = candidate
                except Exception:
                    pass
                if manifest:
                    records.append({"path": path, "managed": True, "legacy": False,
                                    "family": manifest["family"], "state": manifest.get("state", "unknown"),
                                    "operationId": manifest.get("operationId"),
                                    "createdAt": manifest.get("createdAt", ""),
                                    "knownBytes": manifest.get("knownBytes")})
                else:
                    family = self._legacy_backup_family(parent, name)
                    if family:
                        records.append({"path": path, "managed": False, "legacy": True,
                                        "family": family, "state": "legacy-unverified",
                                        "createdAt": name[:16], "knownBytes": None})
        records.sort(key=lambda item: (item.get("family", ""), item.get("createdAt", "")), reverse=True)
        protected = [item for item in records if item.get("state") in {"preparing", "failed"}]
        return {"schemaVersion": 1, "retentionPerFamily": 1, "records": records,
                "managedCount": sum(bool(item["managed"]) for item in records),
                "legacyCount": sum(bool(item["legacy"]) for item in records),
                "protectedCount": len(protected),
                "excludedRoots": [f"{root}/RSDragonwilds/Saved", f"{root}/zanik-data",
                                  f"{root}/zanik-release", f"{root}/zanik-assets-incoming"]}

    @staticmethod
    def _cleanup_candidates(report: dict, include_legacy: bool = False) -> list[dict]:
        grouped: dict[str, list[dict]] = {}
        for item in report.get("records", []):
            if item.get("state") in {"preparing", "failed"}: continue
            if item.get("legacy") and not include_legacy: continue
            if item.get("managed") and item.get("state") not in {"complete", "superseded"}: continue
            grouped.setdefault(item.get("family", ""), []).append(item)
        result = []
        for items in grouped.values():
            items.sort(key=lambda item: item.get("createdAt", ""), reverse=True)
            result.extend(item for index, item in enumerate(items)
                          if item.get("state") == "superseded" or index > 0)
        return sorted(result, key=lambda item: item["path"])

    def cleanup_backup_retention(self, server_id: str, server_root: str,
                                 include_legacy: bool = False, requested: set[str] | None = None) -> dict:
        report = self.backup_storage_status(server_id, server_root)
        candidates = self._cleanup_candidates(report, include_legacy)
        if requested is not None:
            candidates = [item for item in candidates if item["path"] in requested]
        removed, warnings = [], []
        for item in candidates:
            path = PurePosixPath(item["path"])
            if str(path.parent) not in self._backup_parents(validate_server_root(server_root)):
                warnings.append(f"Refused unsafe cleanup target: {path}"); continue
            try:
                self.client.delete(server_id, str(path.parent), [path.name]); removed.append(str(path))
            except Exception as error:
                warnings.append(f"Could not remove {path}: {error}")
        remaining = self.backup_storage_status(server_id, server_root)
        self._publish_backup_status(server_id, server_root, remaining, warnings)
        return {"removed": removed, "warnings": warnings, "remaining": remaining}

    def _publish_backup_status(self, server_id: str, server_root: str, report: dict,
                               warnings: list[str] | None = None) -> None:
        root = validate_server_root(server_root)
        if not self._directory_exists(server_id, f"{root}/zanik/state"): return
        families: dict[str, int] = {}
        for item in report.get("records", []):
            if item.get("managed") and item.get("state") == "complete":
                families[item["family"]] = families.get(item["family"], 0) + 1
        value = {"schemaVersion": 1, "status": "healthy" if not warnings else "warning",
                 "retentionPerFamily": 1, "successfulByFamily": families,
                 "protectedOperations": report.get("protectedCount", 0),
                 "legacyCandidates": report.get("legacyCount", 0),
                 "cleanupWarnings": list(warnings or [])[:10],
                 "checkedAt": dt.datetime.now(dt.timezone.utc).isoformat()}
        self.client.write_file(server_id, f"{root}/zanik/state/backup-retention-status.json",
                               (json.dumps(value, separators=(",", ":")) + "\n").encode())

    def _finish_backup(self, profile_id: str, action: str, server_id: str, server_root: str,
                       path: str, family: str, operation_id: str, state: str) -> None:
        if not path: return
        self._write_backup_manifest(server_id, path, family, operation_id, state)
        if state != "complete": return
        completed_at = dt.datetime.now(dt.timezone.utc).isoformat()
        superseded = self.journals.supersede_failed(profile_id, action, completed_at)
        self._supersede_failed_remote_backups(server_id, server_root, family, path, superseded)
        self.cleanup_backup_retention(server_id, server_root)

    def _supersede_failed_remote_backups(self, server_id: str, server_root: str, family: str,
                                         current_path: str, operation_ids: set[str] | None = None) -> None:
        report = self.backup_storage_status(server_id, server_root)
        for item in report["records"]:
            if (item.get("managed") and item["path"] != current_path and item.get("family") == family
                    and item.get("state") == "failed"):
                self._write_backup_manifest(server_id, item["path"], item["family"],
                                            item.get("operationId") or "unknown", "superseded")
            elif item.get("managed") and item.get("operationId") in (operation_ids or set()):
                self._write_backup_manifest(server_id, item["path"], item["family"],
                                            item["operationId"], "superseded")

    def _upload_payload(self, server_id: str, stage: str, payload: ReleasePayload, journal: OperationJournal,
                        progress: Callable[[str, int], None] = lambda _message, _percent: None) -> None:
        directories = sorted({str(PurePosixPath(stage) / PurePosixPath(name).parent) for name in payload.entries})
        for directory in directories:
            self._mkdir(server_id, directory)
        modes: dict[str, list[tuple[str, int]]] = {}
        total = len(payload.entries)
        for index, (name, entry) in enumerate(payload.entries.items(), 1):
            message = f"Uploading and verifying {name}"
            percent = 25 + int(index * 35 / max(total, 1))
            try:
                progress(message, percent, {"detail": f"Server payload file {index} of {total}",
                                            "taskCurrent": index, "taskTotal": total})
            except TypeError:
                progress(message, percent)
            directory = str(PurePosixPath(stage) / PurePosixPath(name).parent)
            data = payload.read(name)
            self.client.upload(server_id, directory, PurePosixPath(name).name, data)
            remote = str(PurePosixPath(stage) / name)
            if sha256(self.client.read_file(server_id, remote)) != entry.sha256:
                raise RuntimeError(f"Remote verification failed for {name}")
            modes.setdefault(directory, []).append((PurePosixPath(name).name, entry.mode))
        for directory, files in modes.items():
            # Some Wings versions accept an array but apply only its last item.
            # Issue one bounded request per file so every packaged mode is honored.
            for file in files:
                self.client.chmod(server_id, directory, [file])
        journal.completed_steps.append("payload-uploaded-and-verified")
        self.journals.save(journal)

    @staticmethod
    def _payload_file_records(state: dict | None,
                              legacy_contract: dict | None = None) -> dict[str, dict]:
        """Return only validated package-owned records from prior installation metadata."""
        source = (state or {}).get("payloadFiles")
        if not isinstance(source, list):
            source = (legacy_contract or {}).get("files", [])
        records: dict[str, dict] = {}
        for item in source:
            if not isinstance(item, dict):
                continue
            try:
                name = safe_archive_name(str(item["path"]))
                digest = str(item["sha256"]).lower()
                size = int(item["size"])
            except (KeyError, TypeError, ValueError):
                continue
            if (len(digest) != 64 or any(character not in "0123456789abcdef" for character in digest)
                    or size < 0):
                continue
            # A prior inventory may name files no longer present in the new archive,
            # but it must never claim mutable data outside package-owned locations.
            if name in {"install-state.json", "admins.txt", "compatibility.tsv"} or name.startswith("assets/"):
                continue
            records[name] = {**item, "path": name, "sha256": digest, "size": size}
        return records

    def _payload_metadata(self, server_id: str, install: str,
                          paths: set[str]) -> dict[str, dict]:
        result: dict[str, dict] = {}
        parents = sorted({str(PurePosixPath(name).parent) for name in paths})
        for parent in parents:
            directory = install if parent == "." else f"{install}/{parent}"
            failures = 0
            while True:
                try:
                    rows = self.client.list_files(server_id, directory)
                    break
                except PterodactylError as error:
                    if error.status == 404:
                        rows = []
                        break
                    if (error.status in {500, 502, 503, 504}
                            and self._directory_definitely_missing(server_id, PurePosixPath(directory))):
                        rows = []
                        break
                    if error.status == 522 or (error.status in {500, 502, 503, 504} and failures < 5):
                        failures += 1
                        self._status(
                            f"The server node is temporarily unavailable (HTTP {error.status}). Retrying in 60 seconds…"
                        )
                        time.sleep(self.client.retry_seconds)
                        continue
                    raise
            for row in rows:
                if not row.get("is_file", False):
                    continue
                relative = str(PurePosixPath(parent) / str(row.get("name", "")))
                if relative.startswith("./"):
                    relative = relative[2:]
                if relative in paths:
                    result[relative] = row
        return result

    def _plan_payload_reconciliation(self, server_id: str, root: str, payload: ReleasePayload,
                                     previous_install: dict | None,
                                     progress: Callable[[str, int], None]) -> dict:
        install = f"{root}/zanik"
        legacy_contract = None
        has_verified_snapshot = isinstance((previous_install or {}).get("payloadFiles"), list)
        if not has_verified_snapshot:
            try:
                candidate = json.loads(self.client.read_file(server_id, f"{install}/payload-contract.json"))
                if candidate.get("schemaVersion") == CONTRACT_SCHEMA_VERSION:
                    legacy_contract = candidate
            except Exception:
                pass
        previous = self._payload_file_records(previous_install, legacy_contract)
        expected = payload.entries
        paths = set(expected) | set(previous)
        metadata = self._payload_metadata(server_id, install, paths)
        changed: list[str] = []
        unchanged: list[str] = []
        total = len(expected)
        for index, (name, entry) in enumerate(expected.items(), 1):
            try:
                progress("Comparing installed Zanik files", 12 + int(index * 10 / max(total, 1)),
                         {"detail": f"Checking server payload file {index} of {total}",
                          "taskCurrent": index, "taskTotal": total})
            except TypeError:
                progress("Comparing installed Zanik files", 12 + int(index * 10 / max(total, 1)))
            remote = metadata.get(name)
            if not remote or int(remote.get("size") or -1) != entry.size:
                changed.append(name)
                continue
            prior = previous.get(name)
            if prior and prior["sha256"] == entry.sha256 and prior["size"] == entry.size:
                prior_modified = str(prior.get("remoteModifiedAt") or "")
                remote_modified = str(remote.get("modified_at") or "")
                if has_verified_snapshot and prior_modified and remote_modified and prior_modified == remote_modified:
                    unchanged.append(name)
                    continue
            # Only uncertain files cross the network. Known changed files never do.
            if prior and prior["sha256"] != entry.sha256:
                changed.append(name)
                continue
            try:
                current = self.client.read_file(server_id, f"{install}/{name}")
            except Exception:
                changed.append(name)
                continue
            (unchanged if sha256(current) == entry.sha256 else changed).append(name)
        removed = sorted(name for name in previous if name not in expected and name in metadata)
        return {"changed": sorted(changed), "unchanged": sorted(unchanged), "removed": removed,
                "metadata": metadata, "previous": previous}

    def _payload_snapshot(self, server_id: str, root: str, payload: ReleasePayload) -> list[dict]:
        metadata = self._payload_metadata(server_id, f"{root}/zanik", set(payload.entries))
        result = []
        for name in sorted(payload.entries):
            entry = payload.entries[name]
            record = asdict(entry)
            modified = str(metadata.get(name, {}).get("modified_at") or "")
            if modified:
                record["remoteModifiedAt"] = modified
            result.append(record)
        return result

    def _apply_payload_delta(self, server_id: str, root: str, payload: ReleasePayload,
                             plan: dict, backup: str, journal: OperationJournal,
                             ensure_backup: Callable[[], None],
                             progress: Callable[[str, int], None]) -> None:
        changed = list(plan["changed"])
        removed = list(plan["removed"])
        existing = [name for name in changed if name in plan["metadata"]]
        restore = sorted(set(existing + removed))
        created = sorted(name for name in changed if name not in plan["metadata"])
        if restore:
            ensure_backup()
            for parent in sorted({str(PurePosixPath(name).parent) for name in restore}):
                self._mkdir(server_id, f"{backup}/zanik/{parent}")
            journal.rollback.append({"kind": "restore-payload-delta", "backup": f"{backup}/zanik",
                                     "restore": restore, "remove": created})
            self.journals.save(journal)
            for name in restore:
                self.client.rename(server_id, root, [(f"zanik/{name}",
                                                       f"zanik-backups/manager/{PurePosixPath(backup).name}/zanik/{name}")])
        elif created:
            journal.rollback.append({"kind": "restore-payload-delta", "backup": f"{backup}/zanik",
                                     "restore": [], "remove": created})
            self.journals.save(journal)
        modes: dict[str, list[tuple[str, int]]] = {}
        total = len(changed)
        for index, name in enumerate(changed, 1):
            entry = payload.entries[name]
            directory = str(PurePosixPath(f"{root}/zanik") / PurePosixPath(name).parent)
            self._mkdir(server_id, directory)
            try:
                progress(f"Uploading and verifying {name}", 30 + int(index * 30 / max(total, 1)),
                         {"detail": f"Changed server payload file {index} of {total}",
                          "taskCurrent": index, "taskTotal": total})
            except TypeError:
                progress(f"Uploading and verifying {name}", 30 + int(index * 30 / max(total, 1)))
            data = payload.read(name)
            self.client.upload(server_id, directory, PurePosixPath(name).name, data)
            if sha256(self.client.read_file(server_id, f"{root}/zanik/{name}")) != entry.sha256:
                raise RuntimeError(f"Remote verification failed for {name}")
            modes.setdefault(directory, []).append((PurePosixPath(name).name, entry.mode))
        for directory, files in modes.items():
            for file in files:
                self.client.chmod(server_id, directory, [file])
        journal.completed_steps.append("payload-delta-uploaded-and-verified")
        self.journals.save(journal)

    def _wait_offline(self, server_id: str, timeout: float = 120.0) -> None:
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            state = self.client.resources(server_id).get("current_state", "")
            if state == "offline":
                return
            time.sleep(2)
        raise RuntimeError("Server did not stop before the operation timed out")

    def wait_for_restart(self, server_id: str, timeout: float = 240.0,
                         previous_uptime: int | None = None) -> dict:
        """Wait for a requested restart without assuming the first running state is the new process."""
        deadline = time.monotonic() + timeout
        transition_observed = False
        time.sleep(2)
        while time.monotonic() < deadline:
            resources = self.client.resources(server_id)
            state = resources.get("current_state", "")
            uptime = int(resources.get("resources", {}).get("uptime") or 0)
            if state != "running":
                transition_observed = True
            elif transition_observed or (previous_uptime is not None and uptime < previous_uptime):
                return {"running": True, "transitionObserved": transition_observed,
                        "uptimeResetObserved": previous_uptime is not None and uptime < previous_uptime}
            time.sleep(2)
        state = self.client.resources(server_id).get("current_state", "")
        return {"running": state == "running", "transitionObserved": transition_observed,
                "warning": "The server restart could not be observed before the verification timeout."}

    def replacement_launcher_status(self, server_id: str, server_root: str) -> dict:
        """Verify the small compatibility launcher without downloading a displaced game executable."""
        root = validate_server_root(server_root)
        try:
            state = json.loads(self.client.read_file(server_id, f"{root}/zanik/install-state.json"))
        except Exception as error:
            return {"applicable": False, "healthy": False, "reason": "installation-state-unavailable",
                    "detail": str(error)}
        if state.get("mode") != "replace-executable":
            return {"applicable": False, "healthy": True, "mode": state.get("mode", "unknown")}
        game = f"{root}/{GAME_RELATIVE}"
        entry = self._entry(server_id, game)
        if not entry or not entry.get("is_file", False):
            return {"applicable": True, "healthy": False, "mode": "replace-executable",
                    "reason": "launcher-missing"}
        size = int(entry.get("size") or 0)
        if size > 1024 * 1024:
            return {"applicable": True, "healthy": False, "mode": "replace-executable",
                    "reason": "official-executable-replaced-launcher", "size": size}
        try:
            content = self.client.read_file(server_id, game)
        except Exception as error:
            return {"applicable": True, "healthy": False, "mode": "replace-executable",
                    "reason": "launcher-unreadable", "detail": str(error)}
        return {"applicable": True, "healthy": LAUNCHER_MARKER in content,
                "mode": "replace-executable",
                "reason": "managed-launcher" if LAUNCHER_MARKER in content else "unknown-small-executable",
                "size": size}

    def verify_launcher_after_restart(self, server_id: str, server_root: str,
                                      timeout: float = 240.0,
                                      previous_uptime: int | None = None) -> dict:
        restart = self.wait_for_restart(server_id, timeout, previous_uptime)
        launcher = self.replacement_launcher_status(server_id, server_root)
        launcher["restart"] = restart
        return launcher

    def install(self, profile_id: str, server_id: str, server_root: str, payload: ReleasePayload,
                mode: str, acknowledge_replacement: bool = False, stop_server: bool = True,
                wrapper_adapter: dict | None = None,
                progress: Callable[[str, int], None] = lambda _message, _percent: None) -> dict:
        if mode not in {"wrapper", "replace-executable"}:
            raise ValueError("Installation mode must be wrapper or replace-executable")
        if mode == "replace-executable" and not acknowledge_replacement:
            raise ValueError("Executable replacement requires explicit acknowledgement")
        journal = OperationJournal.create(profile_id, "install")
        self.journals.save(journal)
        root = validate_server_root(server_root)
        stage = f"{root}/.zanik-manager/staging/{journal.operation_id}/release"
        timestamp = dt.datetime.now(dt.timezone.utc).strftime("%Y%m%dT%H%M%SZ")
        backup = f"{root}/zanik-backups/manager/{timestamp}-{journal.operation_id[:8]}"
        backup_created = False
        def ensure_backup() -> None:
            nonlocal backup_created
            if backup_created: return
            self._mkdir(server_id, backup)
            self._write_backup_manifest(server_id, backup, "payload", journal.operation_id, "preparing")
            backup_created = True
        previous_install = None
        previous_install_raw = None
        try:
            progress("Checking panel permissions", 5)
            self.preflight(server_id, "replacement-install" if mode == "replace-executable" else "install")
            try:
                previous_install_raw = self.client.read_file(server_id, f"{root}/zanik/install-state.json")
                previous_install = json.loads(previous_install_raw)
            except Exception:
                previous_install = None
            existing_install = self._directory_exists(server_id, f"{root}/zanik")
            auto_update = None
            if mode == "replace-executable":
                progress("Disabling and verifying Pterodactyl automatic updates", 7)
                enforced = self._enforce_replacement_auto_update(server_id)
                prior_metadata = (previous_install or {}).get("autoUpdate") or {}
                if prior_metadata.get("managedByZanik"):
                    enforced["previousValue"] = str(prior_metadata.get("previousValue", enforced["previousValue"]))
                auto_update = enforced
                if enforced["previousValue"] != "0" and not prior_metadata.get("managedByZanik"):
                    journal.rollback.append({"kind": "restore-startup-variable", "key": "AUTO_UPDATE",
                                             "managedValue": "0", "value": enforced["previousValue"]})
                    self.journals.save(journal)
            else:
                try:
                    variable = self._auto_update(server_id)
                    auto_update = {"variable": "AUTO_UPDATE", "currentValue": variable["value"],
                                   "managedByZanik": False, "verified": True}
                except Exception:
                    auto_update = {"variable": "AUTO_UPDATE", "managedByZanik": False, "verified": False}
            incremental = previous_install is not None
            plan = (self._plan_payload_reconciliation(server_id, root, payload, previous_install, progress)
                    if incremental else {"changed": sorted(payload.entries), "unchanged": [],
                                         "removed": [], "metadata": {}, "previous": {}})
            game = f"{root}/{GAME_RELATIVE}"
            launcher = payload.read("scripts/compat-launcher.sh") if mode == "replace-executable" else None
            current_launcher = None
            if mode == "replace-executable":
                game_entry = self._entry(server_id, game)
                if game_entry and game_entry.get("is_file", False) and int(game_entry.get("size") or 0) <= 1024 * 1024:
                    current_launcher = self.client.read_file(server_id, game)
            launcher_change = mode == "replace-executable" and current_launcher != launcher
            payload_change = bool(plan["changed"] or plan["removed"])
            mutation_required = payload_change or launcher_change or not incremental
            was_running = self.client.resources(server_id).get("current_state") != "offline"
            server_stopped = False
            if stop_server and mutation_required and was_running:
                progress("Stopping the game server", 24)
                self.client.power(server_id, "stop")
                self._wait_offline(server_id)
                server_stopped = True
                journal.completed_steps.append("server-stopped")
                self.journals.save(journal)
            remote_journal = f"{root}/.zanik-manager/transactions/{journal.operation_id}.json"
            self._mkdir(server_id, str(PurePosixPath(remote_journal).parent))
            self.client.write_file(server_id, remote_journal, json.dumps(asdict(journal), sort_keys=True).encode())

            if incremental and mutation_required and previous_install_raw is not None:
                ensure_backup()
                self.client.write_file(server_id, f"{backup}/install-state.json", previous_install_raw)
                journal.rollback.append({"kind": "restore-managed-file",
                                         "from": f"{backup}/install-state.json",
                                         "to": f"{root}/zanik/install-state.json",
                                         "sha256": sha256(previous_install_raw), "mode": 0o600})
                self.journals.save(journal)

            if incremental:
                if payload_change:
                    try:
                        progress("Applying changed Zanik files", 28,
                                 {"detail": (f"{len(plan['changed'])} changed or missing · "
                                             f"{len(plan['removed'])} retired · {len(plan['unchanged'])} unchanged")})
                    except TypeError:
                        progress("Applying changed Zanik files", 28)
                    self._apply_payload_delta(server_id, root, payload, plan, backup, journal,
                                              ensure_backup, progress)
                else:
                    try:
                        progress("Server payload already verified", 60,
                                 {"detail": f"Reused {len(plan['unchanged'])} unchanged files · no upload required"})
                    except TypeError:
                        progress("Server payload already verified", 60)
            else:
                progress("Preparing verified staging", 18)
                self._mkdir(server_id, stage)
                self._upload_payload(server_id, stage, payload, journal, progress)
                progress("Activating the verified Zanik payload", 68)
                if existing_install:
                    ensure_backup()
                    self.client.rename(server_id, root,
                                       [("zanik", f"zanik-backups/manager/{PurePosixPath(backup).name}/zanik")])
                    journal.rollback.append({"from": f"{backup}/zanik", "to": f"{root}/zanik"})
                self.client.rename(server_id, root, [(str(PurePosixPath(stage).relative_to(root)), "zanik")])
                journal.rollback.append({"from": f"{root}/zanik", "to": stage})
                journal.completed_steps.append("payload-activated")
                self.journals.save(journal)

                if existing_install:
                    old = f"{backup}/zanik"
                    for relative in ("config/zanik.conf", "admins.txt", "assets"):
                        source = f"{old}/{relative}"
                        present = (self._directory_exists(server_id, source) if relative == "assets"
                                   else self._exists(server_id, source))
                        if present:
                            destination = f"zanik/{relative}"
                            self.client.rename(server_id, root,
                                               [(str(PurePosixPath(source).relative_to(root)), destination)])
                            journal.rollback.append({"kind": "move", "from": f"{root}/{destination}",
                                                     "to": source})
                            self.journals.save(journal)

            journal.state = "activating"
            progress("Preserving configuration and persistent files", 76)
            if not self._exists(server_id, f"{root}/zanik/config/zanik.conf"):
                example = self.client.read_file(server_id, f"{root}/zanik/config/zanik.conf.example")
                self.client.write_file(server_id, f"{root}/zanik/config/zanik.conf", example)
            if not self._exists(server_id, f"{root}/zanik/admins.txt"):
                self.client.write_file(server_id, f"{root}/zanik/admins.txt", b"")
            compatibility = self.client.read_file(server_id, f"{root}/zanik/compatibility/compatibility.tsv")
            try:
                runtime_compatibility = self.client.read_file(server_id, f"{root}/zanik/compatibility.tsv")
            except Exception:
                runtime_compatibility = None
            if runtime_compatibility != compatibility:
                self.client.write_file(server_id, f"{root}/zanik/compatibility.tsv", compatibility)
            if sha256(self.client.read_file(server_id, f"{root}/zanik/compatibility.tsv")) != sha256(compatibility):
                raise RuntimeError("Installed runtime compatibility manifest did not verify")
            self._mkdir(server_id, f"{root}/zanik-data")
            if not self._exists(server_id, f"{root}/zanik-data/console-admin.key"):
                self.client.write_file(server_id, f"{root}/zanik-data/console-admin.key", secrets.token_hex(32).encode())
            self.client.chmod(server_id, root, [("zanik/config/zanik.conf", 0o600), ("zanik/admins.txt", 0o600),
                                                ("zanik/compatibility.tsv", 0o644),
                                                ("zanik-data/console-admin.key", 0o600)])

            progress("Checking the server launcher", 84)
            official = game
            official_data = None
            official_sha = None
            if mode == "replace-executable":
                original = game + ".zanik-original"
                current = current_launcher if current_launcher is not None else self.client.download_file(server_id, game)
                if current == launcher:
                    pass
                elif LAUNCHER_MARKER in current:
                    ensure_backup()
                    self.client.write_file(server_id, f"{backup}/managed-launcher", current)
                    journal.rollback.append({"kind": "restore-file", "from": f"{backup}/managed-launcher",
                                             "to": game, "sha256": sha256(current), "mode": 0o755})
                    self.journals.save(journal)
                elif current.startswith(b"\x7fELF"):
                    if self._exists(server_id, original):
                        ensure_backup()
                        self.client.rename(server_id, root, [(str(PurePosixPath(original).relative_to(root)),
                                                              f"zanik-backups/manager/{PurePosixPath(backup).name}/official-before-steam-update")])
                    self.client.rename(server_id, root, [(str(PurePosixPath(game).relative_to(root)),
                                                          str(PurePosixPath(original).relative_to(root)))])
                    journal.rollback.append({"kind": "restore-official", "from": original, "to": game})
                    self.journals.save(journal)
                elif is_exact_legacy_launcher(current, root):
                    ensure_backup()
                    self.client.write_file(server_id, f"{backup}/managed-launcher", current)
                    journal.rollback.append({"kind": "restore-file", "from": f"{backup}/managed-launcher",
                                             "to": game, "sha256": sha256(current), "mode": 0o755})
                    self.journals.save(journal)
                else:
                    raise RuntimeError("Unknown executable replacement state; refusing to guess")
                if current != launcher:
                    self.client.write_file(server_id, game, launcher)
                    self.client.chmod(server_id, str(PurePosixPath(game).parent), [(PurePosixPath(game).name, 0o755)])
                official = original
            prior_sha = str((previous_install or {}).get("officialSha256") or "").lower()
            if ((previous_install or {}).get("officialExecutable") == official and len(prior_sha) == 64
                    and all(character in "0123456789abcdef" for character in prior_sha)
                    and self._exists(server_id, official)):
                official_sha = prior_sha
            if official_sha is None:
                official_data = self.client.download_file(server_id, official)
                if not official_data.startswith(b"\x7fELF"):
                    raise RuntimeError("The preserved official executable is not a verified ELF file")
                official_sha = sha256(official_data)
            version = self.client.read_file(server_id, f"{root}/zanik/VERSION").decode().strip()
            startup_adapter = None
            if mode == "wrapper" and wrapper_adapter:
                progress("Configuring and verifying the Pterodactyl startup command", 88)
                startup_adapter = self._wrapper_startup_metadata(server_id, wrapper_adapter, previous_install)
            progress("Writing and verifying installation metadata", 90)
            state = {"schemaVersion": 4, "version": version, "mode": mode, "serverRoot": root,
                     "officialExecutable": official, "officialSha256": official_sha,
                     "librarySha256": payload.entries["bin/libzanik.so"].sha256,
                     "analyzerSha256": payload.entries["bin/zanik-compat-analyzer"].sha256,
                     "launcherSha256": payload.entries["bin/zanik-server"].sha256,
                     "payloadContractSchemaVersion": payload.contract["schemaVersion"],
                     "payloadContractSha256": payload.contract["contractSha256"],
                     "payloadFiles": self._payload_snapshot(server_id, root, payload),
                     "autoUpdate": auto_update, "startupAdapter": startup_adapter,
                     "installedAt": dt.datetime.now(dt.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")}
            self.client.write_file(server_id, f"{root}/zanik/install-state.json", (json.dumps(state, separators=(",", ":")) + "\n").encode())
            self.client.chmod(server_id, f"{root}/zanik", [("install-state.json", 0o600)])
            startup_action_required = mode == "wrapper" and not wrapper_adapter
            journal.state = "complete"; journal.completed_steps.append("installation-verified")
            if startup_adapter:
                journal.completed_steps.append("wrapper-startup-configured")
            self.journals.save(journal)
            self.client.write_file(server_id, remote_journal, json.dumps(asdict(journal), sort_keys=True).encode())
            if server_stopped and not startup_action_required:
                progress("Starting the game server", 97)
                self.client.power(server_id, "start")
            if backup_created:
                self._finish_backup(profile_id, "install", server_id, root, backup, "payload",
                                    journal.operation_id, "complete")
            try:
                self.client.delete(server_id, f"{root}/.zanik-manager/staging", [journal.operation_id])
                self.client.delete(server_id, f"{root}/.zanik-manager/transactions", [f"{journal.operation_id}.json"])
            except Exception:
                pass
            self.journals.prune()
            progress("Installation complete", 100)
            return {"installation": state, "backup": backup if backup_created else None,
                    "operationId": journal.operation_id, "startupActionRequired": startup_action_required,
                    "reconciliation": {"uploaded": len(plan["changed"]), "removed": len(plan["removed"]),
                                       "reused": len(plan["unchanged"]), "serverRestarted": server_stopped}}
        except Exception as error:
            journal.state = "failed"; journal.error = str(error); self.journals.save(journal)
            if backup_created:
                try:
                    self._finish_backup(profile_id, "install", server_id, root, backup, "payload",
                                        journal.operation_id, "failed")
                except Exception:
                    pass
            raise

    def rollback(self, journal: OperationJournal, server_id: str, server_root: str) -> None:
        root=server_root.rstrip("/")
        if self.client.resources(server_id).get("current_state") != "offline":
            self.client.power(server_id,"stop"); self._wait_offline(server_id)
        for action in reversed(journal.rollback):
            if action.get("kind") == "restore-startup-variable":
                current = self._auto_update(server_id)
                if current["value"] == action.get("managedValue"):
                    self.client.set_startup_variable(server_id, action["key"], action["value"])
                continue
            if action.get("kind") == "restore-payload-delta":
                for relative in action.get("remove", []):
                    live = f"{root}/zanik/{safe_archive_name(relative)}"
                    if self._exists(server_id, live):
                        self.client.delete(server_id, str(PurePosixPath(live).parent),
                                           [PurePosixPath(live).name])
                backup_root = str(action.get("backup", ""))
                for relative in action.get("restore", []):
                    relative = safe_archive_name(relative)
                    source = f"{backup_root}/{relative}"
                    if not self._exists(server_id, source):
                        continue
                    live = f"{root}/zanik/{relative}"
                    if self._exists(server_id, live):
                        self.client.delete(server_id, str(PurePosixPath(live).parent),
                                           [PurePosixPath(live).name])
                    self._mkdir(server_id, str(PurePosixPath(live).parent))
                    self.client.rename(server_id, root,
                                       [(str(PurePosixPath(source).relative_to(root)),
                                         str(PurePosixPath(live).relative_to(root)))])
                continue
            if action.get("kind") == "restore-managed-file":
                source = str(action["from"]); destination = str(action["to"])
                data = self.client.read_file(server_id, source)
                if sha256(data) != action["sha256"]:
                    raise RuntimeError("Rollback managed-file backup hash mismatch")
                self._mkdir(server_id, str(PurePosixPath(destination).parent))
                self.client.write_file(server_id, destination, data)
                self.client.chmod(server_id, str(PurePosixPath(destination).parent),
                                  [(PurePosixPath(destination).name, action.get("mode", 0o600))])
                continue
            if action.get("kind") == "restore-game-launcher":
                source=action["from"]; destination=action["to"]
                current=self.client.read_file(server_id,destination) if self._exists(server_id,destination) else b""
                if current.startswith(b"\x7fELF"):
                    interrupted=f"{action['backup']}/official-interrupted"
                    self.client.rename(server_id,root,[(str(PurePosixPath(destination).relative_to(root)),
                                                       str(PurePosixPath(interrupted).relative_to(root)))])
                elif current and LAUNCHER_MARKER not in current and sha256(current)!=action.get("updaterSha256"):
                    raise RuntimeError("Rollback found an unknown executable at the game path")
                data=self.client.read_file(server_id,source)
                if sha256(data)!=action["sha256"] or LAUNCHER_MARKER not in data:
                    raise RuntimeError("Rollback launcher backup did not verify")
                self.client.write_file(server_id,destination,data)
                self.client.chmod(server_id,str(PurePosixPath(destination).parent),
                                  [(PurePosixPath(destination).name,0o755)])
                continue
            source=action["from"]; destination=action["to"]
            if action.get("kind") == "restore-file":
                data=self.client.read_file(server_id,source)
                if sha256(data) != action["sha256"]:
                    raise RuntimeError("Rollback backup hash mismatch")
                if self._exists(server_id,destination):
                    current=self.client.read_file(server_id,destination)
                    if LAUNCHER_MARKER not in current:
                        raise RuntimeError("Rollback found an unmanaged executable at the restoration target")
                self.client.write_file(server_id,destination,data)
                self.client.chmod(server_id,str(PurePosixPath(destination).parent),
                                  [(PurePosixPath(destination).name,action.get("mode",0o600))])
                continue
            if action.get("kind") == "restore-official":
                if self._exists(server_id,destination):
                    current=self.client.read_file(server_id,destination)
                    if LAUNCHER_MARKER not in current:
                        raise RuntimeError("Rollback found an unmanaged executable at the restoration target")
                    self.client.delete(server_id,str(PurePosixPath(destination).parent),[PurePosixPath(destination).name])
            exists=self._exists(server_id,source) or self._directory_exists(server_id,source)
            if exists:
                self.client.rename(server_id,root,[(str(PurePosixPath(source).relative_to(root)),
                                                    str(PurePosixPath(destination).relative_to(root)))])
        journal.state="rolled-back"; journal.error=""; journal.completed_steps.append("rollback-complete")
        self.journals.save(journal)
        self.client.power(server_id,"start")

    def _directory_exists(self, server_id: str, path: str) -> bool:
        entry = self._entry(server_id, path)
        return entry is not None and not entry.get("is_file", False)

    def diagnose(self, server_id: str, server_root: str, deep: bool = False) -> dict:
        self.preflight(server_id, "doctor")
        root = server_root.rstrip("/")
        result = {"healthy": True, "checks": {}}
        try:
            result["backupRetention"] = self.backup_storage_status(server_id, root)
        except Exception as error:
            result["backupRetention"] = {"schemaVersion": 1, "status": "unavailable", "error": str(error)}
        try:
            state_raw = self.client.read_file(server_id, f"{root}/zanik/install-state.json")
            state = json.loads(state_raw)
            result["installation"] = state
        except Exception as error:
            result.update({"healthy": False, "checks": {"installation": str(error)}})
            return result
        replacement = self.replacement_launcher_status(server_id, root)
        result["launcher"] = replacement
        if replacement.get("applicable"):
            result["checks"]["replacementLauncher"] = bool(replacement.get("healthy"))
            result["healthy"] &= bool(replacement.get("healthy"))
        try:
            variable = self._auto_update(server_id)
            expected = "0" if state.get("mode") == "replace-executable" else None
            result["autoUpdate"] = {"available": True, "value": variable["value"],
                                    "editable": variable["editable"], "expected": expected,
                                    "healthy": expected is None or (variable["editable"] and variable["value"] == expected)}
            if expected is not None:
                result["healthy"] &= result["autoUpdate"]["healthy"]
        except Exception as error:
            result["autoUpdate"] = {"available": False, "healthy": state.get("mode") != "replace-executable",
                                    "error": str(error)}
            if state.get("mode") == "replace-executable":
                result["healthy"] = False
        targets = {"library": ("librarySha256", "bin/libzanik.so"), "analyzer": ("analyzerSha256", "bin/zanik-compat-analyzer"),
                   "launcher": ("launcherSha256", "bin/zanik-server"), "official": ("officialSha256", None)}
        for name, (field, relative) in targets.items():
            path = state["officialExecutable"] if relative is None else f"{root}/zanik/{relative}"
            try:
                reader = self.client.download_file if name == "official" else self.client.read_file
                valid = sha256(reader(server_id, path)) == state.get(field)
            except Exception:
                valid = False
            result["checks"][name] = valid; result["healthy"] &= valid
        try:
            config = self.client.read_file(server_id, f"{root}/zanik/config/zanik.conf")
            result["checks"]["configSha256"] = sha256(config)
        except Exception:
            result["checks"]["configSha256"] = None; result["healthy"] = False
        try:
            manifest = json.loads(self.client.read_file(server_id, f"{root}/zanik/assets/current/asset-manifest.json"))
            compatibility = manifest.get("compatibility", manifest.get("compatible"))
            result["assets"] = {"installed": True,
                                "compatible": compatibility not in {False, "incompatible"},
                                "compatibility": compatibility,
                                "gameBuild": manifest.get("gameBuild") or manifest.get("clientBuild"),
                                "extractionStage": manifest.get("extractionStage"),
                                "buildingIconSources": manifest.get("buildingIconSources", {}),
                                "recipeRouteCoverage": manifest.get("recipeRouteCoverage", {}),
                                "cleanupWarnings": manifest.get("cleanupWarnings", [])}
            result["healthy"] &= result["assets"]["compatible"]
            if deep:
                try:
                    audit = self._server_asset_audit(server_id, root, manifest)
                    result["assets"]["deepVerification"] = audit
                    result["assets"]["mismatches"] = [] if audit.get("status") == "valid" else ["server-side asset audit failed"]
                except Exception as error:
                    # Compatibility fallback for older launchers or panels that
                    # cannot invoke the bounded server-side verifier.
                    mismatches = []
                    for entry in manifest.get("files", []):
                        try:
                            actual = sha256(self.client.download_file(server_id, f"{root}/zanik/assets/current/{entry['path']}"))
                            if actual != entry["sha256"]:
                                mismatches.append(entry["path"])
                        except Exception:
                            mismatches.append(entry.get("path", "unknown"))
                    result["assets"]["deepVerification"] = {"method": "per-file-api-fallback", "serverAuditError": str(error)}
                    result["assets"]["mismatches"] = mismatches
                result["healthy"] &= not result["assets"]["mismatches"]
        except Exception:
            result["assets"] = {"installed": False}
        try:
            result["gameUpdate"] = self._read_game_update_report(server_id, root)
        except Exception:
            result["gameUpdate"] = {"schemaVersion": 1, "status": "unchecked"}
        return result

    def installation_status(self, server_id: str, server_root: str, expected_contract: dict | None = None,
                            legacy_contracts: dict | None = None) -> dict:
        """Return lightweight recognized installation state without running Doctor."""
        root = validate_server_root(server_root)
        state = self._zanik_state(server_id, root)
        if not state["zanikInstalled"]:
            return {**state, "installed": False, "version": None, "mode": None}
        result = {**state, "installed": True, "version": None, "mode": None, "repairRequired": False,
                  "payloadCompatibility": "unknown"}
        try:
            install = json.loads(self.client.read_file(server_id, f"{root}/zanik/install-state.json"))
            version = str(install.get("version", "")).strip()
            mode = install.get("mode")
            if not version or mode not in {"wrapper", "replace-executable"}:
                raise ValueError("installation metadata is incomplete")
            result.update({"version": version, "mode": mode, "installation": install})
            try:
                variable = self._auto_update(server_id)
                expected_auto = "0" if mode == "replace-executable" else None
                result["autoUpdate"] = {"available": True, "value": variable["value"],
                                        "editable": variable["editable"], "expected": expected_auto,
                                        "healthy": expected_auto is None or (variable["editable"] and variable["value"] == expected_auto)}
            except Exception as auto_error:
                result["autoUpdate"] = {"available": False, "healthy": mode != "replace-executable",
                                        "error": str(auto_error)}
            installed_contract = str(install.get("payloadContractSha256", "")).lower()
            installed_schema = install.get("payloadContractSchemaVersion")
            if (installed_schema != CONTRACT_SCHEMA_VERSION or len(installed_contract) != 64 or
                    any(character not in "0123456789abcdef" for character in installed_contract)):
                legacy = (legacy_contracts or {}).get(version, {})
                critical = legacy.get("criticalHashes", {})
                fields = {"bin/libzanik.so": "librarySha256", "bin/zanik-compat-analyzer": "analyzerSha256",
                          "bin/zanik-server": "launcherSha256"}
                if (legacy.get("contractSchemaVersion") == CONTRACT_SCHEMA_VERSION and
                        len(str(legacy.get("contractSha256", ""))) == 64 and critical and
                        all(install.get(field) == critical.get(path) for path, field in fields.items())):
                    installed_contract = legacy["contractSha256"]
                    installed_schema = CONTRACT_SCHEMA_VERSION
                    result["legacyContractRecognized"] = True
            result["payloadContractSchemaVersion"] = installed_schema
            result["payloadContractSha256"] = installed_contract or None
            if expected_contract:
                expected_digest = expected_contract.get("contractSha256")
                expected_schema = expected_contract.get("schemaVersion")
                if installed_schema == expected_schema == CONTRACT_SCHEMA_VERSION and installed_contract:
                    result["payloadCompatibility"] = "current" if installed_contract == expected_digest else "update-available"
                result["expectedPayloadContractSha256"] = expected_digest
        except Exception as error:
            result.update({"repairRequired": True, "detail": str(error)})
            try:
                result["version"] = self.client.read_file(server_id, f"{root}/zanik/VERSION").decode().strip() or None
            except Exception:
                pass
        return result

    def asset_contract_status(self, server_id: str, server_root: str, contract: dict) -> dict:
        """Validate active asset metadata and a bounded critical subset."""
        root = validate_server_root(server_root)
        manifest_path = f"{root}/zanik/assets/current/asset-manifest.json"
        try:
            manifest = json.loads(self.client.read_file(server_id, manifest_path))
        except Exception as error:
            return {"valid": False, "installed": False,
                    "reasons": ["No readable active asset manifest was found."], "detail": str(error)}
        reasons = []
        compatibility = manifest.get("compatibility", manifest.get("compatible"))
        if compatibility in {False, "incompatible"}:
            reasons.append("The active bundle is marked incompatible.")
        if manifest.get("extractionStage") != "validated-and-packaged":
            reasons.append("The active bundle did not complete validation and packaging.")
        if manifest.get("missingRequired"):
            reasons.append("The active bundle reports missing required assets.")
        if manifest.get("extractionRecipeVersion") != contract.get("extractionRecipeVersion"):
            reasons.append("The active bundle uses a different extraction recipe version.")
        if (contract.get("extractionRecipeSha256")
                and manifest.get("extractionRecipeSha256") != contract.get("extractionRecipeSha256")):
            reasons.append("The active bundle was generated from different extraction rules.")
        profile_version = manifest.get("compatibilityProfile", {}).get("version")
        if profile_version != contract.get("compatibilityProfileVersion"):
            reasons.append("The active bundle uses a different reviewed compatibility profile.")
        entries = manifest.get("files", [])
        if not isinstance(entries, list) or manifest.get("generatedFileCount") != len(entries):
            reasons.append("The active bundle manifest file count is inconsistent.")
            entries = []
        by_path = {entry.get("path"): entry for entry in entries if isinstance(entry, dict)}
        for relative in contract.get("criticalFiles", []):
            entry = by_path.get(relative)
            if not entry or not isinstance(entry.get("sha256"), str):
                reasons.append(f"The active bundle manifest is missing required file {relative}.")
                continue
            try:
                actual = sha256(self.client.download_file(server_id, f"{root}/zanik/assets/current/{relative}"))
                if actual != entry["sha256"]:
                    reasons.append(f"The active critical asset failed verification: {relative}.")
            except Exception:
                reasons.append(f"The active critical asset is unavailable: {relative}.")
        return {"valid": not reasons, "installed": True, "reasons": reasons,
                "gameBuild": manifest.get("gameBuild") or manifest.get("clientBuild"),
                "extractionRecipeVersion": manifest.get("extractionRecipeVersion"),
                "extractionRecipeSha256": manifest.get("extractionRecipeSha256"),
                "compatibilityProfileVersion": profile_version}

    def _read_game_update_report(self, server_id: str, root: str) -> dict:
        raw = self.client.read_file(server_id, f"{root}/zanik/state/game-update-status.json")
        report = json.loads(raw.decode("utf-8"))
        if report.get("schemaVersion") != 1 or not isinstance(report.get("status"), str):
            raise RuntimeError("The cached game-update report is invalid")
        return report

    def _zanik_state(self, server_id: str, root: str) -> dict:
        config_path = f"{root}/zanik/config/zanik.conf"
        config_exists = self._exists(server_id, config_path)
        installed = (config_exists or self._exists(server_id, f"{root}/zanik/VERSION") or
                     self._exists(server_id, f"{root}/zanik/install-state.json"))
        enabled = None
        if installed and config_exists:
            try:
                text = self.client.read_file(server_id, config_path).decode("utf-8-sig")
                match = re.search(r"(?mi)^\s*ZANIK_ENABLED\s*=\s*(true|false|1|0|yes|no|on|off)\s*$", text)
                enabled = not match or match.group(1).lower() in {"true", "1", "yes", "on"}
            except (PterodactylError, UnicodeDecodeError):
                enabled = None
        return {"zanikInstalled": installed, "zanikEnabled": enabled}

    def read_configuration(self, server_id: str, server_root: str) -> dict:
        """Return configuration state without treating an uninstalled server as an error."""
        root = validate_server_root(server_root)
        state = self._zanik_state(server_id, root)
        path = f"{root}/zanik/config/zanik.conf"
        if not state["zanikInstalled"]:
            return {**state, "exists": False, "data": None}
        if not self._exists(server_id, path):
            return {**state, "exists": False, "data": None,
                    "error": "Zanik is installed but its configuration file is missing. Use Update / repair Zanik."}
        return {**state, "exists": True, "data": self.client.read_file(server_id, path)}

    def check_game_update(self, server_id: str, server_root: str, timeout: float = 110.0,
                          force: bool = True) -> dict:
        """Compare the remote app manifest with public metadata, independent of Zanik."""
        root = validate_server_root(server_root)
        state = self._zanik_state(server_id, root)
        cached = None
        if state["zanikInstalled"]:
            try:
                cached = self._read_game_update_report(server_id, root)
            except Exception:
                pass
        if not force and cached and cached.get("status") in {"current", "update-available"}:
            try:
                checked = dt.datetime.fromisoformat(str(cached["checkedAt"]).replace("Z", "+00:00"))
                if (dt.datetime.now(dt.timezone.utc) - checked).total_seconds() < INTERVAL_SECONDS:
                    return {**cached, **state, "fresh": False, "serverRunning": None, "cachedWithinInterval": True}
            except (KeyError, TypeError, ValueError):
                pass

        try:
            manifest = self.client.read_file(server_id, f"{root}/steamapps/appmanifest_4019830.acf")
            installed = installed_build(manifest)
        except (PterodactylError, RuntimeError) as error:
            return {"schemaVersion": 1, "status": "missing-installed-build", **state,
                    "fresh": False, "message": str(error)}
        try:
            latest = self.steam_checker.latest_build()
            available = installed != latest
            return {"schemaVersion": 1, "status": "update-available" if available else "current",
                    "installedBuild": installed, "latestBuild": latest, "updateAvailable": available,
                    "source": "public-steamcmd-metadata", "fresh": True, **state}
        except RuntimeError as public_error:
            if cached:
                return {**cached, **state, "fresh": False,
                        "message": f"{public_error}; showing the last server-cached result."}
            if not state["zanikInstalled"]:
                return {"schemaVersion": 1, "status": "metadata-unavailable", **state,
                        "installedBuild": installed, "fresh": False, "message": str(public_error)}

        resources = self.client.resources(server_id)
        running = resources.get("current_state") == "running"
        if not running:
            return {"schemaVersion": 1, "status": "offline-no-cache", **state,
                    "installedBuild": installed, "fresh": False, "serverRunning": False,
                    "message": "Public metadata was unavailable and the stopped server has no cached launcher result."}

        request_id = secrets.token_hex(12)
        self.client.command(server_id, f"zanik-check-game-update {request_id}")
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            try:
                report = self._read_game_update_report(server_id, root)
                if report.get("requestId") == request_id:
                    return {**report, **state, "fresh": True, "serverRunning": True}
            except (PterodactylError, UnicodeDecodeError, ValueError, RuntimeError):
                pass
            time.sleep(1)
        return {"schemaVersion": 1, "status": "unsupported-or-timeout", **state, "fresh": False,
                "serverRunning": True,
                "message": "The current Zanik launcher did not return a fresh game-update report."}

    def update_game(self, profile_id: str, server_id: str, server_root: str,
                    expected_build: str, progress: Callable[[str, int], None] = lambda _m, _p: None) -> dict:
        """Perform a confirmed game update without running compatibility analysis."""
        if not str(expected_build).isdigit():
            raise ValueError("The expected Dragonwilds build is invalid")
        root = validate_server_root(server_root)
        self.preflight(server_id, "game-update")
        try:
            install = json.loads(self.client.read_file(server_id, f"{root}/zanik/install-state.json"))
        except Exception:
            install = {"mode": "wrapper", "version": None}
        mode = install.get("mode", "wrapper")
        if mode not in {"wrapper", "replace-executable"}:
            raise RuntimeError("The installation mode is unknown; repair Zanik before updating Dragonwilds")
        journal = OperationJournal.create(profile_id, "game-update")
        self.journals.save(journal)
        timestamp = dt.datetime.now(dt.timezone.utc).strftime("%Y%m%dT%H%M%SZ")
        backup = f"{root}/zanik-backups/game-update/{timestamp}-{journal.operation_id[:8]}"
        backup_created = False
        previous_auto = None
        def record_installation(executable_sha256: str, build: str) -> None:
            if not install.get("version"):
                return
            digest = str(executable_sha256).lower()
            if len(digest) != 64 or any(character not in "0123456789abcdef" for character in digest):
                raise RuntimeError("The updated official executable fingerprint is invalid")
            install["officialSha256"] = digest
            install["gameBuild"] = str(build)
            install["gameUpdatedAt"] = dt.datetime.now(dt.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
            path = f"{root}/zanik/install-state.json"
            data = (json.dumps(install, separators=(",", ":")) + "\n").encode()
            self.client.write_file(server_id, path, data)
            if sha256(self.client.read_file(server_id, path)) != sha256(data):
                raise RuntimeError("The updated installation metadata did not verify")
            self.client.chmod(server_id, f"{root}/zanik", [("install-state.json", 0o600)])
        try:
            progress("Checking AUTO_UPDATE and server state", 5)
            variable = self._auto_update(server_id)
            if mode == "replace-executable":
                if not variable["editable"] or variable["value"] != "0":
                    raise RuntimeError("Replacement mode requires editable, verified AUTO_UPDATE=0 before a game update")
            elif variable["value"] == "0":
                if not variable["editable"]:
                    raise RuntimeError("AUTO_UPDATE is disabled and this API key cannot enable it for the requested update")
                previous_auto = "0"
                self.client.set_startup_variable(server_id, "AUTO_UPDATE", "1")
                if self._auto_update(server_id)["value"] != "1":
                    raise RuntimeError("Manager could not temporarily enable AUTO_UPDATE")
                journal.rollback.append({"kind": "restore-startup-variable", "key": "AUTO_UPDATE",
                                         "managedValue": "1", "value": "0"})
                self.journals.save(journal)

            if mode == "wrapper":
                progress("Starting the normal Pterodactyl Steam update", 25)
                current = self.client.resources(server_id).get("current_state")
                self.client.power(server_id, "restart" if current == "running" else "start")
                deadline = time.monotonic() + 900
                installed = ""
                while time.monotonic() < deadline:
                    try:
                        installed = installed_build(self.client.read_file(
                            server_id, f"{root}/steamapps/appmanifest_4019830.acf"))
                        if installed == expected_build:
                            break
                    except Exception:
                        pass
                    time.sleep(2)
                if installed != expected_build:
                    raise RuntimeError("Pterodactyl did not install the expected Dragonwilds build before the timeout")
                if install.get("version"):
                    runtime = None
                    runtime_deadline = time.monotonic() + 120
                    while time.monotonic() < runtime_deadline:
                        try:
                            candidate = json.loads(self.client.read_file(server_id, f"{root}/zanik/state/runtime-game.json"))
                            if candidate.get("gameBuild") == expected_build:
                                runtime = candidate; break
                        except Exception:
                            pass
                        time.sleep(2)
                    if not runtime:
                        raise RuntimeError("Dragonwilds updated, but the Zanik wrapper did not report the new executable fingerprint")
                    record_installation(runtime.get("officialSha256", ""), installed)
                progress("Restoring the previous automatic-update preference", 90)
                if previous_auto is not None:
                    self.client.set_startup_variable(server_id, "AUTO_UPDATE", previous_auto)
                    if self._auto_update(server_id)["value"] != previous_auto:
                        raise RuntimeError("Dragonwilds updated, but Manager could not restore the prior AUTO_UPDATE preference")
                journal.state = "complete"; journal.completed_steps.append("game-updated")
                self.journals.save(journal); self.journals.prune(); progress("Dragonwilds update complete", 100)
                return {"mode": mode, "installedBuild": installed, "maintenanceExpected": True,
                        "operationId": journal.operation_id}

            progress("Stopping the server and preparing a launcher-aware update", 15)
            if self.client.resources(server_id).get("current_state") != "offline":
                self.client.power(server_id, "stop"); self._wait_offline(server_id)
            game = f"{root}/{GAME_RELATIVE}"
            launcher = self.client.read_file(server_id, game)
            original_entry = self._entry(server_id, game + ".zanik-original")
            if (LAUNCHER_MARKER not in launcher or not original_entry or
                    not original_entry.get("is_file") or int(original_entry.get("size") or 0) < 1024 * 1024):
                raise RuntimeError("Replacement launcher or preserved official executable did not verify")
            self._mkdir(server_id, backup)
            self._write_backup_manifest(server_id, backup, "game-update", journal.operation_id, "preparing")
            backup_created = True
            self.client.upload(server_id, backup, "compat-launcher", launcher)
            self.client.write_file(server_id, f"{root}/zanik/state/game-update-launcher", launcher)
            request = (json.dumps({"schemaVersion": 1, "operationId": journal.operation_id,
                                   "backup": backup}, separators=(",", ":")) + "\n").encode()
            self.client.write_file(server_id, f"{root}/zanik/state/game-update-request.json", request)
            self.client.chmod(server_id, f"{root}/zanik/state", [("game-update-launcher", 0o755)])
            updater = self.client.read_file(server_id, f"{root}/zanik/scripts/update-game.sh")
            if b"ZANIK_GAME_UPDATER=1" not in updater:
                raise RuntimeError("The packaged one-shot game updater did not verify")
            try:
                self.client.delete(server_id, f"{root}/zanik/state", ["game-update-result.json"])
            except PterodactylError as error:
                if error.status not in {404, 422}:
                    raise
            self.client.write_file(server_id, game, updater)
            self.client.chmod(server_id, str(PurePosixPath(game).parent), [(PurePosixPath(game).name, 0o755)])
            journal.rollback.append({"kind": "restore-game-launcher", "from": f"{backup}/compat-launcher",
                                     "to": game, "sha256": sha256(launcher), "updaterSha256": sha256(updater),
                                     "backup": backup})
            journal.completed_steps.append("one-shot-updater-installed"); self.journals.save(journal)
            progress("Running SteamCMD through the guarded one-shot updater", 35)
            self.client.power(server_id, "start")
            report = None; deadline = time.monotonic() + 960
            while time.monotonic() < deadline:
                try:
                    report = json.loads(self.client.read_file(server_id, f"{root}/zanik/state/game-update-result.json"))
                    if report.get("status"):
                        break
                except Exception:
                    pass
                time.sleep(2)
            if not report:
                raise RuntimeError("The launcher-aware Dragonwilds updater did not produce a result before the timeout")
            if report.get("status") != "complete" or report.get("installedBuild") != expected_build:
                raise RuntimeError("The launcher-aware Dragonwilds update failed safely: " + str(report.get("status")))
            if report.get("saveFingerprintBefore") != report.get("saveFingerprintAfter"):
                raise RuntimeError("The game save-tree fingerprint changed; the server remains stopped for review")
            record_installation(report.get("officialSha256", ""), report["installedBuild"])
            progress("Starting Dragonwilds through the restored Zanik launcher", 92)
            self.client.power(server_id, "start")
            journal.state = "complete"; journal.completed_steps.append("game-updated")
            self.journals.save(journal)
            self._finish_backup(profile_id, "game-update", server_id, root, backup, "game-update",
                                journal.operation_id, "complete")
            try:
                self.client.delete(server_id, f"{root}/zanik/state",
                                   ["game-update-launcher", "game-update-request.json", "game-update-result.json"])
            except Exception:
                pass
            self.journals.prune(); progress("Dragonwilds update complete", 100)
            return {"mode": mode, "installedBuild": report["installedBuild"], "backup": backup,
                    "maintenanceExpected": True, "operationId": journal.operation_id}
        except Exception as error:
            if previous_auto is not None:
                try:
                    if self._auto_update(server_id)["value"] == "1":
                        self.client.set_startup_variable(server_id, "AUTO_UPDATE", previous_auto)
                except Exception:
                    pass
            journal.state = "failed"; journal.error = str(error); self.journals.save(journal)
            if backup_created:
                try:
                    self._finish_backup(profile_id, "game-update", server_id, root, backup, "game-update",
                                        journal.operation_id, "failed")
                except Exception:
                    pass
            raise

    def _server_asset_audit(self, server_id: str, root: str, manifest: dict,
                            timeout: float = 300.0) -> dict:
        request_id = secrets.token_hex(12)
        filename = f"asset-verification-{request_id}.json"
        report_path = f"{root}/zanik/state/{filename}"
        self.client.command(server_id, f"zanik-doctor-assets {request_id}")
        deadline = time.monotonic() + timeout
        raw = None
        while time.monotonic() < deadline:
            try:
                raw = self.client.read_file(server_id, report_path)
                break
            except PterodactylError as error:
                if error.status != 404:
                    raise
                time.sleep(1)
        if raw is None:
            raise RuntimeError("server-side asset verifier did not produce a report")
        report = json.loads(raw)
        entries = manifest.get("files", [])
        canonical = "".join(f"{entry['sha256']}\t{entry['path']}\n" for entry in entries).encode()
        manifest_path = f"{root}/zanik/assets/current/asset-manifest.json"
        manifest_raw = self.client.read_file(server_id, manifest_path)
        if (report.get("schemaVersion") != 1 or report.get("requestId") != request_id or
                report.get("manifestSha256") != sha256(manifest_raw) or
                report.get("expected") != len(entries) or
                report.get("status") not in {"valid", "invalid"}):
            raise RuntimeError("server-side asset verification report did not match the active manifest")
        if report["status"] == "valid" and report.get("aggregateSha256") != sha256(canonical):
            raise RuntimeError("server-side asset verification aggregate did not match the active manifest")
        report["method"] = "server-side-aggregate"
        try:
            self.client.delete(server_id, f"{root}/zanik/state", [filename])
        except Exception:
            pass
        return report

    def save_config(self, server_id: str, server_root: str, content: bytes, expected_sha256: str) -> dict:
        self.preflight(server_id, "config")
        root = server_root.rstrip("/")
        path = f"{root}/zanik/config/zanik.conf"
        current = self.client.read_file(server_id, path)
        if sha256(current) != expected_sha256:
            raise RuntimeError("Configuration changed on the server; reload it before saving")
        timestamp = dt.datetime.now(dt.timezone.utc).strftime("%Y%m%dT%H%M%SZ")
        operation_id = secrets.token_hex(8)
        backup_dir = f"{root}/zanik-backups/manager/{timestamp}-{operation_id[:8]}-config"
        self._mkdir(server_id, backup_dir)
        self._write_backup_manifest(server_id, backup_dir, "configuration", operation_id, "preparing",
                                    managed_paths=[path], known_bytes=len(current))
        self.client.upload(server_id, backup_dir, "zanik.conf", current)
        try:
            temporary = path + ".manager-new"
            self.client.write_file(server_id, temporary, content)
            if sha256(self.client.read_file(server_id, temporary)) != sha256(content):
                raise RuntimeError("Uploaded configuration did not verify")
            config_root = f"{root}/zanik/config"
            old_name = f"zanik.conf.manager-old-{timestamp}"
            self.client.rename(server_id, config_root, [("zanik.conf", old_name)])
            try:
                self.client.rename(server_id, config_root, [("zanik.conf.manager-new", "zanik.conf")])
            except Exception:
                if not self._exists(server_id, path) and self._exists(server_id, f"{config_root}/{old_name}"):
                    self.client.rename(server_id, config_root, [(old_name, "zanik.conf")])
                raise
            self.client.delete(server_id, config_root, [old_name])
            self.client.chmod(server_id, f"{root}/zanik/config", [("zanik.conf", 0o600)])
            self._write_backup_manifest(server_id, backup_dir, "configuration", operation_id, "complete")
            self._supersede_failed_remote_backups(server_id, root, "configuration", backup_dir)
            self.cleanup_backup_retention(server_id, root)
        except Exception:
            try: self._write_backup_manifest(server_id, backup_dir, "configuration", operation_id, "failed")
            except Exception: pass
            raise
        return {"sha256": sha256(content), "backup": f"{backup_dir}/zanik.conf"}

    def uninstall(self, profile_id: str, server_id: str, server_root: str, purge_data: bool = False) -> dict:
        root = validate_server_root(server_root)
        operation = OperationJournal.create(profile_id, "uninstall")
        self.journals.save(operation)
        state = json.loads(self.client.read_file(server_id, f"{root}/zanik/install-state.json"))
        mode = state.get("mode")
        if mode not in {"wrapper", "replace-executable"}:
            raise RuntimeError("Installation metadata has an unknown mode; refusing uninstall")
        self.preflight(server_id, "replacement-install" if mode == "replace-executable"
                       or (state.get("startupAdapter") or {}).get("managedByZanik") else "install")
        was_running = self.client.resources(server_id).get("current_state") != "offline"
        if was_running:
            self.client.power(server_id, "stop"); self._wait_offline(server_id)
        timestamp = dt.datetime.now(dt.timezone.utc).strftime("%Y%m%dT%H%M%SZ")
        backup = f"{root}/zanik-backups/manager/{timestamp}-{operation.operation_id[:8]}-uninstall"
        self._mkdir(server_id, backup)
        data_was_present = self._directory_exists(server_id, f"{root}/zanik-data")
        managed_paths = [f"{root}/zanik"]
        if mode == "replace-executable":
            managed_paths.extend([f"{root}/{GAME_RELATIVE}", f"{root}/{GAME_RELATIVE}.zanik-original"])
        if purge_data and data_was_present:
            managed_paths.append(f"{root}/zanik-data")
        self._write_backup_manifest(server_id, backup, "uninstall", operation.operation_id, "preparing",
                                    managed_paths=managed_paths)
        try:
            game = f"{root}/{GAME_RELATIVE}"
            startup_status = "not-applicable"
            if mode == "replace-executable":
                launcher = self.client.download_file(server_id, game)
                original = self.client.download_file(server_id, game + ".zanik-original")
                if LAUNCHER_MARKER not in launcher or not original.startswith(b"\x7fELF"):
                    raise RuntimeError("Executable state is not a verified Zanik replacement; refusing uninstall")
                expected = str(state.get("officialSha256") or "").lower()
                if len(expected) == 64 and all(character in "0123456789abcdef" for character in expected):
                    if sha256(original) != expected:
                        raise RuntimeError("Preserved official executable does not match installation metadata")
                self.client.rename(server_id, root, [(str(PurePosixPath(game).relative_to(root)),
                                                      f"zanik-backups/manager/{PurePosixPath(backup).name}/compat-launcher"),
                                                     (str(PurePosixPath(game + '.zanik-original').relative_to(root)),
                                                      str(PurePosixPath(game).relative_to(root)))])
                self._restore_managed_auto_update(server_id, state.get("autoUpdate"))
            else:
                startup_status = self._restore_managed_wrapper_startup(server_id, state.get("startupAdapter"))
            self.client.rename(server_id, root, [("zanik", f"zanik-backups/manager/{PurePosixPath(backup).name}/zanik")])
            if purge_data and self._directory_exists(server_id, f"{root}/zanik-data"):
                self.client.rename(server_id, root, [("zanik-data", f"zanik-backups/manager/{PurePosixPath(backup).name}/purged-zanik-data")])
            if self._directory_exists(server_id, f"{root}/zanik"):
                raise RuntimeError("Uninstall verification found installed Zanik files still active")
            if purge_data and self._directory_exists(server_id, f"{root}/zanik-data"):
                raise RuntimeError("Uninstall verification found persistent data still active after the requested purge")
            if not purge_data and data_was_present and not self._directory_exists(server_id, f"{root}/zanik-data"):
                raise RuntimeError("Uninstall verification found that persistent Zanik data was not preserved")
            if mode == "replace-executable":
                restored = self.client.download_file(server_id, game)
                if not restored.startswith(b"\x7fELF") or self._exists(server_id, game + ".zanik-original"):
                    raise RuntimeError("Uninstall could not verify restoration of the official game executable")
            operation.state = "complete"; operation.completed_steps.append("uninstalled"); self.journals.save(operation)
            self._finish_backup(profile_id, "uninstall", server_id, root, backup, "uninstall",
                                operation.operation_id, "complete")
            self.journals.prune()
            startup_action_required = mode == "wrapper" and startup_status == "unmanaged"
            if was_running and not startup_action_required:
                self.client.power(server_id, "start")
            return {"backup": backup, "dataPurged": purge_data, "dataPreserved": not purge_data,
                    "startupStatus": startup_status, "startupActionRequired": startup_action_required,
                    "serverRestarted": was_running and not startup_action_required,
                    "operationId": operation.operation_id}
        except Exception as error:
            operation.state = "failed"; operation.error = str(error); self.journals.save(operation)
            try:
                self._write_backup_manifest(server_id, backup, "uninstall", operation.operation_id, "failed")
            except Exception:
                pass
            raise
