#!/usr/bin/env python3
"""Fail clearly when the explicit game-asset test mode lacks local inputs."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
REQUIRED_FILES = (
    "deploy/npc-identities.tsv",
    "deploy/npc-exclusions.tsv",
    "dist/quest-step-analysis.json",
    "dist/quest-step-location-audit.json",
    "reference/recipe-unlocks.json",
    "reference/recipe-unlock-testdude-baseline.json",
    "reference/recipe-unlock-testdude-t1-observation.json",
    "web/buildings/catalog.json",
    "web/buildings/catalog.tsv",
    "web/npc-catalog.json",
    "web/plan-items.tsv",
    "web/quest-catalog.json",
    "web/quest-runtime-catalog.tsv",
    "web/quest-step-locations.json",
    "web/recipe-unlocks.json",
    "web/recipe-runtime-catalog.tsv",
    "web/world-map.jpg",
    "web/lodestone-icon.png",
    "web/Jancient-Regular.ttf",
    "web/skills/attack.png",
    "web/quests/main-quest.svg",
)
REQUIRED_DIRECTORIES = (
    "web/buildings",
    "web/items",
    "web/recipe-icons",
)

missing = [path for path in REQUIRED_FILES if not (ROOT / path).is_file()]
missing.extend(path for path in REQUIRED_DIRECTORIES if not (ROOT / path).is_dir())
if missing:
    listing = "\n".join(f"  - {path}" for path in missing)
    raise SystemExit(
        "Game-asset tests were requested, but their ignored local inputs are missing:\n"
        f"{listing}\n"
        "Run the future mod initialization/extraction step, then retry `make test-assets`."
    )

print("game-asset test inputs: available")
