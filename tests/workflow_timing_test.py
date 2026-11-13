#!/usr/bin/env python3
import json
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "tools/workflow_timing.py"

with tempfile.TemporaryDirectory() as temporary:
    output = Path(temporary) / "timing.json"
    run = lambda *arguments: subprocess.run(
        ["python3", str(TOOL), *arguments, "--output", str(output)],
        text=True, capture_output=True)
    assert run("start", "--workflow", "game compatibility update").returncode == 0
    assert run("phase", "--name", "build discovery").returncode == 0
    assert run("phase", "--name", "native analysis").returncode == 0
    assert run("finish", "--status", "complete").returncode == 0
    value = json.loads(output.read_text())
    assert value["status"] == "complete" and value["finishedAt"]
    assert [phase["name"] for phase in value["phases"]] == ["build discovery", "native analysis"]
    assert all(phase["durationMilliseconds"] >= 0 for phase in value["phases"])
    assert run("phase", "--name", "too late").returncode != 0
    assert run("start", "--workflow", "overwrite").returncode != 0
