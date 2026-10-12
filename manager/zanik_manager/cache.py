from __future__ import annotations

import json
import shutil
from dataclasses import dataclass
from pathlib import Path

from .config import content_hash


@dataclass(frozen=True)
class CachedBundle:
    path: Path
    build: str
    manifest_hash: str
    pinned: bool
    created_at: str


class BundleCache:
    def __init__(self, root: Path):
        self.root = root
        self.root.mkdir(parents=True, exist_ok=True)

    def add(self, bundle: Path, game_root_id: str, manifest: dict) -> CachedBundle:
        build = str(manifest.get("gameBuild") or manifest.get("clientBuild") or "unknown")
        raw = json.dumps(manifest, sort_keys=True, separators=(",", ":"))
        digest = content_hash(raw)
        destination = self.root / game_root_id / build / digest
        destination.mkdir(parents=True, exist_ok=True)
        target = destination / bundle.name
        shutil.copy2(bundle, target)
        metadata = {"build": build, "manifestHash": digest, "pinned": False,
                    "createdAt": manifest.get("generatedAt", ""), "bundle": bundle.name}
        (destination / "cache.json").write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
        self.prune(game_root_id)
        return CachedBundle(target, build, digest, False, metadata["createdAt"])

    def list(self, game_root_id: str) -> list[CachedBundle]:
        result = []
        base = self.root / game_root_id
        for metadata_path in base.glob("*/*/cache.json") if base.exists() else []:
            data = json.loads(metadata_path.read_text(encoding="utf-8"))
            result.append(CachedBundle(metadata_path.parent / data["bundle"], data["build"],
                                       data["manifestHash"], bool(data.get("pinned")), data.get("createdAt", "")))
        return sorted(result, key=lambda item: (item.created_at, item.build), reverse=True)

    def pin(self, item: CachedBundle, pinned: bool) -> None:
        path = item.path.parent / "cache.json"
        data = json.loads(path.read_text(encoding="utf-8")); data["pinned"] = pinned
        path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")

    def prune(self, game_root_id: str) -> None:
        unpinned = [item for item in self.list(game_root_id) if not item.pinned]
        for item in unpinned[2:]:
            shutil.rmtree(item.path.parent)
