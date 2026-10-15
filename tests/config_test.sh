#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
. "$ROOT/scripts/lib/zanik-common.sh"
TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT

zanik_defaults
[ "$CFG_FEATURE_PLAYER_STATUS" = 1 ]
[ "$CFG_ZANIK_ENABLED" = 1 ]
[ "$CFG_FEATURE_ADMIN_TELEPORT" = 0 ]
[ "$CFG_FEATURE_QOL_AUTO_COMPOST" = 0 ]

printf 'ZANIK_ENABLED=false\nSTATUS_PORT=30123\nFEATURE_ADMIN_TELEPORT=true\n' > "$TMP/good.conf"
zanik_load_config "$TMP/good.conf"
[ "$CFG_STATUS_PORT" = 30123 ]
[ "$CFG_FEATURE_ADMIN_TELEPORT" = 1 ]
[ "$CFG_ZANIK_ENABLED" = 0 ]

printf 'UNKNOWN_SECRET=do-not-log-this\n' > "$TMP/unknown.conf"
if zanik_load_config "$TMP/unknown.conf" 2> "$TMP/error"; then exit 1; fi
grep -q "Unknown configuration key 'UNKNOWN_SECRET'" "$TMP/error"
! grep -q 'do-not-log-this' "$TMP/error"

printf 'FEATURE_ADMIN_TELEPORT=maybe\n' > "$TMP/bad.conf"
if zanik_load_config "$TMP/bad.conf" >/dev/null 2>&1; then exit 1; fi
echo 'configuration tests passed'
