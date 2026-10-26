#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
SERVER=$TMP/server
ZANIK=$SERVER/zanik
REQUEST=0123456789abcdef01234567
CURRENT=$ZANIK/assets/current
STAGE=$SERVER/.zanik-manager/staging/asset-delta-$REQUEST
mkdir -p "$ZANIK/scripts" "$CURRENT/quests" "$STAGE"
cp "$ROOT/scripts/apply-asset-delta.sh" "$ZANIK/scripts/apply-asset-delta.sh"
chmod +x "$ZANIK/scripts/apply-asset-delta.sh"
printf 'keep' >"$CURRENT/keep.txt"
printf 'retire' >"$CURRENT/quests/retired.png"
printf '{"recipe":10}\n' >"$CURRENT/asset-manifest.json"
printf '{"recipe":11}\n' >"$STAGE/target-asset-manifest.json"
printf '{}\n' >"$STAGE/asset-delta.json"
BASE_HASH=$(sha256sum "$CURRENT/asset-manifest.json" | awk '{print $1}')
KEEP_HASH=$(sha256sum "$CURRENT/keep.txt" | awk '{print $1}')
MANIFEST_HASH=$(sha256sum "$STAGE/target-asset-manifest.json" | awk '{print $1}')
printf '%s\r\n' "$BASE_HASH" >"$STAGE/base-manifest.sha256"
printf 'quests/retired.png\r\n' >"$STAGE/removed.list"
printf '%s  keep.txt\r\n%s  asset-manifest.json\r\n' "$KEEP_HASH" "$MANIFEST_HASH" \
  >"$STAGE/target-files.sha256"
printf 'keep.txt\r\nasset-manifest.json\r\n' >"$STAGE/target-files.list"

"$ZANIK/scripts/apply-asset-delta.sh" --server-root "$SERVER" --request-id "$REQUEST"
REPORT=$ZANIK/state/asset-delta-status-$REQUEST.json
grep -q '"status":"activated"' "$REPORT"
grep -q '"changed":0' "$REPORT"
grep -q '"removed":1' "$REPORT"
grep -q '"fileCount":1' "$REPORT"
test -f "$CURRENT/keep.txt"
test ! -e "$CURRENT/quests/retired.png"
grep -q '"recipe":11' "$CURRENT/asset-manifest.json"
echo 'server asset delta helper CRLF test passed'
