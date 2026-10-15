#!/bin/sh
set -eu

SELF_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ZANIK_DIR=$(CDPATH= cd -- "$SELF_DIR/.." && pwd)
. "$ZANIK_DIR/lib/zanik-common.sh"

server_root=
request_id=
while [ $# -gt 0 ]; do
  case "$1" in
    --server-root) [ $# -ge 2 ] || exit 2; server_root=$2; shift 2;;
    --request-id) [ $# -ge 2 ] || exit 2; request_id=$2; shift 2;;
    *) exit 2;;
  esac
done

case "$server_root" in /*) ;; *) exit 2;; esac
case "$server_root" in /|/home) exit 2;; esac
case "$request_id" in ''|*[!0-9a-f]*) exit 2;; esac
[ ${#request_id} -ge 16 ] && [ ${#request_id} -le 64 ] || exit 2

state_dir=$server_root/zanik/state
status_file=$state_dir/game-update-status.json
manifest=$server_root/steamapps/appmanifest_4019830.acf
steamcmd=$server_root/steamcmd/steamcmd.sh
lock=$state_dir/game-update-check.lock
work=$state_dir/game-update-$request_id.tmp
mkdir -p "$state_dir"

write_status(){
  checked=$(date -u +%Y-%m-%dT%H:%M:%SZ)
  temporary=$status_file.new.$$
  printf '{"schemaVersion":1,"requestId":"%s","checkedAt":"%s","status":"%s","installedBuild":"%s","latestBuild":"%s","updateAvailable":%s,"source":"steamcmd-public-branch"}\n' \
    "$request_id" "$checked" "$1" "$2" "$3" "$4" >"$temporary"
  chmod 600 "$temporary"
  mv -f "$temporary" "$status_file"
}

if ! mkdir "$lock" 2>/dev/null; then
  write_status busy "" "" null
  exit 0
fi
cleanup(){ rm -f "$work"; rmdir "$lock" 2>/dev/null || true; }
trap cleanup EXIT INT TERM

installed=$(awk -F'"' '/"buildid"/{print $4;exit}' "$manifest" 2>/dev/null || true)
case "$installed" in ''|*[!0-9]*) write_status missing-installed-build "" "" null; exit 0;; esac
[ -x "$steamcmd" ] || { write_status steamcmd-unavailable "$installed" "" null; exit 0; }

if ! timeout 90 "$steamcmd" +login anonymous +app_info_update 1 +app_info_print 4019830 +quit >"$work" 2>&1; then
  write_status check-failed "$installed" "" null
  exit 0
fi
latest=$(awk '/"public"/{p=1;next} p&&/"buildid"/{gsub(/[^0-9]/,"");print;exit}' "$work")
case "$latest" in ''|*[!0-9]*) write_status invalid-steam-response "$installed" "" null; exit 0;; esac
if [ "$installed" = "$latest" ]; then
  write_status current "$installed" "$latest" false
else
  write_status update-available "$installed" "$latest" true
fi
