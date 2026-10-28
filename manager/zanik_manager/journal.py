from __future__ import annotations

import datetime as dt
import json
import uuid
from dataclasses import asdict, dataclass, field
from pathlib import Path


@dataclass
class OperationJournal:
    operation_id: str
    profile_id: str
    action: str
    created_at: str
    state: str = "preparing"
    completed_steps: list[str] = field(default_factory=list)
    rollback: list[dict] = field(default_factory=list)
    error: str = ""

    @classmethod
    def create(cls, profile_id: str, action: str):
        return cls(str(uuid.uuid4()), profile_id, action, dt.datetime.now(dt.timezone.utc).isoformat())


class JournalStore:
    def __init__(self, root: Path):
        self.root = root
        self.root.mkdir(parents=True, exist_ok=True)

    def save(self, journal: OperationJournal) -> Path:
        path = self.root / f"{journal.operation_id}.json"
        temporary = path.with_suffix(".tmp")
        temporary.write_text(json.dumps(asdict(journal), indent=2, sort_keys=True) + "\n", encoding="utf-8")
        temporary.replace(path)
        return path

    def load(self, path: Path) -> OperationJournal:
        return OperationJournal(**json.loads(path.read_text(encoding="utf-8")))

    @staticmethod
    def _created_at(journal: OperationJournal) -> dt.datetime | None:
        try:
            value = dt.datetime.fromisoformat(journal.created_at.replace("Z", "+00:00"))
            return value if value.tzinfo else value.replace(tzinfo=dt.timezone.utc)
        except (AttributeError, TypeError, ValueError):
            return None

    def _archive_superseded_failures(self, records: list[tuple[Path, OperationJournal]]) -> None:
        """Archive failed work made obsolete by a later successful equivalent operation."""
        completed: dict[str, dt.datetime] = {}
        for _path, journal in records:
            if journal.state != "complete":
                continue
            created = self._created_at(journal)
            if created is not None and (journal.action not in completed or created > completed[journal.action]):
                completed[journal.action] = created
        archive = self.root / "archive"
        for path, journal in records:
            if journal.state != "failed" or journal.action not in completed:
                continue
            created = self._created_at(journal)
            if created is None or completed[journal.action] <= created:
                continue
            archive.mkdir(parents=True, exist_ok=True)
            path.replace(archive / path.name)

    def incomplete(self, profile_id: str) -> list[OperationJournal]:
        records = []
        for path in self.root.glob("*.json"):
            try:
                journal = self.load(path)
            except (OSError, TypeError, ValueError, json.JSONDecodeError):
                continue
            if journal.profile_id == profile_id:
                records.append((path, journal))
        self._archive_superseded_failures(records)
        result = [journal for path, journal in records
                  if path.exists() and journal.state not in {"complete", "rolled-back", "superseded"}]
        return sorted(result, key=lambda item: item.created_at)

    def supersede_failed(self, profile_id: str, action: str, completed_at: str) -> set[str]:
        """Mark older failures safe for retention cleanup after a verified success."""
        try:
            boundary = dt.datetime.fromisoformat(completed_at.replace("Z", "+00:00"))
            if boundary.tzinfo is None:
                boundary = boundary.replace(tzinfo=dt.timezone.utc)
        except (AttributeError, TypeError, ValueError):
            return set()
        result: set[str] = set()
        for path in self.root.glob("*.json"):
            journal = self.load(path)
            created = self._created_at(journal)
            if (journal.profile_id == profile_id and journal.action == action and journal.state == "failed"
                    and created is not None and created < boundary):
                journal.state = "superseded"
                journal.completed_steps.append("superseded-by-verified-success")
                self.save(journal)
                result.add(journal.operation_id)
        return result

    def prune(self) -> dict[str, int]:
        """Keep unresolved work and the newest completed record per profile/action."""
        records: list[tuple[Path, OperationJournal]] = []
        for path in self.root.glob("*.json"):
            try:
                records.append((path, self.load(path)))
            except (OSError, TypeError, ValueError, json.JSONDecodeError):
                continue
        newest: dict[tuple[str, str], tuple[Path, OperationJournal]] = {}
        for path, journal in records:
            if journal.state != "complete":
                continue
            key = (journal.profile_id, journal.action)
            if key not in newest or journal.created_at > newest[key][1].created_at:
                newest[key] = (path, journal)
        keep = {path for path, _journal in newest.values()}
        removed = 0
        for path, journal in records:
            if path in keep or journal.state not in {"complete", "rolled-back", "superseded"}:
                continue
            try:
                path.unlink()
                removed += 1
            except OSError:
                continue
        archive = self.root / "archive"
        if archive.is_dir():
            for path in archive.glob("*.json"):
                try:
                    path.unlink(); removed += 1
                except OSError:
                    continue
            try:
                archive.rmdir()
            except OSError:
                pass
        return {"removed": removed, "retained": len(records) - removed}
