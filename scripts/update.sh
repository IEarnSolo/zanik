#!/bin/sh
set -eu
SOURCE_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
. "$SOURCE_DIR/lib/zanik-common.sh" 2>/dev/null || . "$SOURCE_DIR/scripts/lib/zanik-common.sh"
server_root=
while [ $# -gt 0 ]; do
  case "$1" in --server-root) [ $# -ge 2 ] || exit 2; server_root=$2; shift 2;; *) zanik_log "Unknown update option: $1"; exit 2;; esac
done
[ -n "$server_root" ] || { zanik_log '--server-root is required'; exit 2; }
state=$server_root/zanik/install-state.json
[ -f "$state" ] || { zanik_log 'Zanik is not installed; run install.sh first'; exit 1; }
mode=$(sed -n 's/.*"mode":"\([^"]*\)".*/\1/p' "$state")
case "$mode" in wrapper|replace-executable) ;; *) zanik_log 'Installation manifest has an invalid mode'; exit 1;; esac
stamp=$(date -u +%Y%m%dT%H%M%SZ)
backup=$server_root/zanik-data/update-history/$stamp
mkdir -p "$backup"
zanik_backup_manifest "$backup" cli-update "$stamp" preparing
cp -a "$server_root/zanik" "$backup/zanik-install"
game=$(zanik_game_path "$server_root")
cp -p "$game" "$backup/game-executable"
[ ! -e "$game.zanik-original" ] || cp -p "$game.zanik-original" "$backup/game-original"
for path in "$server_root/zanik-data"/*.db "$server_root/zanik-data"/*.log "$server_root/zanik-data"/*.key "$server_root/zanik-data"/*.bin; do [ ! -e "$path" ] || cp -p "$path" "$backup/"; done
set -- --server-root "$server_root" --mode "$mode"
[ "$mode" != replace-executable ] || set -- "$@" --acknowledge-core-file-replacement
if ! "$SOURCE_DIR/scripts/install.sh" "$@"; then
  rm -rf "$server_root/zanik"
  cp -a "$backup/zanik-install" "$server_root/zanik"
  cp -p "$backup/game-executable" "$game"
  if [ -e "$backup/game-original" ]; then cp -p "$backup/game-original" "$game.zanik-original"; else rm -f "$game.zanik-original"; fi
  zanik_backup_manifest "$backup" cli-update "$stamp" failed
  zanik_log "Update failed; the prior installation was restored and its backup remains at $backup"
  exit 1
fi
zanik_backup_manifest "$backup" cli-update "$stamp" complete
zanik_prune_backup_family "$server_root/zanik-data/update-history" cli-update 1
zanik_log "Update completed; pre-update data retained at $backup"
