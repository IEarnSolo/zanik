#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
ASSETS=$TMP/assets; mkdir -p "$ASSETS/items"
printf 'map-data' > "$ASSETS/world-map.jpg"
printf 'item-data' > "$ASSETS/items/example.png"
MAP_HASH=$(sha256sum "$ASSETS/world-map.jpg" | awk '{print $1}')
ITEM_HASH=$(sha256sum "$ASSETS/items/example.png" | awk '{print $1}')
printf '{\n  "files": [\n    {\n      "path": "world-map.jpg",\n      "sha256": "%s",\n      "size": 8\n    },\n    {\n      "path": "items/example.png",\n      "sha256": "%s",\n      "size": 9\n    }\n  ]\n}\n' "$MAP_HASH" "$ITEM_HASH" > "$ASSETS/asset-manifest.json"

"$ROOT/scripts/verify-assets.sh" "$ASSETS" "$TMP/report.json" abc123
grep -q '"status":"valid"' "$TMP/report.json"
grep -q '"checked":2' "$TMP/report.json"
grep -q '"requestId":"abc123"' "$TMP/report.json"

printf 'corrupt' > "$ASSETS/items/example.png"
"$ROOT/scripts/verify-assets.sh" "$ASSETS" "$TMP/bad-report.json" def456
grep -q '"status":"invalid"' "$TMP/bad-report.json"
grep -q '"mismatches":1' "$TMP/bad-report.json"
echo 'server asset verifier tests passed'
