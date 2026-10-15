from __future__ import annotations

import json
import re
import urllib.error
import urllib.request


APP_ID = "4019830"
PUBLIC_INFO_URL = f"https://api.steamcmd.net/v1/info/{APP_ID}"
USER_AGENT = "Zanik-Manager-Game-Update-Checker"
INTERVAL_SECONDS = 10 * 60


def installed_build(appmanifest: bytes) -> str:
    match = re.search(rb'"buildid"\s*"([0-9]+)"', appmanifest)
    if not match:
        raise RuntimeError("The dedicated-server Steam app manifest has no valid build ID")
    return match.group(1).decode("ascii")


class PublicSteamBuildChecker:
    """Read the public SteamCMD metadata mirror without credentials."""

    def __init__(self, opener=None):
        self.opener = opener or urllib.request.urlopen

    def latest_build(self) -> str:
        request = urllib.request.Request(
            PUBLIC_INFO_URL,
            headers={"Accept": "application/json", "User-Agent": USER_AGENT},
        )
        try:
            with self.opener(request, timeout=20) as response:
                payload = json.loads(response.read().decode("utf-8"))
        except urllib.error.HTTPError as error:
            raise RuntimeError(f"Public Steam metadata returned HTTP {error.code}") from error
        except (urllib.error.URLError, TimeoutError, OSError, ValueError, UnicodeDecodeError) as error:
            raise RuntimeError("Public Steam metadata is temporarily unavailable") from error
        try:
            build = str(payload["data"][APP_ID]["depots"]["branches"]["public"]["buildid"])
        except (KeyError, TypeError) as error:
            raise RuntimeError("Public Steam metadata did not contain the dedicated-server branch") from error
        if not build.isdigit():
            raise RuntimeError("Public Steam metadata returned an invalid build ID")
        return build
