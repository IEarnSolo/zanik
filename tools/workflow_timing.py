#!/usr/bin/env python3
"""Maintain an atomic, ignored timing ledger for long compatibility workflows."""
from __future__ import annotations

import argparse
import datetime as dt
import json
import os
from pathlib import Path


def now() -> dt.datetime:
    return dt.datetime.now(dt.timezone.utc)


def timestamp(value: dt.datetime) -> str:
    return value.isoformat().replace("+00:00", "Z")


def parsed(value: str) -> dt.datetime:
    result = dt.datetime.fromisoformat(value.replace("Z", "+00:00"))
    return result if result.tzinfo else result.replace(tzinfo=dt.timezone.utc)


def atomic_write(path: Path, value: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + ".tmp")
    temporary.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    os.replace(temporary, path)


def load(path: Path) -> dict:
    value = json.loads(path.read_text(encoding="utf-8"))
    if value.get("schemaVersion") != 1 or not isinstance(value.get("phases"), list):
        raise ValueError("timing ledger has an unsupported schema")
    return value


def close_phase(value: dict, outcome: str, finished: dt.datetime) -> None:
    active = value.get("activePhase")
    if not active:
        return
    started = parsed(active["startedAt"])
    value["phases"].append({
        "name": active["name"], "startedAt": active["startedAt"],
        "finishedAt": timestamp(finished),
        "durationMilliseconds": max(0, int((finished - started).total_seconds() * 1000)),
        "outcome": outcome,
    })
    value["activePhase"] = None


def bounded_name(raw: str) -> str:
    value = " ".join(raw.split())
    if not value or len(value) > 160:
        raise ValueError("phase and workflow names must contain 1 to 160 normalized characters")
    return value


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("command", choices=("start", "phase", "finish", "status"))
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--workflow")
    parser.add_argument("--name")
    parser.add_argument("--status", default="complete", choices=("complete", "failed", "cancelled"))
    args = parser.parse_args()
    current = now()
    if args.command == "start":
        if not args.workflow:
            parser.error("start requires --workflow")
        if args.output.exists():
            existing = load(args.output)
            if not existing.get("finishedAt"):
                raise RuntimeError("an unfinished timing ledger already exists")
            raise RuntimeError("refusing to overwrite an existing timing ledger")
        value = {"schemaVersion": 1, "workflow": bounded_name(args.workflow),
                 "startedAt": timestamp(current), "finishedAt": None, "status": "running",
                 "activePhase": None, "phases": []}
    else:
        value = load(args.output)
        if value.get("finishedAt"):
            raise RuntimeError("timing ledger is already finished")
    if args.command == "phase":
        if not args.name:
            parser.error("phase requires --name")
        name = bounded_name(args.name)
        if (value.get("activePhase") or {}).get("name") != name:
            close_phase(value, "complete", current)
            value["activePhase"] = {"name": name, "startedAt": timestamp(current)}
    elif args.command == "finish":
        close_phase(value, args.status, current)
        value["finishedAt"] = timestamp(current)
        value["durationMilliseconds"] = max(
            0, int((current - parsed(value["startedAt"])).total_seconds() * 1000))
        value["status"] = args.status
    atomic_write(args.output, value)
    print(json.dumps(value, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
