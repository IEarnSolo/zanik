#!/usr/bin/env python3
"""Reject operator-specific or credential-shaped data in tracked files."""

from __future__ import annotations

import re
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TEXT_SUFFIXES = {
    "", ".c", ".cpp", ".h", ".html", ".js", ".json", ".md", ".ps1",
    ".py", ".sh", ".tsv", ".txt", ".yaml", ".yml",
}


def tracked_files() -> list[Path]:
    output = subprocess.check_output(
        ["git", "ls-files", "-z"], cwd=ROOT
    ).decode("utf-8", errors="strict")
    return [ROOT / name for name in output.split("\0") if name]


failures: list[str] = []
token_patterns = {
    "GitHub token": re.compile(r"(?:github_pat_[A-Za-z0-9_]{20,}|gh[pousr]_[A-Za-z0-9]{20,})"),
    "GitLab token": re.compile(r"glpat-[A-Za-z0-9_-]{20,}"),
    "Pterodactyl token": re.compile(r"ptl[ac]_[A-Za-z0-9]{20,}"),
    "AWS access key": re.compile(r"AKIA[0-9A-Z]{16}"),
    "private key": re.compile(r"-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----"),
}
email_pattern = re.compile(r"[A-Za-z0-9._%+-]+@([A-Za-z0-9.-]+\.[A-Za-z]{2,})")
windows_user_pattern = re.compile(r"[A-Za-z]:\\Users\\([^\\\r\n]+)", re.IGNORECASE)
unix_home_pattern = re.compile(r"(?<![A-Za-z0-9_-])/(?:home|mnt/[a-z]/Users)/([A-Za-z0-9._-]+)", re.IGNORECASE)
server_id_pattern = re.compile(
    r"(?:development|production|staging) server `(?=[0-9a-f]{8}`)(?=[0-9a-f]*[a-f])[0-9a-f]{8}`",
    re.IGNORECASE,
)
dated_backup_pattern = re.compile(r"zanik-backups/[^\s`\"']*/20\d{6}T\d{6}Z", re.IGNORECASE)

for path in tracked_files():
    if path.suffix.lower() not in TEXT_SUFFIXES or not path.is_file():
        continue
    try:
        text = path.read_text(encoding="utf-8")
    except UnicodeDecodeError:
        continue
    relative = path.relative_to(ROOT).as_posix()
    for label, pattern in token_patterns.items():
        if label == "private key" and relative == "tools/validate_release.py":
            continue
        for match in pattern.finditer(text):
            if label == "GitLab token" and "replace-with-" in match.group(0):
                continue
            failures.append(f"{relative}: contains a credential-shaped {label}")
    for match in email_pattern.finditer(text):
        domain = match.group(1).lower()
        if domain not in {"example.com", "example.invalid", "users.noreply.github.com"}:
            failures.append(f"{relative}: contains a non-example email address")
    for match in windows_user_pattern.finditer(text):
        if match.group(1).lower() != "exampleuser":
            failures.append(f"{relative}: contains a workstation-specific Windows user path")
    for match in unix_home_pattern.finditer(text):
        if match.group(1).lower() not in {"container", "exampleuser"}:
            failures.append(f"{relative}: contains a workstation-specific home path")
    if relative.startswith("docs/") and server_id_pattern.search(text):
        failures.append(f"{relative}: contains a live-looking server identifier")
    if relative.startswith("docs/") and '"serverId"' in text:
        failures.append(f"{relative}: contains a server identifier field")
    if relative.startswith("docs/") and dated_backup_pattern.search(text):
        failures.append(f"{relative}: contains an operator-specific managed-backup path")

gitlab_example = (ROOT / ".env.gitlab.example").read_text(encoding="utf-8")
if "GITLAB_BASE_URL=https://gitlab.example.com" not in gitlab_example:
    failures.append(".env.gitlab.example: must use the example GitLab origin")
if "GITLAB_PROJECT=namespace/zanik" not in gitlab_example:
    failures.append(".env.gitlab.example: must use an example project path")

if failures:
    raise SystemExit("repository hygiene failures:\n  - " + "\n  - ".join(sorted(set(failures))))
print("repository privacy and credential hygiene: ok")
