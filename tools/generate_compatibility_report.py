#!/usr/bin/env python3
from __future__ import annotations

import argparse
import datetime as dt
import json
from pathlib import Path
from urllib.parse import urlsplit
from urllib.request import urlopen


def nested(document, path):
    value = document
    for part in path.split("."):
        if not isinstance(value, dict) or part not in value:
            raise KeyError(path)
        value = value[part]
    return value


def field_spec(value):
    return ({"path": value, "pendingAllowed": False} if isinstance(value, str) else value)


def evaluate(value, spec):
    if value in spec.get("acceptedValues", []):
        return "compatible"
    if value is True or value == 1 or value == "compatible":
        return "compatible"
    if (value is None or value == 0 or value == "pending") and spec.get("pendingAllowed"):
        return "pending"
    if value == "disabled" and spec.get("disabledAllowed"):
        return "disabled"
    if value in {"not-applicable", "not_applicable"} and spec.get("notApplicableAllowed"):
        return "not-applicable"
    return "incompatible"


def build_report(registry, status, provenance):
    if registry.get("schemaVersion") != 1 or not isinstance(registry.get("capabilities"), list):
        raise ValueError("unsupported capability registry")
    seen = set()
    capabilities = []
    for source in registry["capabilities"]:
        capability_id = source.get("id")
        if not isinstance(capability_id, str) or not capability_id or capability_id in seen:
            raise ValueError("capability IDs must be nonempty and unique")
        seen.add(capability_id)
        checks = []
        for raw in source.get("runtimeStatusFields", []):
            spec = field_spec(raw)
            path = spec["path"]
            try:
                value = nested(status, path)
            except KeyError as error:
                raise ValueError(f"{capability_id} is missing required update-status field {path}") from error
            checks.append({"path": path, "value": value, "result": evaluate(value, spec)})
        diagnostics = []
        for path in source.get("runtimeDiagnosticFields", []):
            try:
                value = nested(status, path)
            except KeyError as error:
                raise ValueError(f"{capability_id} is missing required update-status field {path}") from error
            diagnostics.append({"path": path, "value": value})
        results = {item["result"] for item in checks}
        result = ("incompatible" if "incompatible" in results else
                  "pending" if "pending" in results else
                  "disabled" if "disabled" in results else
                  "not-applicable" if "not-applicable" in results else "compatible")
        capabilities.append({
            "id": capability_id,
            "category": source.get("category", "unspecified"),
            "result": result,
            "runtimeChecks": checks,
            "runtimeDiagnostics": diagnostics,
            "staticChecks": source.get("staticChecks", []),
            "diagnosticsEndpoint": source.get("diagnosticsEndpoint", ""),
            "persistence": source.get("persistence", []),
            "assets": source.get("assets", []),
            "failureBoundary": source.get("failureBoundary", ""),
            "tests": source.get("tests", []),
            "liveValidation": source.get("liveValidation", []),
        })
    counts = {name: sum(item["result"] == name for item in capabilities)
              for name in ("compatible", "pending", "disabled", "not-applicable", "incompatible")}
    # Public reports identify the reviewed candidate date without publishing an
    # operator-specific deployment timestamp.
    return {"schemaVersion": 1, "generatedAt": dt.datetime.now(dt.timezone.utc).date().isoformat(),
            "provenance": provenance, "summary": counts, "capabilities": capabilities}


def markdown(report):
    provenance = report["provenance"]
    lines = ["# Zanik compatibility report", "", f"Generated: `{report['generatedAt']}`", "",
             "## Candidate provenance", ""]
    for key in sorted(provenance):
        lines.append(f"- {key}: `{provenance[key]}`")
    lines += ["", "## Capability results", "", "| Capability | Result | Runtime checks | Failure boundary |",
              "|---|---|---|---|"]
    for capability in report["capabilities"]:
        checks = ", ".join(f"`{item['path']}`={json.dumps(item['value'])}" for item in capability["runtimeChecks"])
        diagnostics = ", ".join(f"`{item['path']}`={json.dumps(item['value'])}" for item in capability.get("runtimeDiagnostics", []))
        if diagnostics:
            checks += "; diagnostics: " + diagnostics
        lines.append(f"| `{capability['id']}` | {capability['result']} | {checks} | {capability['failureBoundary']} |")
    summary = report["summary"]
    lines += ["", (f"Compatible: {summary['compatible']} · Pending: {summary['pending']} · "
                    f"Disabled: {summary['disabled']} · Not applicable: {summary['not-applicable']} · "
                    f"Incompatible: {summary['incompatible']}"), ""]
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--registry", type=Path, required=True)
    status_source = parser.add_mutually_exclusive_group(required=True)
    status_source.add_argument("--status", type=Path)
    status_source.add_argument("--status-url")
    parser.add_argument("--json-output", type=Path, required=True)
    parser.add_argument("--markdown-output", type=Path, required=True)
    parser.add_argument("--game-build", required=True)
    parser.add_argument("--executable-sha256", required=True)
    parser.add_argument("--candidate", required=True)
    parser.add_argument("--analyzer-manifest", required=True)
    parser.add_argument("--payload-contract", required=True)
    parser.add_argument("--asset-contract", required=True)
    args = parser.parse_args()
    registry = json.loads(args.registry.read_text(encoding="utf-8"))
    if args.status_url:
        parsed = urlsplit(args.status_url)
        if parsed.scheme not in {"http", "https"} or not parsed.hostname:
            raise SystemExit("--status-url must be an HTTP or HTTPS URL")
        with urlopen(args.status_url, timeout=30) as response:
            status = json.load(response)
    else:
        status = json.loads(args.status.read_text(encoding="utf-8"))
    provenance = {"gameBuild": args.game_build, "executableSha256": args.executable_sha256,
                  "candidate": args.candidate, "analyzerManifest": args.analyzer_manifest,
                  "payloadContract": args.payload_contract,
                  "assetContract": args.asset_contract}
    report = build_report(registry, status, provenance)
    args.json_output.parent.mkdir(parents=True, exist_ok=True)
    args.markdown_output.parent.mkdir(parents=True, exist_ok=True)
    args.json_output.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    args.markdown_output.write_text(markdown(report), encoding="utf-8")
    if report["summary"]["incompatible"]:
        raise SystemExit("one or more compatibility capabilities are incompatible")


if __name__ == "__main__":
    main()
