#!/bin/sh
set -eu

SOURCE_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
. "$SOURCE_DIR/lib/zanik-common.sh" 2>/dev/null || . "$SOURCE_DIR/scripts/lib/zanik-common.sh"
server_root=
mode=wrapper
ack=0
while [ $# -gt 0 ]; do
  case "$1" in
    --server-root) [ $# -ge 2 ] || exit 2; server_root=$2; shift 2;;
    --mode) [ $# -ge 2 ] || exit 2; mode=$2; shift 2;;
    --acknowledge-core-file-replacement) ack=1; shift;;
    *) zanik_log "Unknown install option: $1"; exit 2;;
  esac
done
[ -n "$server_root" ] || { zanik_log '--server-root is required'; exit 2; }
case "$server_root" in /*) ;; *) zanik_log '--server-root must be absolute'; exit 2;; esac
case "$server_root" in /|/home|/home/container/..) zanik_log 'Refusing unsafe server root'; exit 2;; esac
case "$mode" in wrapper) ;; replace-executable) [ "$ack" -eq 1 ] || { zanik_log 'Replacement mode requires --acknowledge-core-file-replacement'; exit 2; };; *) zanik_log 'Mode must be wrapper or replace-executable'; exit 2;; esac

game=$(zanik_game_path "$server_root")
[ -s "$game" ] || { zanik_log "Official server executable not found: $game"; exit 1; }
[ -x "$game" ] || { zanik_log "Official server executable is not executable: $game"; exit 1; }
original=$game.zanik-original
if [ "$mode" = wrapper ] && grep -q 'ZANIK_COMPAT_LAUNCHER=1' "$game" 2>/dev/null; then
  [ -x "$original" ] || { zanik_log 'Existing Zanik launcher has no preserved original executable'; exit 1; }
  mv "$game" "$game.zanik-mode-change"
  mv "$original" "$game"
  rm -f "$game.zanik-mode-change"
fi
install_dir=$server_root/zanik
data_dir=$server_root/zanik-data
mkdir -p "$install_dir/bin" "$install_dir/lib" "$install_dir/config" "$install_dir/state" "$install_dir/assets" "$install_dir/docs" "$install_dir/tools" "$install_dir/scripts" "$data_dir"
chmod 700 "$data_dir"

copy_required(){ [ -f "$1" ] || { zanik_log "Release payload is missing $1"; exit 1; }; cp -p "$1" "$2"; }
copy_required "$SOURCE_DIR/bin/libzanik.so" "$install_dir/bin/libzanik.so"
copy_required "$SOURCE_DIR/bin/zanik-compat-analyzer" "$install_dir/bin/zanik-compat-analyzer"
copy_required "$SOURCE_DIR/bin/zanik-server" "$install_dir/bin/zanik-server"
copy_required "$SOURCE_DIR/lib/zanik-common.sh" "$install_dir/lib/zanik-common.sh"
copy_required "$SOURCE_DIR/config/zanik.conf.example" "$install_dir/config/zanik.conf.example"
copy_required "$SOURCE_DIR/compatibility/compatibility.tsv" "$install_dir/compatibility.tsv"
for command in install update uninstall doctor init-assets; do copy_required "$SOURCE_DIR/scripts/$command.sh" "$install_dir/scripts/$command.sh"; done
copy_required "$SOURCE_DIR/scripts/compat-launcher.sh" "$install_dir/scripts/compat-launcher.sh"
copy_required "$SOURCE_DIR/tools/zanik_assets.py" "$install_dir/tools/zanik_assets.py"
[ -f "$install_dir/config/zanik.conf" ] || cp -p "$install_dir/config/zanik.conf.example" "$install_dir/config/zanik.conf"
[ -f "$install_dir/admins.txt" ] || { umask 077; : > "$install_dir/admins.txt"; }
chmod 755 "$install_dir/bin/zanik-server" "$install_dir/bin/zanik-compat-analyzer" "$install_dir/scripts"/*.sh
chmod 600 "$install_dir/config/zanik.conf" "$install_dir/admins.txt"
zanik_make_secret "$data_dir/console-admin.key"

if [ "$mode" = replace-executable ]; then
  if grep -q 'ZANIK_COMPAT_LAUNCHER=1' "$game" 2>/dev/null; then
    [ -x "$original" ] || { zanik_log 'Existing Zanik launcher has no preserved original executable'; exit 1; }
  else
    if [ -e "$original" ]; then
      stamp=$(date -u +%Y%m%dT%H%M%SZ)
      mkdir -p "$data_dir/update-history/$stamp"
      mv "$original" "$data_dir/update-history/$stamp/official-before-steam-update"
      zanik_log "Steam replaced the compatibility launcher; preserved the prior official binary in update-history/$stamp."
    fi
    mv "$game" "$original"
    cp -p "$install_dir/scripts/compat-launcher.sh" "$game.tmp"
    chmod 755 "$game.tmp"
    mv "$game.tmp" "$game"
  fi
fi

version=$(cat "$SOURCE_DIR/VERSION" 2>/dev/null || printf development)
game_target=$game
[ "$mode" = replace-executable ] && game_target=$game.zanik-original
game_sha=$(zanik_sha256 "$game_target")
library_sha=$(zanik_sha256 "$install_dir/bin/libzanik.so")
analyzer_sha=$(zanik_sha256 "$install_dir/bin/zanik-compat-analyzer")
launcher_sha=$(zanik_sha256 "$install_dir/bin/zanik-server")
installed_at=$(date -u +%FT%TZ)
cat > "$install_dir/install-state.json.tmp" <<EOF
{"schemaVersion":1,"version":"$(zanik_json_escape "$version")","mode":"$mode","serverRoot":"$(zanik_json_escape "$server_root")","officialExecutable":"$(zanik_json_escape "$game_target")","officialSha256":"$game_sha","librarySha256":"$library_sha","analyzerSha256":"$analyzer_sha","launcherSha256":"$launcher_sha","installedAt":"$installed_at"}
EOF
mv "$install_dir/install-state.json.tmp" "$install_dir/install-state.json"
chmod 600 "$install_dir/install-state.json"
zanik_log "Installed Zanik $version in $mode mode."
if [ "$mode" = wrapper ]; then zanik_log "Set the server startup command to: $install_dir/bin/zanik-server --server-root $server_root --"; else zanik_log 'The existing egg may continue launching the original executable path.'; fi
