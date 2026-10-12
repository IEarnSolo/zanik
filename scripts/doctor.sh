#!/bin/sh
set -u
SELF_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
if [ -f "$SELF_DIR/lib/zanik-common.sh" ]; then . "$SELF_DIR/lib/zanik-common.sh"; else . "$SELF_DIR/../lib/zanik-common.sh"; fi
server_root=
json=0
while [ $# -gt 0 ]; do
  case "$1" in --server-root) [ $# -ge 2 ] || exit 2; server_root=$2; shift 2;; --json) json=1; shift;; *) zanik_log "Unknown doctor option: $1"; exit 2;; esac
done
[ -n "$server_root" ] || { zanik_log '--server-root is required'; exit 2; }
install=$server_root/zanik
game=$(zanik_game_path "$server_root")
mode=$(sed -n 's/.*"mode":"\([^"]*\)".*/\1/p' "$install/install-state.json" 2>/dev/null || printf unknown)
game_target=$game
[ "$mode" != replace-executable ] || game_target=$game.zanik-original
config_valid=true
zanik_defaults
zanik_load_config "$install/config/zanik.conf" >/dev/null 2>&1 || config_valid=false
healthy=true
[ -x "$game_target" ] || healthy=false
[ -s "$install/bin/libzanik.so" ] || healthy=false
[ -x "$install/bin/zanik-server" ] || healthy=false
[ -s "$server_root/zanik-data/console-admin.key" ] || healthy=false
hash_matches(){
  field=$1 path=$2
  expected=$(sed -n "s/.*\"$field\":\"\([^\"]*\)\".*/\1/p" "$install/install-state.json" 2>/dev/null)
  [ -n "$expected" ] && [ -f "$path" ] && [ "$(zanik_sha256 "$path")" = "$expected" ]
}
hash_matches librarySha256 "$install/bin/libzanik.so" || healthy=false
hash_matches analyzerSha256 "$install/bin/zanik-compat-analyzer" || healthy=false
hash_matches launcherSha256 "$install/bin/zanik-server" || healthy=false
assets=false
[ -f "$install/assets/current/asset-manifest.json" ] && assets=true
if [ "$json" -eq 1 ]; then
  printf '{"healthy":%s,"configValid":%s,"mode":"%s","assetsInstalled":%s}\n' "$healthy" "$config_valid" "$mode" "$assets"
else
  zanik_log "healthy=$healthy configValid=$config_valid mode=$mode assetsInstalled=$assets"
fi
[ "$healthy" = true ] && [ "$config_valid" = true ]
