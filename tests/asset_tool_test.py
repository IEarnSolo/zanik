import hashlib, json, os, shutil, subprocess, sys, tempfile, zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "tools/zanik_assets.py"
REQUIRED = ("buildings/catalog.json", "buildings/catalog.tsv", "npc-catalog.json", "plan-items.tsv",
            "quest-catalog.json", "quest-runtime-catalog.tsv", "quest-step-locations.json",
            "recipe-unlocks.json", "recipe-runtime-catalog.tsv", "resource-catalog.json",
            "resource-identities.tsv", "npc-identities.tsv", "world-map.jpg", "lodestone-icon.png",
            "nodestone-icon.png", "gravestone-icon.png", "Jancient-Regular.ttf", "skills/attack.png",
            "quests/main-quest.svg")


def fake_jpeg(width, height):
    # Validation reads only the standard SOF0 dimensions; image decoding is not
    # required for these asset-tool tests.
    return (b"\xff\xd8\xff\xc0\x00\x11\x08" + height.to_bytes(2, "big") +
            width.to_bytes(2, "big") + b"\x03\x01\x11\x00\x02\x11\x00\x03\x11\x00\xff\xd9")

with tempfile.TemporaryDirectory() as name:
    root = Path(name); game = root / "game"; generated = game / "zanik-generated-web"; destination = root / "assets"
    paks = game / "RSDragonwilds/Content/Paks"; paks.mkdir(parents=True)
    (paks / "RSDragonwilds-LinuxServer.utoc").write_bytes(b"container")
    for item in REQUIRED:
        path = generated / item; path.parent.mkdir(parents=True, exist_ok=True)
        if item == "world-map.jpg":
            path.write_bytes(fake_jpeg(4096, 4096))
        elif path.suffix == ".tsv":
            path.write_text("# schema=1\tid\n")
        else:
            shapes = {"buildings/catalog.json": ("pieces", 100), "npc-catalog.json": ("types", 10),
                      "quest-catalog.json": ("quests", 10), "quest-step-locations.json": ("quests", 10),
                      "recipe-unlocks.json": ("entries", 100), "resource-catalog.json": ("types", 5)}
            value = {}
            if item in shapes:
                key, count = shapes[item]; value[key] = [{"id": f"{key}-{index}"} for index in range(count)]
            if item in {"npc-catalog.json", "resource-catalog.json"}:
                value["nodes"] = [{"id": f"node-{index}", "x": 0.5, "y": 0.5} for index in range(10)]
            path.write_text(json.dumps(value) + "\n")
    first = root / "one.zip"; second = root / "two.zip"; env = dict(os.environ, SOURCE_DATE_EPOCH="1")
    tool_lock = root / "tools.lock.json"
    tool_lock.write_text(json.dumps({"tools": {
        "python-embed-windows-x86_64": {"version": "3.12.10"},
        "unreal-asset-scout-windows-x86_64": {"version": "0.5.0"},
    }}))
    subprocess.run([sys.executable, str(TOOL), "build", "--game-root", str(game), "--generated-root", str(generated),
                    "--output", str(first), "--tool-versions-file", str(tool_lock)], check=True, env=env)
    subprocess.run([sys.executable, str(TOOL), "build", "--game-root", str(game), "--generated-root", str(generated),
                    "--output", str(second), "--tool-versions-file", str(tool_lock)], check=True, env=env)
    assert hashlib.sha256(first.read_bytes()).digest() == hashlib.sha256(second.read_bytes()).digest()
    subprocess.run([sys.executable, str(TOOL), "validate", "--generated-root", str(generated)], check=True)
    wrong_map = root / "wrong-map"; shutil.copytree(generated, wrong_map)
    (wrong_map / "world-map.jpg").write_bytes(fake_jpeg(6144, 6144))
    rejected_map = subprocess.run([sys.executable, str(TOOL), "validate", "--generated-root", str(wrong_map)], capture_output=True)
    assert rejected_map.returncode and b"expected 4096x4096" in rejected_map.stderr
    subprocess.run([sys.executable, str(TOOL), "validate", "--bundle", str(first)], check=True)
    with zipfile.ZipFile(first) as archive:
        bundled_manifest = json.loads(archive.read("asset-manifest.json"))
    assert bundled_manifest["toolVersions"] == {
        "python-embed-windows-x86_64": "3.12.10",
        "unreal-asset-scout-windows-x86_64": "0.5.0",
    }
    subprocess.run([sys.executable, str(TOOL), "install-bundle", "--bundle", str(first), "--destination", str(destination)], check=True)
    current = destination / "current"; manifest = json.loads((current / "asset-manifest.json").read_text())
    assert manifest["sourceKind"] == "linux-dedicated-server" and not manifest["missingRequired"]
    previous = current.resolve(); bad = root / "bad.zip"
    with zipfile.ZipFile(bad, "w") as archive: archive.writestr("../escape", "bad")
    failed = subprocess.run([sys.executable, str(TOOL), "install", "--bundle", str(bad), "--destination", str(destination)], capture_output=True)
    assert failed.returncode and current.resolve() == previous and not (root / "escape").exists()
    server_extract = subprocess.run([sys.executable, str(TOOL), "extract", "--game-root", str(game), "--output", str(root / "none"), "--source-kind", "server"], capture_output=True)
    assert server_extract.returncode == 4 and b"inventory-only" in server_extract.stdout

    profile = root / "profile.json"
    profile.write_text(json.dumps({"catalogProfile": {"version": "test", "catalogCounts": {"npc-catalog.json:types": 999}}}))
    incompatible = root / "incompatible.zip"
    subprocess.run([sys.executable, str(TOOL), "bundle", "--game-root", str(game), "--generated-root", str(generated),
                    "--output", str(incompatible), "--profile", str(profile)], check=True, env=env)
    subprocess.run([sys.executable, str(TOOL), "validate", "--bundle", str(incompatible)], check=True)
    rejected_drift = subprocess.run([sys.executable, str(TOOL), "install-bundle", "--bundle", str(incompatible),
                                     "--destination", str(root / "drift-assets")], capture_output=True)
    assert rejected_drift.returncode and b"marked incompatible" in rejected_drift.stderr

    parts = root / "parts"
    subprocess.run([sys.executable, str(TOOL), "bundle-set", "--bundle", str(first), "--output-dir", str(parts), "--max-part-bytes", str(1024 * 1024)], check=True)
    set_destination = root / "set-assets"
    subprocess.run([sys.executable, str(TOOL), "install-bundle-set", "--index", str(parts / "bundle-index.json"), "--destination", str(set_destination)], check=True)
    full_hashes = {p.relative_to(current.resolve()).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in current.resolve().rglob("*") if p.is_file()}
    set_current = (set_destination / "current").resolve()
    set_hashes = {p.relative_to(set_current).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in set_current.rglob("*") if p.is_file()}
    assert full_hashes == set_hashes
    corrupt = next(parts.glob("part-*.zip")); corrupt.write_bytes(corrupt.read_bytes() + b"corrupt")
    rejected = subprocess.run([sys.executable, str(TOOL), "install-bundle-set", "--index", str(parts / "bundle-index.json"), "--destination", str(set_destination)], capture_output=True)
    assert rejected.returncode and (set_destination / "current").resolve() == set_current

    replacement_parts = root / "replacement-parts"
    replacement_parts.mkdir(); (replacement_parts / "old.txt").write_text("old")
    subprocess.run([sys.executable, str(TOOL), "bundle-set", "--bundle", str(first),
                    "--output-dir", str(replacement_parts), "--max-part-bytes", str(1024 * 1024),
                    "--replace"], check=True)
    assert (replacement_parts / "bundle-index.json").is_file() and not (replacement_parts / "old.txt").exists()

    work = root / "owned-work"; work.mkdir()
    (work / ".zanik-asset-work.json").write_text(json.dumps({"schemaVersion": 1, "owner": "Zanik asset extractor"}))
    nested = work / "nested"; nested.mkdir(); locked = nested / "readonly.txt"; locked.write_text("temporary")
    locked.chmod(0o400)
    subprocess.run([sys.executable, str(TOOL), "cleanup-work", "--work-root", str(work)], check=True)
    assert not work.exists()
    unowned = root / "unowned"; unowned.mkdir(); (unowned / "keep.txt").write_text("keep")
    refused_cleanup = subprocess.run([sys.executable, str(TOOL), "cleanup-work", "--work-root", str(unowned)],
                                     capture_output=True)
    assert refused_cleanup.returncode and (unowned / "keep.txt").is_file()
print("asset tool tests passed")
