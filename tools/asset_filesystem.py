"""Filesystem helpers for game-export paths, including Windows long paths."""

from __future__ import annotations

import json
import ntpath
import os
import shutil
from pathlib import Path
from typing import Any


def extended_path(path: Path | str, platform: str = os.name) -> Path:
    """Return an absolute extended-length Windows path when needed."""
    value = (ntpath.abspath(os.fspath(path)) if platform == "nt"
             else os.path.abspath(os.fspath(path)))
    if platform != "nt" or value.startswith("\\\\?\\"):
        return Path(value)
    if value.startswith("\\\\"):
        return Path("\\\\?\\UNC\\" + value[2:])
    return Path("\\\\?\\" + value)


def read_text(path: Path | str, *, encoding: str = "utf-8") -> str:
    return extended_path(path).read_text(encoding=encoding)


def read_bytes(path: Path | str) -> bytes:
    return extended_path(path).read_bytes()


def copy_file(source: Path | str, destination: Path | str) -> None:
    """Copy contents from a possibly long source path; generated files need no source metadata."""
    shutil.copyfile(extended_path(source), destination)


def read_json(path: Path | str, *, encoding: str = "utf-8-sig") -> Any:
    return json.loads(read_text(path, encoding=encoding))
