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
    recipe_catalog = {
        "schemaVersion": 1,
        "generatedAt": "2026-10-12T00:00:00Z",
        "counts": {"recipes": 100, "dependencyEdges": 0, "cycles": 0},
        "entries": [{"id": f"{index:016x}", "name": f"Recipe {index}"} for index in range(100)],
    }
    recipe_path = web_root / "recipe-unlocks.json"
    recipe_path.write_text(json.dumps(recipe_catalog, separators=(",", ":")), encoding="utf-8")
    data_file = temporary_root / "players.db"
    game_update_file = temporary_root / "game-update-status.json"
    game_update_file.write_text(json.dumps({"schemaVersion": 1, "status": "current",
                                            "installedBuild": "123", "latestBuild": "123",
                                            "updateAvailable": False}), encoding="utf-8")

    env = os.environ.copy()
    env.update(
        LD_PRELOAD=str(LIBRARY),
        DW_STATUS_PORT=str(port),
        DW_STATUS_DATA=str(data_file),
        DW_WEB_ROOT=str(web_root),
        DW_GAME_UPDATE_STATE=str(game_update_file),
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
            assert page.index('class="site-header"') < page.index("<main>")
            assert '>Server</a>' not in page and '>Highscores</a>' in page
        with get(base, "/players") as response:
            page = response.read().decode()
            assert "Known players" in page and 'class="site-header"' in page
        with get(base, "/player?id=test") as response:
            page = response.read().decode()
            assert 'class="player-sidebar"' in page
            assert 'data-player-panel="overview"' in page
            assert 'data-player-panel="combat"' in page
            assert 'data-player-panel="recipes"' in page
            assert 'data-player-panel="quests"' in page
            assert 'id="manageSidebarGroup" class="player-sidebar-group" hidden' in page
            assert '.player-sidebar [hidden]{display:none!important}' in page
        with get(base, "/map") as response:
            page = response.read().decode()
            assert "Live world map" in page
            assert "/api/map/npcs/state" in page
            assert page.index('class="site-header"') < page.index('id="headerContent"')
        with get(base, "/kills") as response:
            page = response.read().decode()
            assert "Highscores" in page
            assert 'class="kills-sidebar"' in page
            assert 'id="combatTab"' in page and 'id="skillsTab"' in page
            assert 'id="enemyList"' in page and "selectScore" in page

        expect_error(base, "/api/quests/catalog", 503, "Quest catalog unavailable")
        expect_error(base, "/api/map/npcs/catalog", 503, "NPC catalog unavailable")
        with get(base, "/api/recipe-unlocks") as response:
            assert json.loads(response.read())["counts"]["recipes"] == 100
        recipe_catalog["counts"]["recipes"] = 101
        recipe_path.write_text(json.dumps(recipe_catalog, separators=(",", ":")), encoding="utf-8")
        expect_error(base, "/api/recipe-unlocks", 503, "Recipe unlock catalog unavailable")
        recipe_catalog["counts"]["recipes"] = 100
        recipe_path.write_text(json.dumps(recipe_catalog, separators=(",", ":")), encoding="utf-8")
        with get(base, "/assets/world-map.jpg") as response:
            assert response.headers.get_content_type() == "image/svg+xml"
            assert b"<svg" in response.read()
        with get(base, "/assets/skills/attack.png") as response:
            assert response.headers.get_content_type() == "image/svg+xml"
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
        with get(base, "/api/update-status") as response:
            update = json.loads(response.read())
            assert update["configuration"]["valid"] is True
            assert update["features"]["playerStatus"] is True
            assert update["features"]["adminTeleport"] is False
            assert update["features"]["adminStorageMutation"] is False
            assert update["features"]["adminGravestoneMutation"] is False
            assert update["features"]["qolAutoCompost"] is False
            assert update["featureStates"]["adminTeleport"] == "disabled"
            assert update["assets"] is None
            assert update["gameUpdate"]["status"] == "current"
            assert update["gameUpdate"]["installedBuild"] == "123"
        with get(base, "/api/quest-tracker-diagnostics") as response:
            diagnostics = json.loads(response.read())
            assert "catalogCompatibility" in diagnostics
            assert "maximumQueryMicroseconds" in diagnostics
        with get(base, "/api/diagnostics/npcs") as response:
            diagnostics = json.loads(response.read())
            assert "catalogCompatibility" in diagnostics
            assert "maximumSliceMicroseconds" in diagnostics
            assert "slowSliceCount" in diagnostics
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
        with get(base, "/api/skills") as response:
            assert response.read() == b'{"skills":[]}'
        with get(base, "/api/skill-leaderboard?skill=Mining") as response:
            assert response.read() == b'{"skill":"Mining","players":[]}'

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
