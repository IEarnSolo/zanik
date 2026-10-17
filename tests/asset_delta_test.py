#!/usr/bin/env python3
import hashlib
import json
import tempfile
import unittest
import sys
from pathlib import Path
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from tools import zanik_assets


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def manifest(files: dict[str, bytes]) -> dict:
    return {"schemaVersion": zanik_assets.SCHEMA_VERSION, "compatibility": "compatible",
            "missingRequired": [],
            "files": [{"path": path, "sha256": digest(data), "size": len(data)}
                      for path, data in sorted(files.items())]}


class AssetDeltaTests(unittest.TestCase):
    def test_delta_changes_adds_and_removes_atomically(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary); assets = root / "assets"; current = assets / "current"
            current.mkdir(parents=True)
            base = {"items/changed.png": b"old", "items/removed.png": b"remove", "world-map.jpg": b"same"}
            target = {"items/changed.png": b"new", "items/added.png": b"add", "world-map.jpg": b"same"}
            for path, data in base.items():
                item = current / path; item.parent.mkdir(parents=True, exist_ok=True); item.write_bytes(data)
            base_raw = json.dumps(manifest(base), sort_keys=True).encode()
            (current / "asset-manifest.json").write_bytes(base_raw)
            delta = root / "delta"; (delta / "files/items").mkdir(parents=True)
            (delta / "files/items/changed.png").write_bytes(b"new")
            (delta / "files/items/added.png").write_bytes(b"add")
            target_raw = json.dumps(manifest(target), sort_keys=True).encode()
            (delta / "target-asset-manifest.json").write_bytes(target_raw)
            spec = {"schemaVersion": 1, "requestId": "0123456789abcdef",
                    "baseManifestSha256": digest(base_raw), "targetManifestSha256": digest(target_raw),
                    "changed": [record for record in manifest(target)["files"]
                                if record["path"] in {"items/changed.png", "items/added.png"}],
                    "removed": ["items/removed.png"]}
            (delta / "asset-delta.json").write_text(json.dumps(spec))
            with mock.patch.object(zanik_assets, "validate_manifest_tree", return_value=manifest(target)):
                result = zanik_assets.apply_delta_tree(delta, assets)
            self.assertEqual(result["changed"], 2)
            self.assertEqual((assets / "current/items/changed.png").read_bytes(), b"new")
            self.assertEqual((assets / "current/items/added.png").read_bytes(), b"add")
            self.assertFalse((assets / "current/items/removed.png").exists())
            self.assertEqual((assets / "previous/items/changed.png").read_bytes(), b"old")

    def test_base_mismatch_preserves_active_tree(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary); assets = root / "assets"; current = assets / "current"; current.mkdir(parents=True)
            data = {"world-map.jpg": b"same"}; raw = json.dumps(manifest(data), sort_keys=True).encode()
            (current / "world-map.jpg").write_bytes(b"same"); (current / "asset-manifest.json").write_bytes(raw)
            delta = root / "delta"; delta.mkdir()
            (delta / "target-asset-manifest.json").write_bytes(raw)
            (delta / "asset-delta.json").write_text(json.dumps({"schemaVersion": 1,
                "requestId": "0123456789abcdef", "baseManifestSha256": "0" * 64,
                "targetManifestSha256": digest(raw), "changed": [], "removed": []}))
            with self.assertRaises(zanik_assets.DeltaBaseMismatch):
                zanik_assets.apply_delta_tree(delta, assets)
            self.assertEqual((current / "world-map.jpg").read_bytes(), b"same")


if __name__ == "__main__":
    unittest.main()
