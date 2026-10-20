from __future__ import annotations

import datetime as dt
import posixpath
import time
from dataclasses import dataclass, field
from pathlib import PurePosixPath


MAX_ENTRIES = 100_000
MAX_DIRECTORIES = 10_000
CACHE_SECONDS = 5 * 60


@dataclass(frozen=True)
class VirtualRoot:
    key: str
    label: str
    path: str
    is_file: bool = False


@dataclass(frozen=True)
class FileEntry:
    name: str
    path: str
    is_file: bool
    size: int
    modified_at: str
    mime: str = ""


@dataclass
class FileIndex:
    profile_id: str
    generation: int
    roots: list[VirtualRoot]
    directories: dict[str, list[FileEntry]] = field(default_factory=dict)
    root_totals: dict[str, int] = field(default_factory=dict)
    file_count: int = 0
    directory_count: int = 0
    total_size: int = 0
    complete: bool = False
    partial: bool = False
    warnings: list[str] = field(default_factory=list)
    completed_at: float = 0.0

    def fresh(self, now: float | None = None) -> bool:
        return self.complete and (now or time.monotonic()) - self.completed_at < CACHE_SECONDS


def safe_server_root(value: str) -> str:
    path = value.replace("\\", "/").rstrip("/")
    parsed = PurePosixPath(path)
    if not path.startswith("/") or ".." in parsed.parts or path == "/":
        raise ValueError("The server root is not safe for file browsing")
    return path


def virtual_roots(server_root: str, installation: dict | None = None) -> list[VirtualRoot]:
    root = safe_server_root(server_root)
    roots = [
        VirtualRoot("installation", "Active installation", f"{root}/zanik"),
        VirtualRoot("persistent", "Persistent data", f"{root}/zanik-data"),
        VirtualRoot("backups", "Backups", f"{root}/zanik-backups"),
        VirtualRoot("recovery", "Manager recovery", f"{root}/.zanik-manager"),
        VirtualRoot("releases", "Release uploads", f"{root}/zanik-release"),
        VirtualRoot("incoming", "Incoming assets", f"{root}/zanik-assets-incoming"),
    ]
    state = installation or {}
    install = state.get("installation", {}) if isinstance(state.get("installation"), dict) else {}
    launcher = f"{root}/RSDragonwilds/Binaries/Linux/RSDragonwildsServer-Linux-Shipping"
    expected_original = launcher + ".zanik-original"
    if (state.get("installed") and state.get("mode") == "replace-executable" and
            not state.get("repairRequired") and install.get("officialExecutable") == expected_original):
        roots.append(VirtualRoot("launcher", "Managed launcher", launcher, True))
    return roots


def normalize_entry(directory: str, raw: dict) -> FileEntry | None:
    name = str(raw.get("name", ""))
    if not name or name in {".", ".."} or "/" in name or "\\" in name:
        return None
    mode = str(raw.get("mode", ""))
    if raw.get("is_symlink") or raw.get("is_link") or mode.startswith("l") or name.endswith(".zanik-original"):
        return None
    is_file = bool(raw.get("is_file", False))
    size = raw.get("size", 0) if is_file else 0
    try:
        size = max(0, int(size or 0))
    except (TypeError, ValueError):
        size = 0
    return FileEntry(name=name, path=posixpath.join(directory.rstrip("/"), name), is_file=is_file,
                     size=size, modified_at=str(raw.get("modified_at") or ""),
                     mime=str(raw.get("mimetype") or raw.get("mime") or ""))


def list_directory(client, server_id: str, directory: str) -> list[FileEntry]:
    return [entry for raw in client.list_files(server_id, directory)
            if (entry := normalize_entry(directory, raw)) is not None]


def file_type(entry: FileEntry) -> str:
    if not entry.is_file:
        return "File folder"
    lower = entry.name.lower()
    known = {
        "zanik-server": "Zanik launcher", "zanik-compat-analyzer": "Zanik analyzer",
        "libzanik.so": "Linux shared library", "install-state.json": "Zanik installation metadata",
        "asset-manifest.json": "Zanik asset manifest", ".zanik-backup.json": "Zanik backup metadata",
    }
    if lower in known:
        return known[lower]
    extension = PurePosixPath(lower).suffix
    types = {".json": "JSON file", ".conf": "Configuration file", ".log": "Log file",
             ".txt": "Text file", ".md": "Markdown file", ".sh": "Shell script",
             ".so": "Linux shared library", ".zip": "ZIP archive", ".gz": "Gzip archive",
             ".png": "PNG image", ".jpg": "JPEG image", ".jpeg": "JPEG image",
             ".tsv": "TSV data", ".html": "HTML document", ".css": "CSS stylesheet",
             ".js": "JavaScript file", ".woff2": "Web font"}
    return types.get(extension, (extension[1:].upper() + " file") if extension else "File")


def format_size(value: int) -> str:
    size = float(max(0, value))
    for unit in ("B", "KiB", "MiB", "GiB", "TiB"):
        if size < 1024 or unit == "TiB":
            return f"{int(size)} B" if unit == "B" else f"{size:.1f} {unit}"
        size /= 1024
    return "0 B"


def format_modified(value: str) -> str:
    if not value:
        return "Unknown"
    try:
        parsed = dt.datetime.fromisoformat(value.replace("Z", "+00:00"))
        if parsed.tzinfo:
            parsed = parsed.astimezone()
        return parsed.strftime("%Y-%m-%d %I:%M %p")
    except ValueError:
        return value


class RemoteFileIndexer:
    """Build a bounded metadata-only index using list_files and no content endpoints."""

    def __init__(self, client, server_id: str, profile_id: str, generation: int,
                 roots: list[VirtualRoot], max_entries: int = MAX_ENTRIES,
                 max_directories: int = MAX_DIRECTORIES):
        self.client = client
        self.server_id = server_id
        self.index = FileIndex(profile_id, generation, roots,
                               root_totals={root.key: 0 for root in roots})
        self.max_entries = max_entries
        self.max_directories = max_directories
        self._entry_count = 0

    def run(self, progress=lambda _update: None) -> FileIndex:
        for root in self.index.roots:
            if self.index.partial:
                break
            if root.is_file:
                self._index_file_root(root, progress)
            else:
                self._index_directory_root(root, progress)
        self.index.complete = not self.index.partial
        self.index.completed_at = time.monotonic()
        progress(self._update("complete" if self.index.complete else "partial"))
        return self.index

    def _index_file_root(self, root: VirtualRoot, progress):
        parent, name = posixpath.split(root.path)
        try:
            match = next((item for item in list_directory(self.client, self.server_id, parent)
                          if item.name == name and item.is_file), None)
        except Exception as error:
            self.index.warnings.append(f"{root.label}: {error}")
            progress(self._update("warning")); return
        if match:
            self.index.directories[root.path] = [match]
            self._entry_count += 1
            self.index.file_count += 1; self.index.total_size += match.size
            self.index.root_totals[root.key] += match.size
            progress(self._update("progress", root.key, root.path, [match]))

    def _index_directory_root(self, root: VirtualRoot, progress):
        queue = [root.path]
        while queue and not self.index.partial:
            directory = queue.pop(0)
            if self.index.directory_count >= self.max_directories:
                self._limit("directory"); break
            try:
                entries = list_directory(self.client, self.server_id, directory)
            except Exception as error:
                if directory != root.path:
                    self.index.warnings.append(f"{directory}: {error}")
                else:
                    self.index.warnings.append(f"{root.label} is unavailable.")
                progress(self._update("warning")); continue
            if self._entry_count + len(entries) > self.max_entries:
                remaining = max(0, self.max_entries - self._entry_count)
                entries = entries[:remaining]
                self._limit("entry")
            self._entry_count += len(entries)
            self.index.directories[directory] = entries
            self.index.directory_count += 1
            for entry in entries:
                if entry.is_file:
                    self.index.file_count += 1; self.index.total_size += entry.size
                    self.index.root_totals[root.key] += entry.size
                else:
                    queue.append(entry.path)
            progress(self._update("progress", root.key, directory, entries))

    def _limit(self, kind: str):
        self.index.partial = True
        self.index.warnings.append(
            f"The safe {kind} limit was reached. Displayed counts and sizes are partial; no additional paths were scanned.")

    def _update(self, state: str, root_key: str = "", directory: str = "",
                entries: list[FileEntry] | None = None) -> dict:
        return {"profileId": self.index.profile_id, "generation": self.index.generation,
                "state": state, "rootKey": root_key, "directory": directory,
                "entries": entries or [], "files": self.index.file_count,
                "directories": self.index.directory_count, "bytes": self.index.total_size,
                "rootTotals": dict(self.index.root_totals), "warnings": list(self.index.warnings)}
