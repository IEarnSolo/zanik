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

    def incomplete(self, profile_id: str) -> list[OperationJournal]:
        result = []
        for path in self.root.glob("*.json"):
            journal = self.load(path)
            if journal.profile_id == profile_id and journal.state not in {"complete", "rolled-back"}:
                result.append(journal)
        return sorted(result, key=lambda item: item.created_at)
