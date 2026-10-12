#!/usr/bin/env python3
"""Windows-client extraction orchestrator owned by Zanik."""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import re


RECIPE_VERSION = 1
GAME_EXE = Path("RSDragonwilds/Binaries/Win64/RSDragonwilds-Win64-Shipping.exe")


def remove_tree(path: Path) -> None:
    def handle_error(function, target, error):
        if isinstance(error[1], FileNotFoundError):
            return
        raise error[1]
    try:
        shutil.rmtree(path, onerror=handle_error)
    except FileNotFoundError:
        pass


def run(command: list[str], label: str, *, quiet: bool = False) -> None:
    print(f"[Zanik] {label}", flush=True)
    completed = subprocess.run(
        command,
        check=False,
        stdout=subprocess.DEVNULL if quiet else None,
        stderr=subprocess.STDOUT if quiet else None,
    )
    if completed.returncode:
        raise RuntimeError(f"{label} failed with exit code {completed.returncode}")


def locate(root: Path, name: str) -> Path:
    matches = list(root.rglob(name))
    if len(matches) != 1:
        raise RuntimeError(f"expected one {name} in {root}, found {len(matches)}")
    return matches[0]


def referenced_textures(json_root: Path, required: list[str]) -> list[str]:
    names = set(required)
    def visit(value: object, key: str = "") -> None:
        if isinstance(value, dict):
            for child_key, child in value.items(): visit(child, str(child_key))
        elif isinstance(value, list):
            for child in value: visit(child, key)
        elif isinstance(value, str) and key in {"AssetPathName", "ObjectPath", "ObjectName"}:
            match = re.search(r"/(T_(?:Icon|Journal|Skill|Tag|WorldMap)[^/.]*)\.", value, re.I)
            if match: names.add(match.group(1))
    for path in json_root.rglob("*.json"):
        if path.name == ".uas-manifest.json": continue
        try: visit(json.loads(path.read_text(encoding="utf-8")))
        except (OSError, UnicodeError, json.JSONDecodeError): continue
    return sorted(names, key=str.casefold)


def filter_chunks(names: list[str], maximum: int = 18000) -> list[str]:
    chunks: list[list[str]] = [[]]
    length = 20
    for name in names:
        encoded = re.escape(name)
        if chunks[-1] and length + len(encoded) + 1 > maximum:
            chunks.append([]); length = 20
        chunks[-1].append(encoded); length += len(encoded) + 1
    return [r"(?:/|^)" + "(?:" + "|".join(chunk) + r")\.uasset$" for chunk in chunks if chunk]


def extract(args: argparse.Namespace) -> int:
    game = args.game_root.resolve(); output = args.output.resolve(); tools = args.tool_root.resolve()
    paks = game / "RSDragonwilds/Content/Paks"; executable = game / GAME_EXE
    if not paks.is_dir() or not executable.is_file():
        raise ValueError("the selected directory is not a complete Windows Dragonwilds client")
    recipe = json.loads(args.recipe.read_text(encoding="utf-8"))
    if recipe.get("schemaVersion") != RECIPE_VERSION:
        raise ValueError("unsupported extraction recipe")
    uas = locate(tools, "uas.exe")
    mapping = args.usmap_path.resolve()
    if not mapping.is_file(): raise ValueError(f"mapping file does not exist: {mapping}")
    work_parent = args.work_root.resolve() if args.work_root else output.parent
    work_parent.mkdir(parents=True, exist_ok=True)
    if os.name == "nt" and len(str(work_parent)) > 100:
        raise ValueError("the intermediate work root is too deep for cooked asset paths; use a short path such as $env:TEMP\\ZanikAssetWork")
    temporary = args.resume_work.resolve() if args.resume_work else Path(tempfile.mkdtemp(prefix="zanik-client-extract-", dir=work_parent))
    keep = bool(args.keep_work or args.resume_work)
    try:
        json_root = temporary / "json"; texture_root = temporary / "textures"; font_root = temporary / "font"; web = temporary / "web"; internal = temporary / "internal"
        internal.mkdir(parents=True, exist_ok=True)
        common = [str(uas), "-p", str(paks), "-g", recipe["engine"], "-u", str(mapping)]
        if args.resume_work:
            if not json_root.is_dir() or not texture_root.is_dir():
                raise ValueError("resume work directory has no completed JSON and texture exports")
        else:
            run(common + ["-f", recipe["jsonFilter"], "export", "Json", "-b", "-o", str(json_root), "-c", "-r", "-l", str(temporary / "json-export.log")], "Exporting targeted catalog data", quiet=True)
            texture_names = referenced_textures(json_root, recipe["requiredTextureAssets"])
            if not texture_names: raise RuntimeError("no web texture references were discovered in the targeted JSON export")
            for index, expression in enumerate(filter_chunks(texture_names), 1):
                batch = texture_root / f"batch-{index:02d}"
                run(common + ["-f", expression, "export", "Textures", "-o", str(batch), "-c", "-r", "-l", str(temporary / f"texture-export-{index:02d}.log")], f"Exporting targeted web textures ({index})", quiet=True)
            run(common + ["-f", recipe["fontFilter"], "export", "Raw", "-o", str(font_root), "-c", "-r", "-l", str(temporary / "font-export.log")], "Exporting the web font", quiet=True)
        scripts = Path(__file__).resolve().parent; repo = scripts.parent; py = sys.executable
        run([py, str(scripts / "generate_building_catalog.py"), str(json_root), str(texture_root), str(web / "buildings"), "--catalogue-root", str(json_root)], "Generating building catalog")
        run([py, str(scripts / "generate_plan_item_catalog.py"), "--assets", str(json_root), "--icons", str(texture_root), "--web", str(web)], "Generating plan-item catalog")
        cells = next((path for path in json_root.rglob("_Generated_") if path.is_dir() and "L_World" in path.as_posix()), None)
        if cells is None: raise RuntimeError("the targeted export contains no L_World partition cells")
        run([py, str(scripts / "generate_world_catalogs.py"), "npc", "--cells-root", str(cells), "--spawn-root", str(json_root), "--character-root", str(json_root), "--placed-root", str(json_root), "--public-output", str(web / "npc-catalog.json"), "--identities-output", str(web / "npc-identities.tsv"), "--exclusions-output", str(internal / "npc-exclusions.tsv")], "Generating NPC catalog")
        run([py, str(scripts / "generate_world_catalogs.py"), "resource", "--cells-root", str(cells), "--blueprint-root", str(json_root), "--item-root", str(json_root), "--public-output", str(web / "resource-catalog.json"), "--identities-output", str(web / "resource-identities.tsv")], "Generating resource catalog")
        deploy = scripts.parent / "deploy"
        run([py, str(scripts / "generate_quest_catalog.py"), "--source", str(json_root), "--public", str(web / "quest-catalog.json"), "--runtime", str(web / "quest-runtime-catalog.tsv"), "--overrides", str(deploy / "quest-overrides.json")], "Generating quest catalog")
        run([py, str(scripts / "generate_web_assets.py"), "--json-root", str(json_root), "--texture-root", str(texture_root), "--font-root", str(font_root), "--web-root", str(web)], "Resolving web-visible artwork")
        run([py, str(scripts / "generate_recipe_unlock_catalog.py"), "--assets", str(json_root), "--building-assets", str(json_root), "--building-catalog", str(web / "buildings/catalog.json"), "--usmap", str(mapping), "--game-exe", str(executable), "--rules", str(repo / "asset-tools/catalog-rules.json"), "--output", str(internal / "recipe-unlocks-audit.json"), "--docs-dir", str(internal / "docs"), "--web-output", str(web / "recipe-unlocks.json"), "--runtime-web-output", str(web / "recipe-runtime-catalog.tsv"), "--runtime-history-output", str(web / "recipe-runtime-catalog-history.tsv")], "Generating recipe catalog")
        run([py, str(scripts / "analyze_quest_steps.py"), "--source", str(json_root), "--quest-catalog", str(web / "quest-catalog.json"), "--quest-runtime-catalog", str(web / "quest-runtime-catalog.tsv"), "--npc-catalog", str(web / "npc-catalog.json"), "--npc-identities", str(web / "npc-identities.tsv"), "--overrides", str(deploy / "quest-step-overrides.json"), "--analysis-output", str(internal / "quest-step-analysis.json"), "--public-output", str(web / "quest-step-locations.json"), "--coverage-output", str(internal / "quest-step-location-coverage.md")], "Generating quest destination catalog")
        asset_tool = scripts / "zanik_assets.py"
        run([py, str(asset_tool), "validate", "--generated-root", str(web)], "Validating generated asset tree")
        replacement = output.with_name(output.name + ".new")
        if replacement.exists(): remove_tree(replacement)
        shutil.copytree(web, replacement)
        previous = output.with_name(output.name + ".previous")
        if output.exists():
            if previous.exists(): remove_tree(previous)
            output.replace(previous)
        replacement.replace(output)
        print(json.dumps({"output": str(output), "work": str(temporary) if keep else None, "ready": True}, sort_keys=True))
        return 0
    finally:
        if not keep and temporary.exists(): remove_tree(temporary)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--game-root", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--tool-root", type=Path, required=True)
    parser.add_argument("--usmap-path", type=Path, required=True)
    parser.add_argument("--recipe", type=Path, required=True)
    parser.add_argument("--work-root", type=Path)
    parser.add_argument("--keep-work", action="store_true")
    parser.add_argument("--resume-work", type=Path, help=argparse.SUPPRESS)
    return extract(parser.parse_args())


if __name__ == "__main__":
    raise SystemExit(main())
