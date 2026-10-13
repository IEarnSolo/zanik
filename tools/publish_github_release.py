#!/usr/bin/env python3
"""Publish a validated Zanik release to the public GitHub mirror."""

from __future__ import annotations

import argparse
import hashlib
import http.client
import json
import mimetypes
import os
import sys
import time
from pathlib import Path
from urllib.parse import quote, urlencode, urlsplit

FIXED_FILES = ("SHA256SUMS", "release-manifest.json")
ARCHIVE_PREFIXES = (
    "zanik-server-",
    "zanik-asset-tools-",
    "zanik-manager-",
)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def release_files(directory: Path, version: str) -> list[Path]:
    archives = sorted(
        path for path in directory.iterdir()
        if path.is_file()
        and path.name.startswith(ARCHIVE_PREFIXES)
        and version in path.name
        and path.name.endswith((".zip", ".tar.gz"))
    )
    paths = archives + [directory / name for name in FIXED_FILES]
    missing = [str(path) for path in paths if not path.is_file()]
    if missing or len(archives) != 5 or len(paths) != 7:
        raise ValueError(f"release directory is incomplete: {missing or [path.name for path in archives]}")
    return paths


class GitHubClient:
    def __init__(self, token: str, api_url: str = "https://api.github.com") -> None:
        self.token = token
        self.api_url = api_url.rstrip("/")

    def request(
        self,
        method: str,
        path_or_url: str,
        *,
        body: bytes | Path | None = None,
        content_type: str = "application/json",
    ) -> tuple[int, dict[str, str], bytes]:
        url = path_or_url if "://" in path_or_url else self.api_url + path_or_url
        parsed = urlsplit(url)
        connection_type = http.client.HTTPSConnection if parsed.scheme == "https" else http.client.HTTPConnection
        connection = connection_type(parsed.hostname, parsed.port, timeout=90)
        target = parsed.path + (("?" + parsed.query) if parsed.query else "")
        headers = {
            "Accept": "application/vnd.github+json",
            "Authorization": f"Bearer {self.token}",
            "User-Agent": "Zanik-release-mirror",
            "X-GitHub-Api-Version": "2022-11-28",
        }
        if body is not None:
            headers["Content-Type"] = content_type
            headers["Content-Length"] = str(body.stat().st_size if isinstance(body, Path) else len(body))
        if isinstance(body, Path):
            connection.putrequest(method, target)
            for key, value in headers.items():
                connection.putheader(key, value)
            connection.endheaders()
            with body.open("rb") as stream:
                for block in iter(lambda: stream.read(1024 * 1024), b""):
                    connection.send(block)
        else:
            connection.request(method, target, body=body, headers=headers)
        response = connection.getresponse()
        response_body = response.read()
        response_headers = {key.lower(): value for key, value in response.getheaders()}
        status = response.status
        connection.close()
        return status, response_headers, response_body


def json_body(body: bytes, operation: str) -> dict:
    try:
        value = json.loads(body)
    except (UnicodeDecodeError, json.JSONDecodeError) as error:
        raise RuntimeError(f"GitHub returned invalid JSON while {operation}") from error
    if not isinstance(value, dict):
        raise RuntimeError(f"GitHub returned an invalid response while {operation}")
    return value


def resolve_tag(
    client: GitHubClient,
    repository: str,
    tag: str,
    expected_commit: str,
    *,
    attempts: int = 11,
    delay: float = 30,
) -> None:
    path = f"/repos/{repository}/commits/{quote(tag, safe='')}"
    for attempt in range(attempts):
        status, _, body = client.request("GET", path)
        if status == 200:
            actual = json_body(body, "resolving the release tag").get("sha")
            if actual != expected_commit:
                raise RuntimeError(f"GitHub tag {tag} resolves to a different commit")
            return
        if status == 404 and attempt + 1 < attempts:
            time.sleep(delay)
            continue
        raise RuntimeError(f"GitHub tag lookup failed with HTTP {status}: {tag}")
    raise RuntimeError(f"GitHub tag did not appear: {tag}")


def release_by_tag(client: GitHubClient, repository: str, tag: str) -> dict | None:
    status, _, body = client.request("GET", f"/repos/{repository}/releases/tags/{quote(tag, safe='')}")
    if status == 404:
        return None
    if status != 200:
        raise RuntimeError(f"GitHub release lookup failed with HTTP {status}: {tag}")
    return json_body(body, "reading the release")


def release_payload(tag: str, version: str, commit: str, notes: str) -> bytes:
    return json.dumps({
        "tag_name": tag,
        "target_commitish": commit,
        "name": f"Zanik {version}",
        "body": notes,
        "draft": False,
        "prerelease": False,
        "generate_release_notes": False,
    }, separators=(",", ":")).encode("utf-8")


def ensure_release(
    client: GitHubClient,
    repository: str,
    tag: str,
    version: str,
    commit: str,
    notes: str,
) -> dict:
    payload = release_payload(tag, version, commit, notes)
    existing = release_by_tag(client, repository, tag)
    if existing is None:
        try:
            status, _, body = client.request("POST", f"/repos/{repository}/releases", body=payload)
            if status == 201:
                return json_body(body, "creating the release")
        except (OSError, TimeoutError, http.client.HTTPException):
            status = 0
        # A timeout or competing retry can create the release before this response
        # is useful. Reconcile by tag before reporting a failure.
        existing = release_by_tag(client, repository, tag)
        if existing is None:
            raise RuntimeError(f"GitHub release creation failed with HTTP {status}: {tag}")
    release_id = existing.get("id")
    if not isinstance(release_id, int):
        raise RuntimeError("GitHub release response is missing its numeric ID")
    status, _, body = client.request("PATCH", f"/repos/{repository}/releases/{release_id}", body=payload)
    if status != 200:
        raise RuntimeError(f"GitHub release update failed with HTTP {status}: {tag}")
    return json_body(body, "updating the release")


def assets_for_release(client: GitHubClient, repository: str, release_id: int) -> list[dict]:
    status, _, body = client.request("GET", f"/repos/{repository}/releases/{release_id}/assets?per_page=100")
    if status != 200:
        raise RuntimeError(f"GitHub asset lookup failed with HTTP {status}")
    try:
        assets = json.loads(body)
    except (UnicodeDecodeError, json.JSONDecodeError) as error:
        raise RuntimeError("GitHub returned invalid JSON while listing assets") from error
    if not isinstance(assets, list):
        raise RuntimeError("GitHub returned an invalid asset list")
    return assets


def matching_asset(assets: list[dict], path: Path, digest: str) -> bool:
    same_name = [asset for asset in assets if asset.get("name") == path.name]
    if not same_name:
        return False
    if len(same_name) != 1 or same_name[0].get("digest") != f"sha256:{digest}":
        raise RuntimeError(f"existing GitHub release asset differs from validated file: {path.name}")
    return True


def upload_asset(
    client: GitHubClient,
    repository: str,
    release: dict,
    path: Path,
    digest: str,
    *,
    attempts: int = 3,
    delay: float = 2,
) -> None:
    release_id = release.get("id")
    if not isinstance(release_id, int):
        raise RuntimeError("GitHub release response is missing its numeric ID")
    if matching_asset(assets_for_release(client, repository, release_id), path, digest):
        print(f"kept identical existing GitHub asset {path.name}")
        return
    template = release.get("upload_url")
    if not isinstance(template, str):
        raise RuntimeError("GitHub release response is missing its upload URL")
    upload_url = template.split("{", 1)[0] + "?" + urlencode({"name": path.name})
    media_type = mimetypes.guess_type(path.name)[0] or "application/octet-stream"
    for attempt in range(attempts):
        ambiguous = False
        try:
            status, headers, _ = client.request("POST", upload_url, body=path, content_type=media_type)
            if status == 201:
                print(f"published GitHub asset {path.name}")
                return
            ambiguous = status in {422, 429} or status >= 500
            retryable = status == 429 or status >= 500
            retry_after = headers.get("retry-after")
        except (OSError, TimeoutError, http.client.HTTPException):
            ambiguous = True
            retryable = True
            retry_after = None
        if ambiguous:
            if matching_asset(assets_for_release(client, repository, release_id), path, digest):
                print(f"confirmed GitHub asset after an ambiguous upload: {path.name}")
                return
            if retryable and attempt + 1 < attempts:
                wait = float(retry_after) if retry_after and retry_after.isdigit() else delay
                time.sleep(min(wait, 60))
                continue
        raise RuntimeError(f"GitHub asset upload failed: {path.name}")
    raise RuntimeError(f"GitHub asset upload remained ambiguous: {path.name}")


def publish(
    directory: Path,
    version: str,
    tag: str,
    commit: str,
    notes_path: Path,
    repository: str,
    client: GitHubClient,
    *,
    tag_attempts: int = 11,
    tag_delay: float = 30,
) -> dict:
    paths = release_files(directory, version)
    notes = notes_path.read_text(encoding="utf-8")
    resolve_tag(client, repository, tag, commit, attempts=tag_attempts, delay=tag_delay)
    release = ensure_release(client, repository, tag, version, commit, notes)
    records = []
    for path in paths:
        digest = sha256(path)
        upload_asset(client, repository, release, path, digest)
        records.append({"name": path.name, "size": path.stat().st_size, "sha256": digest})
    return {
        "schemaVersion": 1,
        "status": "published",
        "repository": repository,
        "tag": tag,
        "commit": commit,
        "releaseUrl": release.get("html_url"),
        "assets": records,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--directory", type=Path, required=True)
    parser.add_argument("--version", required=True)
    parser.add_argument("--tag", required=True)
    parser.add_argument("--commit", required=True)
    parser.add_argument("--notes", type=Path, required=True)
    parser.add_argument("--repository", default="IEarnSolo/zanik")
    parser.add_argument("--report", type=Path, required=True)
    parser.add_argument("--api-url", default="https://api.github.com")
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    try:
        paths = release_files(args.directory, args.version)
        args.notes.read_text(encoding="utf-8")
        if args.dry_run:
            result = {
                "schemaVersion": 1,
                "status": "validated",
                "repository": args.repository,
                "tag": args.tag,
                "commit": args.commit,
                "assets": [
                    {"name": path.name, "size": path.stat().st_size, "sha256": sha256(path)}
                    for path in paths
                ],
            }
        else:
            token = os.environ["GITHUB_RELEASE_TOKEN"]
            result = publish(
                args.directory,
                args.version,
                args.tag,
                args.commit,
                args.notes,
                args.repository,
                GitHubClient(token, args.api_url),
            )
        args.report.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        return 0
    except (KeyError, OSError, ValueError, RuntimeError) as error:
        try:
            args.report.write_text(json.dumps({
                "schemaVersion": 1,
                "status": "failed",
                "repository": args.repository,
                "tag": args.tag,
                "commit": args.commit,
                "error": str(error),
            }, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        except OSError:
            pass
        print(f"GitHub publication failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
