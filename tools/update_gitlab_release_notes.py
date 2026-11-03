#!/usr/bin/env python3
"""Update an existing GitLab release description from the canonical changelog."""

from __future__ import annotations

import argparse
import http.client
import json
import os
from pathlib import Path
import sys
from urllib.parse import quote, urlsplit

from release_contract import release_notes, validate


class GitLabClient:
    def __init__(self, api_url: str, project_id: str, token: str) -> None:
        self.api_url = api_url.rstrip("/")
        self.project_id = project_id
        self.token = token

    def request(self, method: str, path: str, body: bytes | None = None) -> tuple[int, bytes]:
        parsed = urlsplit(self.api_url)
        connection_type = http.client.HTTPSConnection if parsed.scheme == "https" else http.client.HTTPConnection
        connection = connection_type(parsed.hostname, parsed.port, timeout=60)
        target = parsed.path.rstrip("/") + f"/projects/{quote(self.project_id, safe='')}/{path.lstrip('/')}"
        headers = {"JOB-TOKEN": self.token, "Accept": "application/json"}
        if body is not None:
            headers.update({"Content-Type": "application/json", "Content-Length": str(len(body))})
        connection.request(method, target, body=body, headers=headers)
        response = connection.getresponse()
        payload = response.read()
        status = response.status
        connection.close()
        return status, payload


def response_json(body: bytes, operation: str) -> dict:
    try:
        value = json.loads(body)
    except (UnicodeDecodeError, json.JSONDecodeError) as error:
        raise RuntimeError(f"GitLab returned invalid JSON while {operation}") from error
    if not isinstance(value, dict):
        raise RuntimeError(f"GitLab returned an invalid response while {operation}")
    return value


def update_release_notes(client: GitLabClient, tag: str, version: str, notes: str) -> dict:
    encoded = quote(tag, safe="")
    status, body = client.request("GET", f"releases/{encoded}")
    if status == 404:
        raise RuntimeError(f"GitLab release does not exist: {tag}")
    if status != 200:
        raise RuntimeError(f"GitLab release lookup failed with HTTP {status}: {tag}")
    release = response_json(body, "reading the release")
    if release.get("tag_name") != tag:
        raise RuntimeError(f"GitLab release tag response does not match: {tag}")
    payload = json.dumps({"name": f"Zanik {version}", "description": notes}, separators=(",", ":")).encode()
    status, body = client.request("PUT", f"releases/{encoded}", payload)
    if status != 200:
        raise RuntimeError(f"GitLab release-note update failed with HTTP {status}: {tag}")
    updated = response_json(body, "updating release notes")
    if updated.get("tag_name") != tag or updated.get("name") != f"Zanik {version}":
        raise RuntimeError(f"GitLab returned a mismatched release after updating notes: {tag}")
    return {"schemaVersion": 1, "status": "updated", "tag": tag,
            "releaseUrl": updated.get("_links", {}).get("self") or updated.get("url")}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--version", required=True)
    parser.add_argument("--tag", required=True)
    parser.add_argument("--changelog", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    try:
        validate(args.tag, args.version)
        notes = release_notes(args.changelog, args.version)
        result = ({"schemaVersion": 1, "status": "validated", "tag": args.tag}
                  if args.dry_run else
                  update_release_notes(
                      GitLabClient(os.environ["CI_API_V4_URL"], os.environ["CI_PROJECT_ID"],
                                   os.environ["CI_JOB_TOKEN"]),
                      args.tag, args.version, notes))
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        return 0
    except (KeyError, OSError, ValueError, RuntimeError) as error:
        print(f"GitLab release-note update failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
