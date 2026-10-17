from __future__ import annotations

import re
from dataclasses import dataclass


_RELEASE_HEADING = re.compile(
    r"^## \[(?P<version>\d+\.\d+\.\d+)\](?:\s+-\s+(?P<date>[^\r\n]+))?\s*$",
    re.MULTILINE,
)


@dataclass(frozen=True)
class ChangelogRelease:
    version: str
    date: str
    changes: str


def version_key(version: str) -> tuple[int, int, int]:
    match = re.fullmatch(r"(\d+)\.(\d+)\.(\d+)", version.strip())
    if not match:
        raise ValueError(f"invalid stable version: {version}")
    return tuple(int(part) for part in match.groups())


def parse_changelog(markdown: str, current_version: str) -> list[ChangelogRelease]:
    """Return stable releases no newer than the running Manager, newest first."""
    current = version_key(current_version)
    headings = list(_RELEASE_HEADING.finditer(markdown))
    releases: list[ChangelogRelease] = []
    for index, heading in enumerate(headings):
        version = heading.group("version")
        if version_key(version) > current:
            continue
        end = headings[index + 1].start() if index + 1 < len(headings) else len(markdown)
        changes = markdown[heading.end():end].strip()
        releases.append(ChangelogRelease(version, (heading.group("date") or "").strip(), changes))
    return sorted(releases, key=lambda release: version_key(release.version), reverse=True)
