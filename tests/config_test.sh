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

(
  unset DW_SERVER_ROOT DW_STATUS_DATA DW_ZANIK_ROOT DW_DATA_ROOT DW_WEB_ROOT DW_GAME_LOG DW_WORLD_SAVE DW_ORIGINAL_EXE
  zanik_defaults
  zanik_export_runtime /srv/dragonwilds wrapper
  [ "$DW_SERVER_ROOT" = /srv/dragonwilds ]
  [ "$DW_STATUS_DATA" = /srv/dragonwilds/zanik-data/players.db ]
  [ "$DW_DATA_ROOT" = /srv/dragonwilds/zanik-data ]
  [ "$DW_WEB_ROOT" = /srv/dragonwilds/zanik/assets/current ]
  [ "$DW_GAME_LOG" = /srv/dragonwilds/RSDragonwilds/Saved/Logs/RSDragonwilds.log ]
  [ "$DW_WORLD_SAVE" = "/srv/dragonwilds/RSDragonwilds/Saved/SaveGames/Emberlands TEST.sav" ]
  [ "$DW_ORIGINAL_EXE" = /srv/dragonwilds/RSDragonwilds/Binaries/Linux/RSDragonwildsServer-Linux-Shipping ]
)
(
  unset DW_SERVER_ROOT DW_STATUS_DATA DW_ZANIK_ROOT DW_DATA_ROOT DW_WEB_ROOT DW_GAME_LOG DW_WORLD_SAVE DW_ORIGINAL_EXE
  zanik_defaults
  zanik_export_runtime /srv/dragonwilds replace-executable
  [ "$DW_ORIGINAL_EXE" = /srv/dragonwilds/RSDragonwilds/Binaries/Linux/RSDragonwildsServer-Linux-Shipping.zanik-original ]
)

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
printf 'FEATURE_QOL_AUTO_COMPOST=true\n' > "$TMP/incomplete.conf"
if zanik_load_config "$TMP/incomplete.conf" >/dev/null 2>&1; then exit 1; fi
echo 'configuration tests passed'
