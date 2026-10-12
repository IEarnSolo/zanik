#!/usr/bin/env python3
"""Validate or upload a Zanik release through the CurseForge author API."""

from __future__ import annotations

import argparse
import hashlib
import http.client
import json
import os
import secrets
from pathlib import Path
from urllib.parse import urlsplit


def digest(path: Path) -> str:
    value = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(block)
    return value.hexdigest()


def multipart_parts(metadata: dict, path: Path, boundary: str):
    prefix = (
        f"--{boundary}\r\nContent-Disposition: form-data; name=\"metadata\"\r\n"
        "Content-Type: application/json\r\n\r\n"
        f"{json.dumps(metadata, separators=(',', ':'))}\r\n"
        f"--{boundary}\r\nContent-Disposition: form-data; name=\"file\"; filename=\"{path.name}\"\r\n"
        "Content-Type: application/octet-stream\r\n\r\n"
    ).encode()
    suffix = f"\r\n--{boundary}--\r\n".encode()
    return prefix, suffix


def upload(base_url: str, project_id: str, token: str, metadata: dict, path: Path) -> int:
    parsed = urlsplit(base_url)
    cls = http.client.HTTPSConnection if parsed.scheme == "https" else http.client.HTTPConnection
    conn = cls(parsed.hostname, parsed.port, timeout=120)
    boundary = "zanik-" + secrets.token_hex(16)
    prefix, suffix = multipart_parts(metadata, path, boundary)
    endpoint = f"{parsed.path.rstrip('/')}/api/projects/{project_id}/upload-file"
    conn.putrequest("POST", endpoint)
    conn.putheader("X-Api-Token", token)
    conn.putheader("Content-Type", f"multipart/form-data; boundary={boundary}")
    conn.putheader("Content-Length", str(len(prefix) + path.stat().st_size + len(suffix)))
    conn.endheaders()
    conn.send(prefix)
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            conn.send(block)
    conn.send(suffix)
    response = conn.getresponse()
    body = response.read(4096).decode("utf-8", errors="replace").replace(token, "[REDACTED]")
    status = response.status
    conn.close()
    if status not in {200, 201}:
        raise RuntimeError(f"CurseForge returned HTTP {status}: {body[:500]}")
    try:
        file_id = int(json.loads(body)["id"])
    except (ValueError, KeyError, TypeError, json.JSONDecodeError) as error:
        raise RuntimeError("CurseForge returned an invalid upload response") from error
    return file_id


def write_report(path: Path, report: dict) -> None:
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    temporary.replace(path)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--directory", type=Path, required=True)
    parser.add_argument("--version", required=True)
    parser.add_argument("--tag", required=True)
    parser.add_argument("--changelog", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    parser.add_argument("--project-id", default=os.environ.get("CURSEFORGE_PROJECT_ID", ""))
    parser.add_argument("--base-url", default="https://www.curseforge.com")
    parser.add_argument("--primary-file-id", type=int)
    parser.add_argument("--skip-child", action="append", choices=("linux", "windows"), default=[])
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    token = os.environ.get("CURSEFORGE_API_TOKEN", "")
    files = {
        "primary": args.directory / f"zanik-server-linux-x86_64-{args.version}.zip",
        "linux": args.directory / f"zanik-asset-tools-linux-x86_64-{args.version}.tar.xz",
        "windows": args.directory / f"zanik-asset-tools-windows-x86_64-{args.version}.zip",
    }
    report = {"schemaVersion": 1, "tag": args.tag, "version": args.version, "status": "validating", "files": {}}
    try:
        if args.tag != f"v{args.version}":
            raise ValueError("tag and version do not match")
        changelog = args.changelog.read_text(encoding="utf-8").strip()
        if not changelog:
            raise ValueError("release changelog is empty")
        for role, path in files.items():
            if not path.is_file():
                raise ValueError(f"missing CurseForge file: {path.name}")
            report["files"][role] = {"name": path.name, "sha256": digest(path), "status": "validated"}
        if args.dry_run:
            report["status"] = "validated"
            write_report(args.report, report)
            print("validated CurseForge upload without sending files")
            return 0
        if not args.project_id.isdigit():
            raise ValueError("CURSEFORGE_PROJECT_ID must be numeric")
        if not token:
            raise ValueError("CURSEFORGE_PROJECT_ID and CURSEFORGE_API_TOKEN are required")
        primary_id = args.primary_file_id
        if primary_id is None:
            metadata = {"changelog": changelog, "changelogType": "markdown", "displayName": f"Zanik {args.version} - Linux Dedicated Server", "gameVersionNames": ["1.0"], "releaseType": "release", "isMarkedForManualRelease": False}
            primary_id = upload(args.base_url, args.project_id, token, metadata, files["primary"])
            report["files"]["primary"].update(status="uploaded", fileId=primary_id)
            write_report(args.report, report)
        else:
            report["files"]["primary"].update(status="reused", fileId=primary_id)
        for role, label in (("linux", "Linux x86_64"), ("windows", "Windows x86_64")):
            if role in args.skip_child:
                report["files"][role]["status"] = "skipped"
                continue
            metadata = {"changelog": changelog, "changelogType": "markdown", "displayName": f"Zanik Asset Tools {label} {args.version}", "parentFileID": primary_id, "releaseType": "release", "isMarkedForManualRelease": False}
            file_id = upload(args.base_url, args.project_id, token, metadata, files[role])
            report["files"][role].update(status="uploaded", fileId=file_id)
            write_report(args.report, report)
        report["status"] = "submitted"
        write_report(args.report, report)
        print(f"submitted Zanik {args.version} to CurseForge as primary file {primary_id}")
        return 0
    except (OSError, ValueError, RuntimeError) as error:
        report["status"] = "failed"
        report["error"] = str(error).replace(token, "[REDACTED]") if token else str(error)
        write_report(args.report, report)
        print(f"CurseForge upload failed: {report['error']}", file=__import__("sys").stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
