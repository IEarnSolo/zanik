#!/usr/bin/env python3
"""Read-only GitLab pipeline status, job, and trace monitor."""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import time
from typing import Any
from urllib.error import HTTPError, URLError
from urllib.parse import quote, urlencode
from urllib.request import Request, urlopen


ROOT = Path(__file__).resolve().parents[1]
TERMINAL_STATUSES = {"success", "failed", "canceled", "skipped"}
FAILURE_STATUSES = {"failed", "canceled"}


class MonitorError(RuntimeError):
    pass


def load_env(path: Path) -> None:
    if not path.is_file():
        raise MonitorError(
            f"GitLab environment file not found: {path}\n"
            "Copy .env.gitlab.example to .env.gitlab and add a read_api token."
        )
    for number, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        if "=" not in line:
            raise MonitorError(f"Invalid environment entry at {path}:{number}")
        key, value = line.split("=", 1)
        key, value = key.strip(), value.strip()
        if value[:1] == value[-1:] and value[:1] in {"'", '"'}:
            value = value[1:-1]
        if key and key not in os.environ:
            os.environ[key] = value


def required_env(name: str) -> str:
    value = os.environ.get(name, "").strip()
    if not value or "replace-with" in value:
        raise MonitorError(f"Set {name} in .env.gitlab")
    return value


class GitLab:
    def __init__(self) -> None:
        self.base = required_env("GITLAB_BASE_URL").rstrip("/")
        self.project = quote(required_env("GITLAB_PROJECT"), safe="")
        self.token = required_env("GITLAB_TOKEN")

    def get(self, path: str, query: dict[str, Any] | None = None, raw: bool = False) -> Any:
        url = f"{self.base}/api/v4/projects/{self.project}/{path.lstrip('/')}"
        if query:
            url += "?" + urlencode(query)
        request = Request(url, headers={"PRIVATE-TOKEN": self.token, "User-Agent": "Zanik-pipeline-monitor/1"})
        try:
            with urlopen(request, timeout=30) as response:
                payload = response.read()
        except HTTPError as error:
            detail = error.read().decode("utf-8", errors="replace")[:500]
            raise MonitorError(f"GitLab API returned HTTP {error.code} for {path}: {detail}") from error
        except URLError as error:
            raise MonitorError(f"Could not reach the GitLab API: {error.reason}") from error
        if raw:
            return payload.decode("utf-8", errors="replace")
        try:
            return json.loads(payload)
        except json.JSONDecodeError as error:
            raise MonitorError(f"GitLab returned invalid JSON for {path}") from error

    def download(self, path: str, destination: Path) -> None:
        url = f"{self.base}/api/v4/projects/{self.project}/{path.lstrip('/')}"
        request = Request(url, headers={"PRIVATE-TOKEN": self.token, "User-Agent": "Zanik-pipeline-monitor/1"})
        try:
            destination.parent.mkdir(parents=True, exist_ok=True)
            with urlopen(request, timeout=120) as response, destination.open("wb") as output:
                while block := response.read(1024 * 1024):
                    output.write(block)
        except HTTPError as error:
            raise MonitorError(f"GitLab API returned HTTP {error.code} while downloading job artifacts") from error
        except URLError as error:
            raise MonitorError(f"Could not reach the GitLab API: {error.reason}") from error


def current_sha() -> str:
    try:
        return subprocess.check_output(
            ["git", "rev-parse", "HEAD"], cwd=ROOT, text=True, stderr=subprocess.DEVNULL
        ).strip()
    except (OSError, subprocess.CalledProcessError) as error:
        raise MonitorError("Could not determine the current Git commit; pass --sha explicitly") from error


def find_pipeline(api: GitLab, ref: str, sha: str | None) -> dict[str, Any] | None:
    query: dict[str, Any] = {"ref": ref, "per_page": 20, "order_by": "id", "sort": "desc"}
    if sha:
        query["sha"] = sha
    pipelines = api.get("pipelines", query)
    return pipelines[0] if pipelines else None


def pipeline_jobs(api: GitLab, pipeline_id: int) -> list[dict[str, Any]]:
    return api.get(
        f"pipelines/{pipeline_id}/jobs",
        {"per_page": 100, "include_retried": "true"},
    )


def print_pipeline(pipeline: dict[str, Any]) -> None:
    print(
        f"pipeline={pipeline.get('id')} status={pipeline.get('status')} "
        f"ref={pipeline.get('ref')} sha={str(pipeline.get('sha', ''))[:12]}"
    )
    if pipeline.get("web_url"):
        print(pipeline["web_url"])


def print_jobs(jobs: list[dict[str, Any]]) -> None:
    if not jobs:
        print("No jobs found.")
        return
    print("\nJobs:")
    for job in sorted(jobs, key=lambda value: (value.get("stage", ""), value.get("id", 0))):
        duration = job.get("duration")
        elapsed = "-" if duration is None else f"{duration:.1f}s"
        print(
            f"  {job.get('status', '-'):9} {job.get('stage', '-'):10} "
            f"{job.get('name', '-'):24} id={job.get('id')} duration={elapsed}"
        )


def tail_trace(api: GitLab, job: dict[str, Any], line_count: int) -> None:
    trace = api.get(f"jobs/{job['id']}/trace", raw=True)
    lines = trace.splitlines()
    print(f"\n--- {job.get('name')} (job {job['id']}) last {line_count} lines ---")
    print("\n".join(lines[-line_count:]))


def resolve_pipeline(api: GitLab, args: argparse.Namespace) -> dict[str, Any]:
    if args.pipeline_id:
        return api.get(f"pipelines/{args.pipeline_id}")
    pipeline = find_pipeline(api, args.ref, args.sha)
    if not pipeline:
        target = f"commit {args.sha}" if args.sha else f"ref {args.ref}"
        raise MonitorError(f"No pipeline found for {target}")
    return pipeline


def watch(api: GitLab, args: argparse.Namespace) -> int:
    sha = args.sha or current_sha()
    deadline = time.monotonic() + args.timeout
    pipeline: dict[str, Any] | None = None
    print(f"Waiting for GitLab pipeline for {sha} on {args.ref}...")
    while pipeline is None:
        pipeline = find_pipeline(api, args.ref, sha)
        if pipeline is not None:
            break
        if time.monotonic() >= deadline:
            raise MonitorError(f"Timed out waiting for a pipeline for commit {sha}")
        time.sleep(args.interval)

    pipeline_id = int(pipeline["id"])
    previous_status = ""
    while True:
        pipeline = api.get(f"pipelines/{pipeline_id}")
        status = str(pipeline.get("status", "unknown"))
        if status != previous_status:
            print_pipeline(pipeline)
            previous_status = status
        if status in TERMINAL_STATUSES:
            break
        if time.monotonic() >= deadline:
            raise MonitorError(f"Timed out while pipeline {pipeline_id} was {status}")
        time.sleep(args.interval)

    jobs = pipeline_jobs(api, pipeline_id)
    print_jobs(jobs)
    if status in FAILURE_STATUSES:
        for job in jobs:
            if job.get("status") in FAILURE_STATUSES:
                tail_trace(api, job, args.tail)
        return 1
    return 0


def download_artifacts(api: GitLab, args: argparse.Namespace) -> int:
    pipeline = resolve_pipeline(api, args)
    jobs = pipeline_jobs(api, int(pipeline["id"]))
    matches = [job for job in jobs if job.get("name") == args.job_name and job.get("status") == "success"]
    if not matches:
        raise MonitorError(f"No successful {args.job_name!r} job exists in pipeline {pipeline['id']}")
    job = max(matches, key=lambda value: int(value["id"]))
    destination = args.output.resolve()
    api.download(f"jobs/{job['id']}/artifacts", destination)
    print(f"downloaded {args.job_name} artifacts from pipeline {pipeline['id']} to {destination}")
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("latest", "watch", "jobs", "trace", "artifacts"), nargs="?", default="watch")
    parser.add_argument("--env-file", type=Path, default=ROOT / ".env.gitlab")
    parser.add_argument("--ref", default="main")
    parser.add_argument("--sha", help="Commit SHA; watch defaults to the current Git HEAD")
    parser.add_argument("--pipeline-id", type=int)
    parser.add_argument("--job-id", type=int)
    parser.add_argument("--interval", type=float)
    parser.add_argument("--timeout", type=float)
    parser.add_argument("--tail", type=int, default=200)
    parser.add_argument("--job-name", default="package-release")
    parser.add_argument("--output", type=Path, default=ROOT / ".tmp" / "gitlab-artifacts.zip")
    return parser


def main() -> int:
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(line_buffering=True)
    args = build_parser().parse_args()
    try:
        load_env(args.env_file.resolve())
        args.interval = args.interval or float(os.environ.get("GITLAB_POLL_SECONDS", "10"))
        args.timeout = args.timeout or float(os.environ.get("GITLAB_TIMEOUT_SECONDS", "1800"))
        if args.interval <= 0 or args.timeout <= 0 or args.tail <= 0:
            raise MonitorError("Polling interval, timeout, and trace line count must be positive")
        api = GitLab()
        if args.action == "watch":
            return watch(api, args)
        if args.action == "trace":
            if not args.job_id:
                raise MonitorError("trace requires --job-id")
            print(api.get(f"jobs/{args.job_id}/trace", raw=True), end="")
            return 0
        if args.action == "artifacts":
            return download_artifacts(api, args)
        pipeline = resolve_pipeline(api, args)
        print_pipeline(pipeline)
        if args.action == "jobs":
            print_jobs(pipeline_jobs(api, int(pipeline["id"])))
        return 0
    except (MonitorError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
