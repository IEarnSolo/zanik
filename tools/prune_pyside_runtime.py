#!/usr/bin/env python3
"""Reduce a PySide6 Essentials wheel to Zanik Manager's runtime modules."""
from __future__ import annotations

import argparse
import shutil
from pathlib import Path

KEEP_FILES = {
    "PySide6/__init__.py", "PySide6/_config.py", "PySide6/_git_pyside_version.py", "PySide6/py.typed",
    "PySide6/QtCore.pyd", "PySide6/QtGui.pyd", "PySide6/QtWidgets.pyd",
    "PySide6/Qt6Core.dll", "PySide6/Qt6Gui.dll", "PySide6/Qt6Widgets.dll", "PySide6/pyside6.abi3.dll",
    "PySide6/concrt140.dll", "PySide6/msvcp140.dll", "PySide6/msvcp140_1.dll", "PySide6/msvcp140_2.dll",
    "PySide6/msvcp140_codecvt_ids.dll", "PySide6/vcamp140.dll", "PySide6/vccorlib140.dll",
    "PySide6/vcomp140.dll", "PySide6/vcruntime140.dll", "PySide6/vcruntime140_1.dll",
    "PySide6/plugins/platforms/qwindows.dll", "PySide6/plugins/platforms/qoffscreen.dll",
    "PySide6/plugins/styles/qmodernwindowsstyle.dll", "PySide6/plugins/imageformats/qgif.dll",
    "PySide6/plugins/imageformats/qico.dll", "PySide6/plugins/imageformats/qjpeg.dll",
}


def prune(root: Path) -> None:
    pyside = root / "PySide6"
    if not pyside.is_dir():
        raise ValueError("PySide6 runtime directory is missing")
    for path in sorted(pyside.rglob("*"), reverse=True):
        relative = path.relative_to(root).as_posix()
        if path.is_file() and relative not in KEEP_FILES:
            path.unlink()
        elif path.is_dir() and not any(path.iterdir()):
            path.rmdir()
    missing = [name for name in KEEP_FILES if not (root / name).is_file()]
    if missing:
        raise ValueError(f"Pruned runtime is missing required files: {missing}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("root", type=Path)
    args = parser.parse_args(); prune(args.root.resolve()); return 0


if __name__ == "__main__":
    raise SystemExit(main())
