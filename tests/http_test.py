#!/usr/bin/env python3
"""Smoke test the preload listener without a Dragonwilds process."""

import json
import os
import pathlib
import socket
import subprocess
import time
import urllib.error
import urllib.request

root = pathlib.Path(__file__).resolve().parents[1]
library = root / "dist" / "libdwstatus.so"
with socket.socket() as probe:
    probe.bind(("127.0.0.1", 0))
    port = probe.getsockname()[1]

env = os.environ.copy()
data_file = root / "dist" / "http-test-players.db"
data_file.unlink(missing_ok=True)
env.update(LD_PRELOAD=str(library), DW_STATUS_PORT=str(port), DW_STATUS_DATA=str(data_file),
           DW_WEB_ROOT=str(root / "web"), DW_STATUS_TEST_HOST="1")
process = subprocess.Popen(["/bin/sleep", "20"], env=env,
                           stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
base = f"http://127.0.0.1:{port}"

try:
    for _ in range(50):
        try:
            urllib.request.urlopen(base, timeout=1).close()
            break
        except (OSError, urllib.error.URLError):
            time.sleep(0.1)
    else:
        process.terminate()
        stderr = process.communicate(timeout=5)[1].decode(errors="replace")
        raise AssertionError(f"Listener did not start (exit={process.returncode}): {stderr}")

    with urllib.request.urlopen(base, timeout=3) as response:
        page = response.read().decode()
        assert "Dragonwilds Server" in page
        assert "encodeURIComponent(String(player.id))" in page
        assert page.index('class="site-header"') < page.index("<main>")
        assert 'class="site-nav"' in page

    with urllib.request.urlopen(base + "/players", timeout=3) as response:
        page = response.read().decode()
        assert "Known players" in page

    with urllib.request.urlopen(base + "/player?id=test", timeout=3) as response:
        page = response.read().decode()
        assert "skill-icon-wrap" in page
        assert "/assets/skills/" in page
        assert 'class="player-sidebar"' in page
        assert 'data-player-view="quests"' in page
        assert 'data-player-panel="quests"' in page
        assert "selectPlayerView" in page
        assert 'class="overview-grid"' in page
        assert 'id="activityList"' in page
        assert 'id="activityPrevious"' in page and 'id="activityNext"' in page
        assert 'id="activeQuestContent"' in page
        assert "/api/player-activity?id=" in page

    with urllib.request.urlopen(base + "/api/quests/catalog", timeout=3) as response:
        quest_catalog = json.loads(response.read())
        assert len(quest_catalog["quests"]) >= 20
        assert len(quest_catalog["fingerprint"]) == 64

    with urllib.request.urlopen(base + "/assets/quests/main-quest.png", timeout=3) as response:
        assert response.headers.get_content_type() == "image/png"
        assert len(response.read()) > 100

    with urllib.request.urlopen(base + "/api/quest-tracker-diagnostics", timeout=3) as response:
        quest_diagnostics = json.loads(response.read())
        assert "catalogCompatibility" in quest_diagnostics
        assert "trackedQuestCompatibility" in quest_diagnostics
        assert "trackedQuestCaptures" in quest_diagnostics
        assert "maximumQueryMicroseconds" in quest_diagnostics

    with urllib.request.urlopen(base + "/assets/skills/attack.png", timeout=3) as response:
        assert response.headers.get_content_type() == "image/png"
        assert len(response.read()) > 5000

    with urllib.request.urlopen(base + "/api/diagnostics/player-skills", timeout=3) as response:
        skill_diagnostics = json.loads(response.read())
        assert skill_diagnostics == {"iconCompatibility": 1, "iconAssets": 12, "missingIconAssets": 0}

    with urllib.request.urlopen(base + "/api/diagnostics/storage-plans", timeout=3) as response:
        plan_diagnostics = json.loads(response.read())
        assert plan_diagnostics["items"] == 123
        assert "compatibility" in plan_diagnostics

    try:
        urllib.request.urlopen(base + "/assets/skills/../attack.png", timeout=3)
        raise AssertionError("Skill asset traversal should be rejected")
    except urllib.error.HTTPError as error:
        assert error.code == 404

    with urllib.request.urlopen(base + "/kills", timeout=3) as response:
        page = response.read().decode()
        assert "Highscores" in page
        assert 'class="kills-sidebar"' in page
        assert 'id="combatTab"' in page and 'id="skillsTab"' in page
        assert "/api/skill-leaderboard?skill=" in page

    with urllib.request.urlopen(base + "/map", timeout=3) as response:
        page = response.read().decode()
        assert "Live world map" in page
        assert "setInterval(refresh,2000)" in page
        assert "dragonwilds.map.lodestones" in page
        assert "/api/map/lodestones" in page
        assert "/api/map/labels" in page
        assert "dragonwilds.map.npcs" in page
        assert "/api/map/npcs/state" in page

    with urllib.request.urlopen(base + "/api/map/npcs/catalog", timeout=3) as response:
        npc_catalog = json.loads(response.read())
        assert npc_catalog["nodes"] and npc_catalog["types"]
        assert "contentFingerprint" in npc_catalog

    with urllib.request.urlopen(base + "/api/diagnostics/npcs", timeout=3) as response:
        npc_diagnostics = json.loads(response.read())
        assert "catalogCompatibility" in npc_diagnostics
        assert "inactiveNodes" in npc_diagnostics
        assert "loadedSpawnPoints" in npc_diagnostics
        assert "readyNodes" in npc_diagnostics
        assert "authoritativeTimerCompatibility" in npc_diagnostics
        assert "authoritativeRespawnTimers" in npc_diagnostics
        assert "lastGenerationWorkMicroseconds" in npc_diagnostics
        assert "lastFinalizationMicroseconds" in npc_diagnostics
        assert "classCacheHits" in npc_diagnostics
        assert "invalidObjectSlots" in npc_diagnostics
        assert npc_diagnostics["idleMilliseconds"] >= 0
        assert not any("pointer" in key.lower() or "address" in key.lower() for key in npc_diagnostics)

    with urllib.request.urlopen(base + "/assets/world-map.jpg", timeout=3) as response:
        assert response.headers.get_content_type() == "image/jpeg"
        assert len(response.read()) > 100000

    with urllib.request.urlopen(base + "/assets/lodestone-icon.png", timeout=3) as response:
        assert response.headers.get_content_type() == "image/png"
        assert len(response.read()) > 10000

    with urllib.request.urlopen(base + "/assets/Jancient-Regular.ttf", timeout=3) as response:
        assert response.headers.get_content_type() == "font/ttf"
        assert len(response.read()) > 100000

    try:
        urllib.request.urlopen(base + "/api/map/players", timeout=3)
        raise AssertionError("Map positions should be unavailable outside the game")
    except urllib.error.HTTPError as error:
        assert error.code == 503
        assert json.loads(error.read())["error"] == "Live map unavailable"

    try:
        urllib.request.urlopen(base + "/api/map/lodestones", timeout=3)
        raise AssertionError("Lodestones should be unavailable outside the game")
    except urllib.error.HTTPError as error:
        assert error.code == 503
        assert json.loads(error.read())["error"] == "Lodestone map unavailable"

    try:
        urllib.request.urlopen(base + "/api/map/labels", timeout=3)
        raise AssertionError("Map labels should be unavailable outside the game")
    except urllib.error.HTTPError as error:
        assert error.code == 503
        assert json.loads(error.read())["error"] == "Map labels unavailable"

    with urllib.request.urlopen(base + "/player?id=abc123", timeout=3) as response:
        page = response.read().decode()
        assert "Current skill levels" in page
        assert "levelNumber.textContent = String(skill.level)" in page
        assert "Current location:" in page
        assert "Last known location:" in page

    with urllib.request.urlopen(base + "/api/players", timeout=3) as response:
        assert response.read() == b'{"count":0,"players":[]}'

    with urllib.request.urlopen(base + "/api/known-players", timeout=3) as response:
        assert response.read() == b'{"count":0,"players":[]}'

    with urllib.request.urlopen(base + "/api/kills", timeout=3) as response:
        assert response.read() == b'{"totalKills":0,"enemies":[]}'

    with urllib.request.urlopen(base + "/api/kill-leaderboard", timeout=3) as response:
        assert response.read() == b'{"enemy":null,"players":[]}'

    with urllib.request.urlopen(base + "/api/skills", timeout=3) as response:
        assert response.read() == b'{"playerCount":0,"skills":[]}'

    with urllib.request.urlopen(base + "/api/skill-leaderboard", timeout=3) as response:
        assert response.read() == b'{"skill":null,"players":[]}'

    with urllib.request.urlopen(base + "/api/skill-leaderboard?skill=Mining", timeout=3) as response:
        assert response.read() == b'{"skill":"Mining","players":[]}'

    with urllib.request.urlopen(base + "/api/kill-tracker-diagnostics", timeout=3) as response:
        diagnostics = json.loads(response.read())
        assert set(diagnostics) == {"hookCompatibility", "regionCompatibility", "rawFatalEvents", "acceptedKills",
                                    "duplicateEvents", "unresolvedAttackers",
                                    "rejectedNonPlayerDeaths", "droppedQueueEvents",
                                    "maximumHookMicroseconds", "slowHookEvents"}

    with urllib.request.urlopen(base + "/api/update-status", timeout=3) as response:
        update = json.loads(response.read())
        assert update["website"] is True
        assert {
            "website", "playerDataCompatibility", "killHookCompatibility",
            "regionCompatibility", "mapProjectionCompatibility",
            "lodestoneCompatibility", "resourceCompatibility",
            "npcCatalogCompatibility", "playerRecipeCompatibility",
            "playerQuestCompatibility", "adminAuthorizationStorage",
            "adminTeleportCompatibility", "adminStorageCompatibility",
            "adminGravestoneCompatibility", "updater",
        } <= set(update)
        assert not any("pointer" in key.lower() or "address" in key.lower() for key in update)

    with urllib.request.urlopen(base + "/api/lodestone-diagnostics", timeout=3) as response:
        diagnostics = json.loads(response.read())
        assert set(diagnostics) == {"compatibility", "registryFound", "registryCount",
                                    "validMarkers", "rejectedRecords", "lastSuccessfulCollection"}

    try:
        urllib.request.urlopen(base + "/api/player?id=abc123", timeout=3)
        raise AssertionError("Unknown player should not exist")
    except urllib.error.HTTPError as error:
        assert error.code == 404

    try:
        urllib.request.urlopen(base + "/api/kick", timeout=3)
        raise AssertionError("Admin route should not exist")
    except urllib.error.HTTPError as error:
        assert error.code == 404

    try:
        urllib.request.urlopen(urllib.request.Request(base + "/api/players", method="POST"), timeout=3)
        raise AssertionError("Mutation method should not be served")
    except urllib.error.HTTPError as error:
        assert error.code == 405
finally:
    process.terminate()
    try:
        process.wait(timeout=5)
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait()
    data_file.unlink(missing_ok=True)
