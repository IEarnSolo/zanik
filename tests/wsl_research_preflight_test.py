#!/usr/bin/env python3
import hashlib
import json
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "tools/verify_wsl_research.py"

with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary); wsl = root / "wsl"; windows = root / "steamapps/common/Game"
    (wsl / "steamapps").mkdir(parents=True); windows.mkdir(parents=True)
    (windows.parent.parent / "appmanifest_1374490.acf").write_text('"buildid" "200"')
    (wsl / "steamapps/appmanifest_1374490.acf").write_text('"buildid" "200"')
    relative = Path("Game/Content/input.bin")
    (windows / relative).parent.mkdir(parents=True); (wsl / relative).parent.mkdir(parents=True)
    (windows / relative).write_bytes(b"same"); (wsl / relative).write_bytes(b"same")
    server = root / "build-100/server"; server.parent.mkdir(); server.write_bytes(b"server")
    output = root / "evidence.json"; digest = hashlib.sha256(b"server").hexdigest()
    command = ["python3", str(TOOL), "--wsl-client-root", str(wsl),
               "--windows-client-root", str(windows), "--wsl-server-executable", str(server),
               "--expected-server-sha256", digest, "--latest-server-build", "100",
               "--latest-client-build", "200", "--research-input", relative.as_posix(),
               "--output", str(output)]
    assert subprocess.run(command, capture_output=True).returncode == 0
    assert json.loads(output.read_text())["compatible"] is True
    (wsl / "steamapps/appmanifest_1374490.acf").write_text('"buildid" "199"')
    assert subprocess.run(command, capture_output=True).returncode == 3
    stale = json.loads(output.read_text())
    assert stale["checks"]["wslClientCurrent"] is False and stale["compatible"] is False
    (wsl / "steamapps/appmanifest_1374490.acf").write_text('"buildid" "200"')
    (wsl / relative).write_bytes(b"different")
    assert subprocess.run(command, capture_output=True).returncode == 3
    assert json.loads(output.read_text())["mismatches"][0]["reason"] == "sha256-mismatch"
