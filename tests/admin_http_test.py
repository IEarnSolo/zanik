#!/usr/bin/env python3
"""End-to-end coverage for the authenticated admin session manager."""

import http.cookiejar
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


def player_row(guid: str, public_id: str, name: str, last_seen: int) -> str:
    identity = ("character:" + guid).encode().hex()
    return f"P\t{identity}\t{public_id}\t{name.encode().hex()}\t{last_seen}\n"


def request(opener, url: str, *, body=None, headers=None):
    data = None if body is None else json.dumps(body).encode()
    merged = dict(headers or {})
    if data is not None:
        merged["Content-Type"] = "application/json"
    return opener.open(urllib.request.Request(url, data=data, headers=merged), timeout=3)


def expect_http(opener, url: str, status: int, *, body=None):
    try:
        request(opener, url, body=body)
        raise AssertionError(f"Expected HTTP {status} from {url}")
    except urllib.error.HTTPError as error:
        assert error.code == status, error.read()


with tempfile.TemporaryDirectory() as temporary:
    root = pathlib.Path(temporary)
    zanik = root / "zanik"
    data = root / "zanik-data"
    zanik.mkdir()
    data.mkdir()
    first_id = "11111111111111111111111111111111"
    second_id = "22222222222222222222222222222222"
    (zanik / "admins.txt").write_text(f"{first_id} # Existing Admin\n")
    players = data / "players.db"
    players.write_text(
        player_row("aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa", first_id, "Existing Admin", 1704067200)
        + player_row("bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb", second_id, "New Admin", 1704153600)
    )

    with socket.socket() as probe:
        probe.bind(("127.0.0.1", 0))
        port = probe.getsockname()[1]
    base = f"http://127.0.0.1:{port}"
    env = os.environ.copy()
    env.update(
        LD_PRELOAD=str(ROOT / "dist" / "libdwstatus.so"),
        DW_STATUS_PORT=str(port),
        DW_STATUS_DATA=str(players),
        DW_ZANIK_ROOT=str(zanik),
        DW_DATA_ROOT=str(data),
        DW_WEB_ROOT=str(ROOT / "web"),
        DW_STATUS_TEST_HOST="1",
        DW_CONSOLE_SECRET="test-console-secret",
        DW_PUBLIC_BASE_URL=base,
    )
    process = subprocess.Popen(
        ["/bin/sleep", "20"], env=env, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE
    )
    anonymous = urllib.request.build_opener()
    first = urllib.request.build_opener(urllib.request.HTTPCookieProcessor(http.cookiejar.CookieJar()))
    second = urllib.request.build_opener(urllib.request.HTTPCookieProcessor(http.cookiejar.CookieJar()))
    try:
        for _ in range(50):
            try:
                anonymous.open(base, timeout=1).close()
                break
            except (OSError, urllib.error.URLError):
                time.sleep(0.1)
        else:
            raise AssertionError("Admin HTTP listener did not start")

        expect_http(anonymous, base + "/admin/sessions", 401)
        console_request = urllib.request.Request(
            base + "/api/admin/console-claim",
            data=first_id.encode(),
            headers={"X-Zanik-Console-Key": "test-console-secret"},
        )
        with anonymous.open(console_request, timeout=3) as response:
            first_claim = json.loads(response.read())["url"].split("#token=", 1)[1]
        with request(first, base + "/api/admin/claim", body={"token": first_claim}) as response:
            first_login = json.loads(response.read())
        csrf = first_login["csrf"]

        with first.open(base + "/admin/sessions", timeout=3) as response:
            assert b"Admin sessions" in response.read()
        with first.open(base + "/api/admin/link-manager", timeout=3) as response:
            manager = json.loads(response.read())
        assert manager["players"]["count"] == 2
        assert manager["authorizedIds"] == [first_id]
        assert len(manager["sessions"]) == 1 and manager["sessions"][0]["current"] is True

        with request(first, base + "/api/admin/create-link", body={"csrf": csrf, "playerId": second_id}) as response:
            created = json.loads(response.read())
        assert created["authorized"] is True and created["characterName"] == "New Admin"
        assert second_id + " # New Admin" in (zanik / "admins.txt").read_text()
        second_claim = created["url"].split("#token=", 1)[1]
        with request(second, base + "/api/admin/claim", body={"token": second_claim}) as response:
            assert json.loads(response.read())["characterId"] == second_id

        with first.open(base + "/api/admin/link-manager", timeout=3) as response:
            manager = json.loads(response.read())
        assert len(manager["sessions"]) == 2
        target = next(row for row in manager["sessions"] if row["playerId"] == second_id)
        with request(first, base + "/api/admin/revoke-session", body={"csrf": csrf, "session": target["session"]}) as response:
            assert json.loads(response.read()) == {"revoked": True, "current": False}
        expect_http(second, base + "/api/admin/session", 401)
        expect_http(first, base + "/api/admin/create-link", 403, body={"csrf": "wrong", "playerId": second_id})

        with first.open(base + "/api/admin/link-manager", timeout=3) as response:
            current = next(row for row in json.loads(response.read())["sessions"] if row["current"])
        with request(first, base + "/api/admin/revoke-session", body={"csrf": csrf, "session": current["session"]}) as response:
            assert json.loads(response.read()) == {"revoked": True, "current": True}
        expect_http(first, base + "/admin/sessions", 401)
    finally:
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()

print("admin session manager HTTP tests passed")
