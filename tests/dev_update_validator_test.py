#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / "deploy/validate-dev-update.ps1").read_text()
assert "[ValidateSet('Candidate','Production')]" in source
assert "$ValidationMode = 'Production'" in source
assert "$storageModeOk" in source
assert "$update.adminStorageProductionCompatibility -eq -1" in source
assert "$update.adminStorageCandidateOverride -eq $true" in source
assert "$update.adminStorageProductionCompatibility -eq 1" in source
assert "$update.adminStorageCandidateOverride -eq $false" in source
assert "$adminDiagnostics.storageProductionCompatibility" in source
assert "$adminDiagnostics.storageCandidateOverride" in source
assert "$storageModeOk -and $claimPageOk" in source
candidate = source.split("$storageModeOk=if($ValidationMode -eq 'Candidate'){", 1)[1].split("}else{", 1)[0]
assert "ProductionCompatibility -eq 1" not in candidate
assert "CandidateOverride -eq $false" not in candidate
