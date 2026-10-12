#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p reference/grave-cells-json
for file in reference/grave-cells/*.lvl; do
  base="$(basename "$file" .lvl)"
  python3 tools/inspect_spud_level.py "$file" > "reference/grave-cells-json/$base.json"
done
