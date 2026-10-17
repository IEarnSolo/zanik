#!/bin/sh
set -eu
SELF_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ZANIK_DIR=$(CDPATH= cd -- "$SELF_DIR/.." && pwd)
server_root= request_id=
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
state_dir=$ZANIK_DIR/state
stage=$server_root/.zanik-manager/staging/asset-delta-$request_id
report=$state_dir/asset-delta-status-$request_id.json
lock=$state_dir/asset-delta.lock
mkdir -p "$state_dir"
if ! mkdir "$lock" 2>/dev/null; then
  temporary=$report.new.$$
  printf '{"schemaVersion":1,"requestId":"%s","status":"busy"}\n' "$request_id" >"$temporary"
  chmod 600 "$temporary"; mv -f "$temporary" "$report"; exit 0
fi
cleanup(){ rmdir "$lock" 2>/dev/null || true; }
trap cleanup EXIT INT TERM
python3 "$ZANIK_DIR/tools/zanik_assets.py" install-delta --delta-root "$stage" \
  --destination "$ZANIK_DIR/assets" --request-id "$request_id" --report "$report" >/dev/null 2>&1 || true
