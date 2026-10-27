#!/usr/bin/env python3
"""Exercise public player activity pagination, validation, and privacy."""

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


def hex_text(value):
    return value.encode("utf-8").hex()


with socket.socket() as probe:
    probe.bind(("127.0.0.1", 0))
    port = probe.getsockname()[1]

with tempfile.TemporaryDirectory(prefix="zanik-player-activity-http-") as temporary:
    root = pathlib.Path(temporary)
    web = root / "web"
    data = root / "data"
    web.mkdir()
    data.mkdir()
    players = data / "players.db"
    players.write_text(
        "P\t" + hex_text("character:hero") + "\tpublic-hero\t" + hex_text("Hero") + "\t123\n" +
        "P\t" + hex_text("character:empty") + "\tpublic-empty\t" + hex_text("Empty") + "\t123\n",
        encoding="utf-8",
    )
    activity = data / "player-activity.db"
    rows = ["N\t56"]
    for event_id in range(1, 56):
        rows.append(
            f"E\t{event_id}\thero\t{1000 + event_id}\t{hex_text('skill_level')}\t"
            f"{hex_text('Event ' + str(event_id))}\t{hex_text('<safe>')}\t{hex_text('/assets/skills/attack.png')}"
        )
    activity.write_text("\n".join(rows) + "\n", encoding="utf-8")
    os.chmod(activity, 0o600)

    env = os.environ.copy()
    env.update(
        LD_PRELOAD=str(LIBRARY),
        DW_STATUS_PORT=str(port),
        DW_STATUS_DATA=str(players),
        DW_DATA_ROOT=str(data),
        DW_WEB_ROOT=str(web),
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
                urllib.request.urlopen(base + "/", timeout=3).close()
                break
            except (OSError, urllib.error.URLError):
                time.sleep(0.1)
        else:
            raise AssertionError("Listener did not start")

        def fetch(path):
            with urllib.request.urlopen(base + path, timeout=3) as response:
                return json.loads(response.read())

        page = fetch("/api/player-activity?id=public-hero")
        assert [event["id"] for event in page["events"]] == list(range(55, 47, -1))
        assert page["hasMore"] is True and page["nextBefore"] == 48
        assert set(page["events"][0]) == {"id", "type", "timestamp", "title", "detail", "icon"}
        encoded = json.dumps(page)
        assert "characterGuid" not in encoded and "character:hero" not in encoded and '"hero"' not in encoded

        older = fetch("/api/player-activity?id=public-hero&limit=3&before=48")
        assert [event["id"] for event in older["events"]] == [47, 46, 45]
        bounded = fetch("/api/player-activity?id=public-hero&limit=999")
        assert len(bounded["events"]) == 50 and bounded["hasMore"] is True
        empty = fetch("/api/player-activity?id=public-empty")
        assert empty["events"] == [] and empty["hasMore"] is False and empty["nextBefore"] is None

        for query in ("limit=0", "limit=-1", "limit=nope", "before=-1", "before=999999999999999999999"):
            try:
                urllib.request.urlopen(base + "/api/player-activity?id=public-hero&" + query, timeout=3)
                raise AssertionError(query + " should be rejected")
            except urllib.error.HTTPError as error:
                assert error.code == 400
    finally:
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()

print("player activity HTTP tests passed")
