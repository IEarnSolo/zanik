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
        required = {"bin/libzanik.so", "bin/zanik-server", "scripts/compat-launcher.sh",
                    "scripts/verify-assets.sh",
                    "config/zanik.conf.example", "compatibility/compatibility.tsv", "VERSION"}
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
                if error.status == 522 or (error.status in {500, 502, 503, 504} and failures < 5):
                    failures += 1
                    time.sleep(self.client.retry_seconds)
                    continue
                raise

    def _exists(self, server_id: str, path: str) -> bool:
        return self._entry(server_id, path) is not None

    def _upload_payload(self, server_id: str, stage: str, payload: ReleasePayload, journal: OperationJournal) -> None:
        directories = sorted({str(PurePosixPath(stage) / PurePosixPath(name).parent) for name in payload.entries})
        for directory in directories:
            self._mkdir(server_id, directory)
        modes: dict[str, list[tuple[str, int]]] = {}
        for name, entry in payload.entries.items():
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
                    journal.rollback.append({"kind": "move", "from": f"{root}/{destination}", "to": source})
                    self.journals.save(journal)
            if not self._exists(server_id, f"{root}/zanik/config/zanik.conf"):
                example = self.client.read_file(server_id, f"{root}/zanik/config/zanik.conf.example")
                self.client.write_file(server_id, f"{root}/zanik/config/zanik.conf", example)
            if not self._exists(server_id, f"{root}/zanik/admins.txt"):
                self.client.write_file(server_id, f"{root}/zanik/admins.txt", b"")
            compatibility = self.client.read_file(server_id, f"{root}/zanik/compatibility/compatibility.tsv")
            self.client.write_file(server_id, f"{root}/zanik/compatibility.tsv", compatibility)
            if sha256(self.client.read_file(server_id, f"{root}/zanik/compatibility.tsv")) != sha256(compatibility):
                raise RuntimeError("Installed runtime compatibility manifest did not verify")
            self._mkdir(server_id, f"{root}/zanik-data")
            if not self._exists(server_id, f"{root}/zanik-data/console-admin.key"):
                self.client.write_file(server_id, f"{root}/zanik-data/console-admin.key", secrets.token_hex(32).encode())
            self.client.chmod(server_id, root, [("zanik/config/zanik.conf", 0o600), ("zanik/admins.txt", 0o600),
                                                ("zanik/compatibility.tsv", 0o644),
                                                ("zanik-data/console-admin.key", 0o600)])

            game = f"{root}/{GAME_RELATIVE}"
            official = game
            official_data = None
            if mode == "replace-executable":
                original = game + ".zanik-original"
                current = self.client.download_file(server_id, game)
                if LAUNCHER_MARKER in current:
                    original_data = self.client.download_file(server_id, original)
                    if not original_data.startswith(b"\x7fELF"):
                        raise RuntimeError("The preserved official executable is not a verified ELF file")
                    official_data = original_data
                    self.client.upload(server_id, backup, "managed-launcher", current)
                    journal.rollback.append({"kind": "restore-file", "from": f"{backup}/managed-launcher",
                                             "to": game, "sha256": sha256(current), "mode": 0o755})
                    self.journals.save(journal)
                elif current.startswith(b"\x7fELF"):
                    if self._exists(server_id, original):
                        prior = self.client.download_file(server_id, original)
                        if not prior.startswith(b"\x7fELF"):
                            raise RuntimeError("Existing .zanik-original is not a verified ELF file")
                        self.client.rename(server_id, root, [(str(PurePosixPath(original).relative_to(root)),
                                                              f"zanik-backups/manager/{PurePosixPath(backup).name}/official-before-steam-update")])
                    self.client.rename(server_id, root, [(str(PurePosixPath(game).relative_to(root)),
                                                          str(PurePosixPath(original).relative_to(root)))])
                    journal.rollback.append({"kind": "restore-official", "from": original, "to": game})
                    self.journals.save(journal)
                elif is_exact_legacy_launcher(current, root):
                    original_data = self.client.download_file(server_id, original)
                    if not original_data.startswith(b"\x7fELF"):
                        raise RuntimeError("The legacy launcher has no verified executable ELF at .zanik-original")
                    official_data = original_data
                    self.client.upload(server_id, backup, "managed-launcher", current)
                    journal.rollback.append({"kind": "restore-file", "from": f"{backup}/managed-launcher",
                                             "to": game, "sha256": sha256(current), "mode": 0o755})
                    self.journals.save(journal)
                else:
                    raise RuntimeError("Unknown executable replacement state; refusing to guess")
                launcher = self.client.read_file(server_id, f"{root}/zanik/scripts/compat-launcher.sh")
                self.client.write_file(server_id, game, launcher)
                self.client.chmod(server_id, str(PurePosixPath(game).parent), [(PurePosixPath(game).name, 0o755)])
                official = original
            if official_data is None:
                official_data = self.client.download_file(server_id, official)
            version = self.client.read_file(server_id, f"{root}/zanik/VERSION").decode().strip()
            state = {"schemaVersion": 1, "version": version, "mode": mode, "serverRoot": root,
                     "officialExecutable": official, "officialSha256": sha256(official_data),
                     "librarySha256": payload.entries["bin/libzanik.so"].sha256,
                     "analyzerSha256": payload.entries["bin/zanik-compat-analyzer"].sha256,
                     "launcherSha256": payload.entries["bin/zanik-server"].sha256,
                     "installedAt": dt.datetime.now(dt.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")}
            self.client.write_file(server_id, f"{root}/zanik/install-state.json", (json.dumps(state, separators=(",", ":")) + "\n").encode())
            self.client.chmod(server_id, f"{root}/zanik", [("install-state.json", 0o600)])
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
            result["assets"] = {"installed": True, "compatible": manifest.get("compatible"),
                                "gameBuild": manifest.get("gameBuild") or manifest.get("clientBuild")}
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
        return result

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
        backup_dir = f"{root}/zanik-backups/manager/{timestamp}-config"
        self._mkdir(server_id, backup_dir)
        self.client.upload(server_id, backup_dir, "zanik.conf", current)
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
            launcher = self.client.download_file(server_id, game)
            original = self.client.download_file(server_id, game + ".zanik-original")
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
