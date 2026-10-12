#!/usr/bin/env python3
"""Idempotently publish validated release files to GitLab Generic Packages."""

from __future__ import annotations

import argparse
import hashlib
import http.client
import json
import os
from pathlib import Path
from urllib.parse import quote, urlsplit

FILES = ("SHA256SUMS", "release-manifest.json")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def connection(url: str):
    parsed = urlsplit(url)
    cls = http.client.HTTPSConnection if parsed.scheme == "https" else http.client.HTTPConnection
    return cls(parsed.hostname, parsed.port, timeout=60), parsed


def remote_hash(url: str, token: str) -> tuple[int, str]:
    conn, parsed = connection(url)
    conn.request("GET", parsed.path, headers={"JOB-TOKEN": token})
    response = conn.getresponse()
    digest = hashlib.sha256()
    while True:
        block = response.read(1024 * 1024)
        if not block:
            break
        digest.update(block)
    status = response.status
    conn.close()
    return status, digest.hexdigest()


def upload(url: str, token: str, path: Path) -> None:
    conn, parsed = connection(url)
    conn.putrequest("PUT", parsed.path)
    conn.putheader("JOB-TOKEN", token)
    conn.putheader("Content-Type", "application/octet-stream")
    conn.putheader("Content-Length", str(path.stat().st_size))
    conn.endheaders()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            conn.send(block)
    response = conn.getresponse()
    detail = response.read(500).decode("utf-8", errors="replace")
    status = response.status
    conn.close()
    if status not in {200, 201}:
        raise RuntimeError(f"GitLab package upload failed with HTTP {status}: {detail}")


def release_files(directory: Path, version: str) -> list[Path]:
    archives = sorted(directory.glob(f"zanik-*{version}.*"))
    paths = archives + [directory / name for name in FILES]
    missing = [str(path) for path in paths if not path.is_file()]
    if missing or len(archives) != 5:
        raise ValueError(f"release directory is incomplete: {missing or archives}")
    return paths


def publish(directory: Path, version: str, api_url: str, project_id: str, token: str) -> list[dict[str, str]]:
    base = f"{api_url.rstrip('/')}/projects/{quote(project_id, safe='')}/packages/generic/zanik/{version}"
    links = []
    for path in release_files(directory, version):
        url = f"{base}/{quote(path.name)}"
        status, existing = remote_hash(url, token)
        local = sha256(path)
        if status == 404:
            upload(url, token, path)
            print(f"published {path.name}")
        elif status == 200 and existing == local:
            print(f"kept identical existing package {path.name}")
        elif status == 200:
            raise RuntimeError(f"existing package differs from validated file: {path.name}")
        else:
            raise RuntimeError(f"GitLab package lookup failed with HTTP {status}: {path.name}")
        links.append({"name": path.name, "url": url, "link_type": "package"})
    return links


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--directory", type=Path, required=True)
    parser.add_argument("--version", required=True)
    parser.add_argument("--links-output", type=Path, required=True)
    args = parser.parse_args()
    try:
        links = publish(
            args.directory,
            args.version,
            os.environ["CI_API_V4_URL"],
            os.environ["CI_PROJECT_ID"],
            os.environ["CI_JOB_TOKEN"],
        )
        args.links_output.write_text(json.dumps(links, separators=(",", ":")), encoding="utf-8")
        return 0
    except (KeyError, OSError, ValueError, RuntimeError) as error:
        print(f"GitLab publication failed: {error}", file=__import__("sys").stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
