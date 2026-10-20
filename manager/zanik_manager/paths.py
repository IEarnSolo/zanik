from __future__ import annotations

import os
from pathlib import Path


def app_data_root() -> Path:
    base = os.environ.get("LOCALAPPDATA")
    if not base:
        base = str(Path.home() / "AppData" / "Local")
    return Path(base) / "Zanik Manager"


def ensure_layout(root: Path | None = None) -> dict[str, Path]:
    root = root or app_data_root()
    paths = {
        "root": root,
        "assets": root / "assets",
        "journals": root / "journals",
        "logs": root / "logs",
        "support": root / "support",
        "mapping_recovery": root / "mapping-recovery",
        "asset_work": root / "asset-work",
        "updates": root / "updates",
        "file_metadata": root / "file-metadata",
        "doctor_cache": root / "doctor-cache",
    }
    for path in paths.values():
        path.mkdir(parents=True, exist_ok=True)
    return paths
