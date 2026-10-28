#!/usr/bin/env python3
"""Update an existing GitHub release description from the canonical changelog."""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import sys

from publish_github_release import GitHubClient, json_body, release_by_tag
from release_contract import release_notes, validate


def update_release_notes(client: GitHubClient, repository: str, tag: str,
                         version: str, notes: str) -> dict:
    release = release_by_tag(client, repository, tag)
    if release is None:
        raise RuntimeError(f"GitHub release does not exist: {tag}")
    release_id = release.get("id")
    if not isinstance(release_id, int):
        raise RuntimeError("GitHub release response is missing its numeric ID")
    payload = json.dumps({
        "name": f"Zanik {version}",
        "body": notes,
    }, separators=(",", ":")).encode("utf-8")
    status, _, body = client.request(
        "PATCH", f"/repos/{repository}/releases/{release_id}", body=payload)
    if status != 200:
        raise RuntimeError(f"GitHub release-note update failed with HTTP {status}: {tag}")
    updated = json_body(body, "updating release notes")
    return {
        "schemaVersion": 1,
        "status": "updated",
        "repository": repository,
        "tag": tag,
        "releaseUrl": updated.get("html_url") or release.get("html_url"),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--version", required=True)
    parser.add_argument("--tag", required=True)
    parser.add_argument("--changelog", type=Path, required=True)
    parser.add_argument("--repository", default="IEarnSolo/zanik")
    parser.add_argument("--report", type=Path, required=True)
    parser.add_argument("--api-url", default="https://api.github.com")
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    try:
        validate(args.tag, args.version)
        notes = release_notes(args.changelog, args.version)
        result = ({"schemaVersion": 1, "status": "validated", "repository": args.repository,
                   "tag": args.tag}
                  if args.dry_run else
                  update_release_notes(GitHubClient(os.environ["GITHUB_RELEASE_TOKEN"], args.api_url),
                                       args.repository, args.tag, args.version, notes))
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        return 0
    except (KeyError, OSError, ValueError, RuntimeError) as error:
        print(f"GitHub release-note update failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
