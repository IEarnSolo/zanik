from __future__ import annotations

import datetime as dt
import json
import zipfile
from pathlib import Path

from .security import redact


def create_support_bundle(output: Path, diagnostics: dict, profiles: list[dict], logs: list[Path], secrets: tuple[str, ...] = ()) -> Path:
    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output, "w", compression=zipfile.ZIP_DEFLATED) as archive:
        metadata = {"schemaVersion": 1, "createdAt": dt.datetime.now(dt.timezone.utc).isoformat(),
                    "diagnostics": diagnostics, "profiles": profiles}
        archive.writestr("diagnostics.json", redact(json.dumps(metadata, indent=2, sort_keys=True), secrets))
        for path in logs:
            if path.is_file():
                archive.writestr(f"logs/{path.name}", redact(path.read_text(encoding="utf-8", errors="replace"), secrets))
    return output
