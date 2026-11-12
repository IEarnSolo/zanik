#!/usr/bin/env python3
import importlib.util
import json
import pathlib
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("compatibility_report", ROOT / "tools/generate_compatibility_report.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)

registry = {
    "schemaVersion": 1,
    "capabilities": [
        {"id": "native", "category": "native", "runtimeStatusFields": ["nativeCompatibility"],
         "runtimeDiagnosticFields": ["nativeFailures"],
         "staticChecks": ["signature"], "diagnosticsEndpoint": "/api/native", "persistence": [],
         "assets": [], "failureBoundary": "native only", "tests": ["negative"]},
        {"id": "network", "category": "native", "runtimeStatusFields": [
            {"path": "network.compatibility", "pendingAllowed": True}], "staticChecks": [],
         "diagnosticsEndpoint": "/api/network", "persistence": [], "assets": [],
         "failureBoundary": "network only", "tests": []},
        {"id": "mode", "category": "operational", "runtimeStatusFields": [
            {"path": "configuration.installMode", "acceptedValues": ["wrapper", "replace-executable"]}],
         "staticChecks": [], "diagnosticsEndpoint": "/api/update-status", "persistence": [],
         "assets": [], "failureBoundary": "updates only", "tests": []},
    ],
}
provenance = {"gameBuild": "1", "executableSha256": "a" * 64, "candidate": "test",
              "analyzerManifest": "analyzer:a;manifest:b",
              "payloadContract": "b" * 64, "assetContract": "c" * 64}
report = MODULE.build_report(registry, {"nativeCompatibility": 1, "nativeFailures": 0, "network": {"compatibility": 0},
                                       "configuration": {"installMode": "wrapper"}}, provenance)
assert report["summary"] == {"compatible": 2, "pending": 1, "disabled": 0, "not-applicable": 0, "incompatible": 0}
assert "`nativeCompatibility`=1" in MODULE.markdown(report)
assert "`nativeFailures`=0" in MODULE.markdown(report)
assert report["capabilities"][0]["runtimeDiagnostics"] == [{"path": "nativeFailures", "value": 0}]

report = MODULE.build_report(registry, {"nativeCompatibility": -1, "nativeFailures": 0, "network": {"compatibility": 1},
                                       "configuration": {"installMode": "replace-executable"}}, provenance)
assert report["summary"]["incompatible"] == 1

try:
    MODULE.build_report(registry, {"network": {"compatibility": 1},
                                   "configuration": {"installMode": "wrapper"}}, provenance)
except ValueError as error:
    assert "missing required update-status field nativeCompatibility" in str(error)
else:
    raise AssertionError("missing status field did not fail closed")

real = json.loads((ROOT / "deploy/compatibility-capabilities.json").read_text())
ids = [item["id"] for item in real["capabilities"]]
assert len(ids) == len(set(ids)) and {"player-quests", "home-teleport", "native-inventory-attribution", "website-assets", "game-update-lifecycle"} <= set(ids)
for item in real["capabilities"]:
    assert item["runtimeStatusFields"] and item["diagnosticsEndpoint"] and item["failureBoundary"] and item["tests"] and item["liveValidation"]

print("compatibility report tests passed")
