#!/usr/bin/env python3
"""Exercise the HTTP listener and asset failure boundaries without game files."""

import json
import os
import pathlib
import socket
import subprocess
import tempfile
import time
import urllib.error
import urllib.request


ROOT = pathlib.Path(__file__).resolve().parents[1]
LIBRARY = ROOT / "dist" / "libdwstatus.so"


def get(base, path):
    return urllib.request.urlopen(base + path, timeout=3)


def expect_error(base, path, status, message=None, method=None):
    try:
        get(base, path) if method is None else urllib.request.urlopen(
            urllib.request.Request(base + path, method=method), timeout=3
        )
        raise AssertionError(f"{path} should return HTTP {status}")
    except urllib.error.HTTPError as error:
        assert error.code == status, (path, error.code)
        if message is not None:
            assert json.loads(error.read())["error"] == message


with socket.socket() as probe:
    probe.bind(("127.0.0.1", 0))
    port = probe.getsockname()[1]

with tempfile.TemporaryDirectory(prefix="zanik-http-core-") as temporary:
    temporary_root = pathlib.Path(temporary)
    web_root = temporary_root / "empty-web"
    web_root.mkdir()
    data_file = temporary_root / "players.db"

    env = os.environ.copy()
    env.update(
        LD_PRELOAD=str(LIBRARY),
        DW_STATUS_PORT=str(port),
        DW_STATUS_DATA=str(data_file),
        DW_WEB_ROOT=str(web_root),
        DW_STATUS_TEST_HOST="1",
    )
    process = subprocess.Popen(
        ["/bin/sleep", "20"], env=env,
        stdout=subprocess.DEVNULL, stderr=subprocess.PIPE,
    )
    base = f"http://127.0.0.1:{port}"

    try:
        for _ in range(50):
            try:
                get(base, "/").close()
                break
            except (OSError, urllib.error.URLError):
                time.sleep(0.1)
        else:
            process.terminate()
            stderr = process.communicate(timeout=5)[1].decode(errors="replace")
            raise AssertionError(f"Listener did not start (exit={process.returncode}): {stderr}")

        with get(base, "/") as response:
            page = response.read().decode()
            assert "Dragonwilds Server" in page
            assert "encodeURIComponent(String(player.id))" in page
        with get(base, "/players") as response:
            assert "Known players" in response.read().decode()
        with get(base, "/map") as response:
            page = response.read().decode()
            assert "Live world map" in page
            assert "/api/map/npcs/state" in page

        expect_error(base, "/api/quests/catalog", 503, "Quest catalog unavailable")
        expect_error(base, "/api/map/npcs/catalog", 503, "NPC catalog unavailable")
        expect_error(base, "/assets/world-map.jpg", 503, "Map asset unavailable")
        expect_error(base, "/assets/skills/attack.png", 404, "Skill icon unavailable")
        expect_error(base, "/assets/skills/../attack.png", 404)

        with get(base, "/api/diagnostics/player-skills") as response:
            assert json.loads(response.read()) == {
                "iconCompatibility": -1,
                "iconAssets": 12,
                "missingIconAssets": 12,
            }
        with get(base, "/api/diagnostics/storage-plans") as response:
            diagnostics = json.loads(response.read())
            assert diagnostics["items"] == 0
            assert diagnostics["compatibility"] != 1
        with get(base, "/api/quest-tracker-diagnostics") as response:
            diagnostics = json.loads(response.read())
            assert "catalogCompatibility" in diagnostics
            assert "maximumQueryMicroseconds" in diagnostics
        with get(base, "/api/diagnostics/npcs") as response:
            diagnostics = json.loads(response.read())
            assert "catalogCompatibility" in diagnostics
            assert "maximumSliceMicroseconds" in diagnostics
            assert not any("pointer" in key.lower() or "address" in key.lower() for key in diagnostics)

        for path, message in (
            ("/api/map/players", "Live map unavailable"),
            ("/api/map/lodestones", "Lodestone map unavailable"),
            ("/api/map/labels", "Map labels unavailable"),
        ):
            expect_error(base, path, 503, message)

        with get(base, "/api/players") as response:
            assert response.read() == b'{"count":0,"players":[]}'
        with get(base, "/api/known-players") as response:
            assert response.read() == b'{"count":0,"players":[]}'
        with get(base, "/api/kills") as response:
            assert response.read() == b'{"totalKills":0,"enemies":[]}'

        expect_error(base, "/api/player?id=abc123", 404)
        expect_error(base, "/api/kick", 404)
        expect_error(base, "/api/players", 405, method="POST")
    finally:
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()

print("asset-free HTTP behavior: ok")
