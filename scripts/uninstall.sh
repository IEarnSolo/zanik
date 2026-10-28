#!/bin/sh
set -eu
SELF_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
if [ -f "$SELF_DIR/lib/zanik-common.sh" ]; then . "$SELF_DIR/lib/zanik-common.sh"; else . "$SELF_DIR/../lib/zanik-common.sh"; fi
server_root=
purge=0
while [ $# -gt 0 ]; do
  case "$1" in --server-root) [ $# -ge 2 ] || exit 2; server_root=$2; shift 2;; --purge-data) purge=1; shift;; *) zanik_log "Unknown uninstall option: $1"; exit 2;; esac
done
[ -n "$server_root" ] || { zanik_log '--server-root is required'; exit 2; }
case "$server_root" in /*) ;; *) zanik_log '--server-root must be absolute'; exit 2;; esac
game=$(zanik_game_path "$server_root")
state=$server_root/zanik/install-state.json
mode=$(sed -n 's/.*"mode":"\([^"]*\)".*/\1/p' "$state" 2>/dev/null || true)
data_dir=$server_root/zanik-data
data_was_present=0
[ ! -d "$data_dir" ] || data_was_present=1
if [ "$mode" = replace-executable ]; then
  original=$game.zanik-original
  grep -q 'ZANIK_COMPAT_LAUNCHER=1' "$game" 2>/dev/null || { zanik_log 'Refusing to replace an executable not managed by Zanik'; exit 1; }
  [ -x "$original" ] || { zanik_log 'Preserved official executable is missing'; exit 1; }
  mv "$game" "$game.zanik-uninstalled"
  mv "$original" "$game"
  rm -f "$game.zanik-uninstalled"
  [ -x "$game" ] && [ ! -e "$original" ] || { zanik_log 'Official executable restoration did not verify'; exit 1; }
fi
install_dir=$server_root/zanik
[ "$install_dir" != / ] || exit 1
rm -rf "$install_dir"
[ ! -e "$install_dir" ] || { zanik_log 'Installed Zanik files remain after uninstall'; exit 1; }
if [ "$purge" -eq 1 ]; then
  [ "$data_dir" != / ] || exit 1
  rm -rf "$data_dir"
  [ ! -e "$data_dir" ] || { zanik_log 'Persistent data remains after the requested purge'; exit 1; }
  zanik_log 'Zanik and persistent data removed.'
else
  if [ "$data_was_present" -eq 1 ] && [ ! -d "$data_dir" ]; then
    zanik_log 'Persistent data was not preserved; uninstall verification failed'
    exit 1
  fi
  zanik_log "Zanik removed; persistent data retained at $server_root/zanik-data"
fi
