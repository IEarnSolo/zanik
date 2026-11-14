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
    spell_and_perk_dir = web_root / "spells-and-perks"
    spell_and_perk_dir.mkdir()
    (spell_and_perk_dir / "PerkV2_Attack_TempestShield.png").write_bytes(b"perk-icon")
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
        DW_DATA_ROOT=str(temporary_root),
        DW_WEB_ROOT=str(web_root),
        DW_GAME_UPDATE_STATE=str(game_update_file),
        DW_STATUS_TEST_HOST="1",
        ZANIK_FEATURE_QOL_AUTO_COMPOST="1",
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
            assert 'href="/diagnostics"' in page
            assert 'class="site-admin-link" href="/admin/sessions" hidden>Admin</a>' in page
            assert '.site-nav a[hidden]{display:none!important}' in page
            assert 'class="site-scale-select"' in page
            assert 'class="site-admin-state"' in page
            assert "localStorage.getItem('zanik.uiScale')" in page
            assert "window.addEventListener('storage'" in page
            assert "applyScale(event.newValue??1,false)" in page
            assert "document.documentElement.style.fontSize=(value*100)+'%'" in page
            assert "viewport-fit=cover" in page
            assert 'id="zanik-responsive"' in page
            assert "--app-touch-target:44px" in page
            assert "--site-bottom-nav-h" in page
            assert 'class="site-more-toggle"' in page
            assert 'id="siteMorePanel"' in page
            assert "window.ZanikSiteShell={closeMore}" in page
            assert "new CustomEvent('zanik:site-surface'" in page
            assert "new CustomEvent('zanik:admin-session'" in page
            assert '<link rel="manifest" href="/app.webmanifest">' in page
            assert '<meta name="apple-mobile-web-app-capable" content="yes">' in page
            assert '<meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">' in page
            assert 'id="zanik-app-behavior"' in page
            assert "root.dataset.appDisplayMode" in page
            assert "url.origin===location.origin&&(!sameDocument||link.target==='_blank')" in page
            assert "window.location.href=url.pathname+url.search+url.hash" in page
            assert "html{overscroll-behavior:none}" in page
            assert '<link rel="apple-touch-icon" sizes="180x180" href="/app-icon-180.png">' in page
        with get(base, "/app.webmanifest") as response:
            assert response.headers.get_content_type() == "application/manifest+json"
            manifest = json.loads(response.read())
            assert manifest["start_url"] == "/" and manifest["scope"] == "/"
            assert manifest["display"] == "standalone"
            assert manifest["display_override"] == ["fullscreen", "standalone"]
            assert manifest["id"] == "/" and manifest["prefer_related_applications"] is False
            assert manifest["launch_handler"] == {"client_mode": "navigate-existing"}
            assert {icon["sizes"] for icon in manifest["icons"]} == {"192x192", "512x512"}
        for path, size in (("/app-icon-180.png", 180), ("/app-icon-192.png", 192), ("/app-icon-512.png", 512)):
            with get(base, path) as response:
                icon = response.read()
                assert response.headers.get_content_type() == "image/png"
                assert icon[:8] == b"\x89PNG\r\n\x1a\n"
                assert int.from_bytes(icon[16:20], "big") == size
                assert int.from_bytes(icon[20:24], "big") == size
        with get(base, "/players") as response:
            page = response.read().decode()
            assert "Known players" in page and 'class="site-header"' in page
            assert '<link rel="manifest" href="/app.webmanifest">' in page
            assert "Last seen " in page and "Online for " in page
            assert "data-online-since" in page and "updateOnlineDurations" in page
            assert "player-online-dot" in page and "box-shadow: 0 0 .5rem #69db8d88" in page
        with get(base, "/player?id=test") as response:
            page = response.read().decode()
            assert 'class="player-sidebar"' in page
            assert 'data-player-panel="overview"' in page
            assert 'data-player-panel="combat"' in page
            assert 'data-player-panel="recipes"' in page
            assert 'data-player-panel="quests"' in page
            assert 'id="manageSidebarGroup" class="player-sidebar-group" hidden' in page
            assert '.player-sidebar [hidden]{display:none!important}' in page
            assert 'class="overview-grid"' in page
            assert 'id="activityList"' in page and 'id="activityPrevious"' in page and 'id="activityNext"' in page
            assert 'id="activeQuestContent"' in page
            assert "/api/player-activity?id=" in page
            assert "ZanikPolling.add('global-live-status',2000,refreshLive)" in page
        with get(base, "/map") as response:
            page = response.read().decode()
            assert "Live world map" in page
            assert "/api/map/npcs/state" in page
            assert page.index('class="site-header"') < page.index('id="viewport"')
            assert 'id="headerContent"' not in page and 'id="adminState"' not in page
            assert 'id="finderTabs"' in page and 'id="settingsPopover"' in page
            assert 'data-finder-tab="resources"' in page and 'data-finder-tab="quests"' in page
            assert 'id="clearResourceSearch"' in page and 'id="clearNpcSearch"' in page
            assert 'id="clearQuestSearch"' in page and 'trackFocusedQuestTip' in page
        with get(base, "/kills") as response:
            page = response.read().decode()
            assert "Highscores" in page
            assert 'class="kills-sidebar"' in page
            assert 'id="combatTab"' in page and 'id="skillsTab"' in page
            assert 'id="enemyList"' in page and "selectScore" in page
        with get(base, "/diagnostics") as response:
            page = response.read().decode()
            assert "Server diagnostics" in page
            assert 'id="chart"' in page and "/api/server-performance?window=" in page
            assert "Engine tick work" in page and "Player network" in page and "Spike analysis" in page
            assert 'data-sort="recent"' in page and 'data-sort="worst"' in page
            assert 'id="insightPrevious"' in page and 'id="insightNext"' in page
            assert "Player connections" in page and "Network health" in page
            assert "login warm-up" in page and "client packet loss" in page
            assert "ZanikPolling.add('diagnostics',2000,load)" in page and "document.hidden" in page
        with get(base, "/api/server-performance?window=3600") as response:
            performance = json.loads(response.read())
            assert performance["windowSeconds"] == 3600
            assert performance["sampleIntervalSeconds"] == 2
            assert performance["pingJoinGraceSeconds"] == 10
            assert performance["sustainedPingSamples"] == 3
            assert performance["clientPacketLossAvailable"] is False
            assert performance["insightPage"] == 0 and performance["insightPageSize"] == 10
            assert isinstance(performance["insightTotal"], int)
            assert isinstance(performance["players"], list)
            assert isinstance(performance["points"], list) and isinstance(performance["insights"], list)

        # A connection that never sends a request must be identified as a
        # receive timeout rather than the old ambiguous "uncaptured route".
        with socket.create_connection(("127.0.0.1", port), timeout=3):
            time.sleep(2.2)
        with get(base, "/api/server-performance?window=3600") as response:
            timeout_performance = json.loads(response.read())
            timeout_insights = timeout_performance["insights"]
            assert any(item["title"] == "Incomplete web connection · receive timeout"
                       for item in timeout_insights)
            assert all("uncaptured route" not in item["title"] and
                       "uncaptured route" not in item["detail"]
                       for item in timeout_insights)
        expect_error(base, "/api/server-performance?window=7", 400, "Unsupported performance window")
        expect_error(base, "/api/server-performance?window=3600&sort=unknown", 400, "Unsupported spike sort")
        expect_error(base, "/api/server-performance?window=3600&page=invalid", 400, "Invalid spike page")

        expect_error(base, "/api/quests/catalog", 503, "Quest catalog unavailable")
        expect_error(base, "/api/player-activity?id=missing", 404, "Player not found")
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
        for map_lod in ("world-map-1024.jpg", "world-map-2048.jpg"):
            with get(base, f"/assets/{map_lod}") as response:
                assert response.headers.get_content_type() == "image/svg+xml"
                assert b"<svg" in response.read()
        with get(base, "/assets/skills/attack.png") as response:
            assert response.headers.get_content_type() == "image/svg+xml"
        with get(base, "/assets/spells-and-perks/PerkV2_Attack_TempestShield.png") as response:
            assert response.headers.get_content_type() == "image/png"
            assert response.read() == b"perk-icon"
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
            for field in ("playerQuestObjectiveCompatibility", "playerQuestTrackedQuestCompatibility",
                          "playerPingCompatibility", "adminHomeLocationCompatibility",
                          "adminHomePersistenceCompatibility", "adminNativeInventoryMutationCompatibility"):
                assert field in update
        with get(base, "/api/quest-tracker-diagnostics") as response:
            diagnostics = json.loads(response.read())
            assert "catalogCompatibility" in diagnostics
            assert "trackedQuestCaptures" in diagnostics
            assert "maximumQueryMicroseconds" in diagnostics
        with get(base, "/api/diagnostics/npcs") as response:
            diagnostics = json.loads(response.read())
            assert "catalogCompatibility" in diagnostics
            assert "readyNodes" in diagnostics
            assert "authoritativeTimerCompatibility" in diagnostics
            assert "authoritativeRespawnTimers" in diagnostics
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
            assert response.read() == b'{"playerCount":0,"skills":[]}'
        with get(base, "/api/skill-leaderboard") as response:
            assert response.read() == b'{"skill":null,"players":[]}'
        with get(base, "/api/skill-leaderboard?skill=Mining") as response:
            assert response.read() == b'{"skill":"Mining","players":[]}'

        expect_error(base, "/api/player?id=abc123", 404)
        expect_error(base, "/api/kick", 404)
        expect_error(base, "/api/players", 405, method="POST")
        expect_error(base, "/admin/sessions", 401, "Authentication required")
        expect_error(base, "/api/admin/link-manager", 401, "Authentication required")
        expect_error(base, "/api/admin/create-link", 401, "Authentication required", method="POST")
        expect_error(base, "/api/admin/revoke-session", 401, "Authentication required", method="POST")
        expect_error(base, "/api/admin/console-player-ids", 403, "Console authorization required", method="POST")
        expect_error(base, "/api/admin/quest/teleport", 401, "Authentication required", method="POST")
    finally:
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()

print("asset-free HTTP behavior: ok")
