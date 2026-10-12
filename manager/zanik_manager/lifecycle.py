from __future__ import annotations

import datetime as dt
import hashlib
import json
import os
import secrets
import stat
import time
import zipfile
from dataclasses import asdict, dataclass
from pathlib import Path, PurePosixPath

from .config import content_hash
from .journal import JournalStore, OperationJournal
from .pterodactyl import PterodactylClient, PterodactylError, api_file_path
from .provider import ServerProvider

GAME_RELATIVE = "RSDragonwilds/Binaries/Linux/RSDragonwildsServer-Linux-Shipping"
LAUNCHER_MARKER = b"ZANIK_COMPAT_LAUNCHER=1"


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
        required = {"bin/libzanik.so", "bin/zanik-server", "scripts/compat-launcher.sh", "config/zanik.conf.example", "VERSION"}
        missing = required - set(self.entries)
        if missing:
            raise ValueError(f"Server payload is missing {', '.join(sorted(missing))}")

    def read(self, name: str) -> bytes:
        with zipfile.ZipFile(self.archive) as package:
            return package.read(name)

    def manifest(self) -> dict:
        return {"schemaVersion": 1, "archive": self.archive.name, "archiveSha256": content_hash(self.archive.read_bytes()),
                "files": [asdict(self.entries[name]) for name in sorted(self.entries)]}


class PterodactylProvider(ServerProvider):
    def __init__(self, client: PterodactylClient, journals: JournalStore):
        self.client = client
        self.journals = journals

    def list_servers(self) -> list[dict]:
        return self.client.servers()

    def preflight(self, server_id: str, operation: str, server: dict | None = None) -> None:
        required={
            "doctor": {"file.read", "file.read-content"},
            "install": {"control.start", "control.stop", "file.read", "file.read-content", "file.create", "file.update", "file.delete"},
            "assets": {"file.read", "file.read-content", "file.create", "file.update", "file.delete"},
            "config": {"file.read", "file.read-content", "file.create", "file.update", "file.delete"},
        }.get(operation, set())
        details=server or self.client.server(server_id)
        permissions=set(details.get("_permissions", details.get("permissions", [])) or [])
        if details.get("server_owner") or not permissions:
            return
        missing=required-permissions
        if missing:
            raise RuntimeError("The panel account is missing required permissions: " + ", ".join(sorted(missing)))

    def _path(self, root: str, relative: str) -> str:
        return root.rstrip("/") + "/" + relative.lstrip("/")

    def _mkdir(self, server_id: str, path: str) -> None:
        current = "/"
        for component in PurePosixPath(api_file_path(path)).parts:
            if component == "/":
                continue
            parent = current
            try:
                self.client.create_folder(server_id, parent, component)
            except PterodactylError as error:
                if error.status not in {409, 422}:
                    raise
            current = str(PurePosixPath(parent) / component)

    def _exists(self, server_id: str, path: str) -> bool:
        try:
            self.client.read_file(server_id, path)
            return True
        except PterodactylError as error:
            if error.status == 404:
                return False
            raise

    def _upload_payload(self, server_id: str, stage: str, payload: ReleasePayload, journal: OperationJournal) -> None:
        directories = sorted({str(PurePosixPath(stage) / PurePosixPath(name).parent) for name in payload.entries})
        for directory in directories:
            self._mkdir(server_id, directory)
        modes: list[tuple[str, int]] = []
        for name, entry in payload.entries.items():
            directory = str(PurePosixPath(stage) / PurePosixPath(name).parent)
            data = payload.read(name)
            self.client.upload(server_id, directory, PurePosixPath(name).name, data)
            remote = str(PurePosixPath(stage) / name)
            if sha256(self.client.read_file(server_id, remote)) != entry.sha256:
                raise RuntimeError(f"Remote verification failed for {name}")
            modes.append((name, entry.mode))
        self.client.chmod(server_id, stage, modes)
        journal.completed_steps.append("payload-uploaded-and-verified")
        self.journals.save(journal)

    def _wait_offline(self, server_id: str, timeout: float = 120.0) -> None:
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            state = self.client.resources(server_id).get("current_state", "")
            if state == "offline":
                return
            time.sleep(2)
        raise RuntimeError("Server did not stop before the operation timed out")

    def install(self, profile_id: str, server_id: str, server_root: str, payload: ReleasePayload,
                mode: str, acknowledge_replacement: bool = False, stop_server: bool = True,
                wrapper_adapter: dict | None = None) -> dict:
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
        try:
            self.preflight(server_id, "install")
            if stop_server:
                state = self.client.resources(server_id).get("current_state")
                if state != "offline":
                    self.client.power(server_id, "stop")
                    self._wait_offline(server_id)
                journal.completed_steps.append("server-stopped")
                self.journals.save(journal)
            self._mkdir(server_id, stage)
            self._mkdir(server_id, backup)
            self._upload_payload(server_id, stage, payload, journal)
            remote_journal = f"{root}/.zanik-manager/transactions/{journal.operation_id}.json"
            self._mkdir(server_id, str(PurePosixPath(remote_journal).parent))
            self.client.write_file(server_id, remote_journal, json.dumps(asdict(journal), sort_keys=True).encode())

            # Keep an existing installation recoverable before the critical activation.
            if self._exists(server_id, f"{root}/zanik/install-state.json"):
                self.client.rename(server_id, root, [("zanik", f"zanik-backups/manager/{PurePosixPath(backup).name}/zanik")])
                journal.rollback.append({"from": f"{backup}/zanik", "to": f"{root}/zanik"})
            self.client.rename(server_id, root, [(str(PurePosixPath(stage).relative_to(root)), "zanik")])
            journal.rollback.append({"from": f"{root}/zanik", "to": stage})
            journal.state = "activating"
            journal.completed_steps.append("payload-activated")
            self.journals.save(journal)

            old = f"{backup}/zanik"
            for relative in ("config/zanik.conf", "admins.txt", "assets"):
                source = f"{old}/{relative}"
                if self._exists(server_id, source) if relative != "assets" else self._directory_exists(server_id, source):
                    destination = f"zanik/{relative}"
                    self.client.rename(server_id, root, [(str(PurePosixPath(source).relative_to(root)), destination)])
            if not self._exists(server_id, f"{root}/zanik/config/zanik.conf"):
                example = self.client.read_file(server_id, f"{root}/zanik/config/zanik.conf.example")
                self.client.write_file(server_id, f"{root}/zanik/config/zanik.conf", example)
            if not self._exists(server_id, f"{root}/zanik/admins.txt"):
                self.client.write_file(server_id, f"{root}/zanik/admins.txt", b"")
            self._mkdir(server_id, f"{root}/zanik-data")
            if not self._exists(server_id, f"{root}/zanik-data/console-admin.key"):
                self.client.write_file(server_id, f"{root}/zanik-data/console-admin.key", secrets.token_hex(32).encode())
            self.client.chmod(server_id, root, [("zanik/config/zanik.conf", 600), ("zanik/admins.txt", 600),
                                                ("zanik-data/console-admin.key", 600)])

            game = f"{root}/{GAME_RELATIVE}"
            official = game
            if mode == "replace-executable":
                original = game + ".zanik-original"
                current = self.client.read_file(server_id, game)
                if LAUNCHER_MARKER in current:
                    original_data = self.client.read_file(server_id, original)
                    if not original_data.startswith(b"\x7fELF"):
                        raise RuntimeError("The preserved official executable is not a verified ELF file")
                elif current.startswith(b"\x7fELF"):
                    if self._exists(server_id, original):
                        prior = self.client.read_file(server_id, original)
                        if not prior.startswith(b"\x7fELF"):
                            raise RuntimeError("Existing .zanik-original is not a verified ELF file")
                        self.client.rename(server_id, root, [(str(PurePosixPath(original).relative_to(root)),
                                                              f"zanik-backups/manager/{PurePosixPath(backup).name}/official-before-steam-update")])
                    self.client.rename(server_id, root, [(str(PurePosixPath(game).relative_to(root)),
                                                          str(PurePosixPath(original).relative_to(root)))])
                    journal.rollback.append({"kind": "restore-official", "from": original, "to": game})
                    self.journals.save(journal)
                elif is_exact_legacy_launcher(current, root):
                    original_data = self.client.read_file(server_id, original)
                    if not original_data.startswith(b"\x7fELF"):
                        raise RuntimeError("The legacy launcher has no verified executable ELF at .zanik-original")
                else:
                    raise RuntimeError("Unknown executable replacement state; refusing to guess")
                launcher = self.client.read_file(server_id, f"{root}/zanik/scripts/compat-launcher.sh")
                self.client.write_file(server_id, game, launcher)
                self.client.chmod(server_id, str(PurePosixPath(game).parent), [(PurePosixPath(game).name, 755)])
                official = original
            official_data = self.client.read_file(server_id, official)
            version = self.client.read_file(server_id, f"{root}/zanik/VERSION").decode().strip()
            state = {"schemaVersion": 1, "version": version, "mode": mode, "serverRoot": root,
                     "officialExecutable": official, "officialSha256": sha256(official_data),
                     "librarySha256": payload.entries["bin/libzanik.so"].sha256,
                     "analyzerSha256": payload.entries["bin/zanik-compat-analyzer"].sha256,
                     "launcherSha256": payload.entries["bin/zanik-server"].sha256,
                     "installedAt": dt.datetime.now(dt.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")}
            self.client.write_file(server_id, f"{root}/zanik/install-state.json", (json.dumps(state, separators=(",", ":")) + "\n").encode())
            self.client.chmod(server_id, f"{root}/zanik", [("install-state.json", 600)])
            startup_action_required = mode == "wrapper" and not wrapper_adapter
            journal.state = "complete"; journal.completed_steps.append("installation-verified")
            if mode == "wrapper" and wrapper_adapter:
                self.client.set_startup_variable(server_id, wrapper_adapter["key"], wrapper_adapter["value"])
                journal.completed_steps.append("wrapper-startup-configured")
            self.journals.save(journal)
            self.client.write_file(server_id, remote_journal, json.dumps(asdict(journal), sort_keys=True).encode())
            if stop_server and not startup_action_required:
                self.client.power(server_id, "start")
            return {"installation": state, "backup": backup, "operationId": journal.operation_id,
                    "startupActionRequired": startup_action_required}
        except Exception as error:
            journal.state = "failed"; journal.error = str(error); self.journals.save(journal)
            raise

    def rollback(self, journal: OperationJournal, server_id: str, server_root: str) -> None:
        root=server_root.rstrip("/")
        if self.client.resources(server_id).get("current_state") != "offline":
            self.client.power(server_id,"stop"); self._wait_offline(server_id)
        for action in reversed(journal.rollback):
            source=action["from"]; destination=action["to"]
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
        try:
            self.client.list_files(server_id, path)
            return True
        except PterodactylError as error:
            if error.status == 404:
                return False
            raise

    def diagnose(self, server_id: str, server_root: str, deep: bool = False) -> dict:
        self.preflight(server_id, "doctor")
        root = server_root.rstrip("/")
        result = {"healthy": True, "checks": {}}
        try:
            state_raw = self.client.read_file(server_id, f"{root}/zanik/install-state.json")
            state = json.loads(state_raw)
            result["installation"] = state
        except Exception as error:
            return {"healthy": False, "checks": {"installation": str(error)}}
        targets = {"library": ("librarySha256", "bin/libzanik.so"), "analyzer": ("analyzerSha256", "bin/zanik-compat-analyzer"),
                   "launcher": ("launcherSha256", "bin/zanik-server"), "official": ("officialSha256", None)}
        for name, (field, relative) in targets.items():
            path = state["officialExecutable"] if relative is None else f"{root}/zanik/{relative}"
            try:
                valid = sha256(self.client.read_file(server_id, path)) == state.get(field)
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
            result["assets"] = {"installed": True, "compatible": manifest.get("compatible"),
                                "gameBuild": manifest.get("gameBuild") or manifest.get("clientBuild")}
            if deep:
                mismatches = []
                for entry in manifest.get("files", []):
                    try:
                        actual = sha256(self.client.read_file(server_id, f"{root}/zanik/assets/current/{entry['path']}"))
                        if actual != entry["sha256"]:
                            mismatches.append(entry["path"])
                    except Exception:
                        mismatches.append(entry.get("path", "unknown"))
                result["assets"]["mismatches"] = mismatches
                result["healthy"] &= not mismatches
        except Exception:
            result["assets"] = {"installed": False}
        return result

    def save_config(self, server_id: str, server_root: str, content: bytes, expected_sha256: str) -> dict:
        self.preflight(server_id, "config")
        root = server_root.rstrip("/")
        path = f"{root}/zanik/config/zanik.conf"
        current = self.client.read_file(server_id, path)
        if sha256(current) != expected_sha256:
            raise RuntimeError("Configuration changed on the server; reload it before saving")
        timestamp = dt.datetime.now(dt.timezone.utc).strftime("%Y%m%dT%H%M%SZ")
        backup_dir = f"{root}/zanik-backups/manager/{timestamp}-config"
        self._mkdir(server_id, backup_dir)
        self.client.upload(server_id, backup_dir, "zanik.conf", current)
        temporary = path + ".manager-new"
        self.client.write_file(server_id, temporary, content)
        if sha256(self.client.read_file(server_id, temporary)) != sha256(content):
            raise RuntimeError("Uploaded configuration did not verify")
        self.client.rename(server_id, f"{root}/zanik/config", [("zanik.conf", "zanik.conf.manager-old"),
                                                               ("zanik.conf.manager-new", "zanik.conf")])
        self.client.delete(server_id, f"{root}/zanik/config", ["zanik.conf.manager-old"])
        self.client.chmod(server_id, f"{root}/zanik/config", [("zanik.conf", 600)])
        return {"sha256": sha256(content), "backup": f"{backup_dir}/zanik.conf"}

    def uninstall(self, profile_id: str, server_id: str, server_root: str, purge_data: bool = False) -> dict:
        root = server_root.rstrip("/")
        operation = OperationJournal.create(profile_id, "uninstall")
        self.journals.save(operation)
        state = json.loads(self.client.read_file(server_id, f"{root}/zanik/install-state.json"))
        if self.client.resources(server_id).get("current_state") != "offline":
            self.client.power(server_id, "stop"); self._wait_offline(server_id)
        timestamp = dt.datetime.now(dt.timezone.utc).strftime("%Y%m%dT%H%M%SZ")
        backup = f"{root}/zanik-backups/manager/{timestamp}-{operation.operation_id[:8]}-uninstall"
        self._mkdir(server_id, backup)
        game = f"{root}/{GAME_RELATIVE}"
        if state.get("mode") == "replace-executable":
            launcher = self.client.read_file(server_id, game)
            original = self.client.read_file(server_id, game + ".zanik-original")
            if LAUNCHER_MARKER not in launcher or not original.startswith(b"\x7fELF"):
                raise RuntimeError("Executable state is not a verified Zanik replacement; refusing uninstall")
            self.client.rename(server_id, root, [(str(PurePosixPath(game).relative_to(root)),
                                                  f"zanik-backups/manager/{PurePosixPath(backup).name}/compat-launcher"),
                                                 (str(PurePosixPath(game + '.zanik-original').relative_to(root)),
                                                  str(PurePosixPath(game).relative_to(root)))])
        self.client.rename(server_id, root, [("zanik", f"zanik-backups/manager/{PurePosixPath(backup).name}/zanik")])
        if purge_data and self._directory_exists(server_id, f"{root}/zanik-data"):
            self.client.rename(server_id, root, [("zanik-data", f"zanik-backups/manager/{PurePosixPath(backup).name}/purged-zanik-data")])
        operation.state = "complete"; operation.completed_steps.append("uninstalled"); self.journals.save(operation)
        self.client.power(server_id, "start")
        return {"backup": backup, "dataPurged": purge_data, "operationId": operation.operation_id}
