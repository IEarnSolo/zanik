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
write_report(){
  status=$1 changed=${2:-0} removed=${3:-0} count=${4:-0} reason=${5:-none}
  temporary=$report.new.$$
  printf '{"schemaVersion":1,"requestId":"%s","status":"%s","reason":"%s","changed":%s,"removed":%s,"fileCount":%s}\n' \
    "$request_id" "$status" "$reason" "$changed" "$removed" "$count" >"$temporary"
  chmod 600 "$temporary"; mv -f "$temporary" "$report"
}
safe_relative(){
  case "$1" in ''|/*|*\\*|../*|*/../*|*/..|*//* ) return 1;; esac
  return 0
}
required='asset-delta.json target-asset-manifest.json target-files.sha256 target-files.list removed.list base-manifest.sha256'
for name in $required; do [ -f "$stage/$name" ] || { write_report failed 0 0 0 missing-input; exit 0; }; done
current=$ZANIK_DIR/assets/current
[ -f "$current/asset-manifest.json" ] || { write_report failed 0 0 0 missing-current; exit 0; }
expected_base=$(tr -d '\r\n' <"$stage/base-manifest.sha256")
actual_base=$(sha256sum "$current/asset-manifest.json" | awk '{print $1}')
[ "$actual_base" = "$expected_base" ] || { write_report base-mismatch; exit 0; }
candidate=$ZANIK_DIR/assets/.delta-$request_id.new
target=$ZANIK_DIR/assets/build-delta-$request_id
carriage_return=$(printf '\r')
rm -rf "$candidate"
mkdir -p "$candidate"
if ! cp -al "$current/." "$candidate/" 2>/dev/null; then
  rm -rf "$candidate"; mkdir -p "$candidate"; cp -a "$current/." "$candidate/"
fi
while IFS= read -r relative; do
  # Manager runs on Windows, whose text-mode writes may use CRLF. Strip the
  # transport carriage return before validating or applying a removal path.
  relative=${relative%"$carriage_return"}
  [ -n "$relative" ] || continue
  safe_relative "$relative" || { rm -rf "$candidate"; write_report failed 0 0 0 unsafe-removal; exit 0; }
  rm -f "$candidate/$relative"
done <"$stage/removed.list"
changed=0
if [ -d "$stage/files" ]; then
  find "$stage/files" -type f | while IFS= read -r source; do
    relative=${source#"$stage/files/"}
    safe_relative "$relative" || exit 9
    destination=$candidate/$relative
    mkdir -p "$(dirname -- "$destination")"
    cp "$source" "$destination.new" && mv -f "$destination.new" "$destination"
  done
  copy_status=$?
  [ "$copy_status" -eq 0 ] || { rm -rf "$candidate"; write_report failed 0 0 0 copy-changed; exit 0; }
  changed=$(find "$stage/files" -type f | wc -l | tr -d ' ')
fi
cp "$stage/target-asset-manifest.json" "$candidate/asset-manifest.json.new"
mv -f "$candidate/asset-manifest.json.new" "$candidate/asset-manifest.json"
if ! (cd "$candidate" && sha256sum -c "$stage/target-files.sha256" >/dev/null 2>&1); then
  rm -rf "$candidate"; write_report failed 0 0 0 checksum; exit 0
fi
actual_list=$stage/actual-files.$$
expected_list=$stage/expected-files.$$
(cd "$candidate" && find . -type f | sed 's#^\./##' | LC_ALL=C sort) >"$actual_list"
tr -d '\r' <"$stage/target-files.list" | LC_ALL=C sort >"$expected_list"
if ! cmp -s "$actual_list" "$expected_list"; then
  rm -f "$actual_list" "$expected_list"; rm -rf "$candidate"; write_report failed 0 0 0 file-set; exit 0
fi
rm -f "$actual_list" "$expected_list"
rm -rf "$target"
mv "$candidate" "$target"
if [ -L "$current" ]; then
  old_link=$(readlink "$current")
  if ! rm -f "$current" || ! mv "$target" "$current"; then
    rm -rf "$current"; ln -s "$old_link" "$current"
    write_report failed 0 0 0 activation-swap; exit 0
  fi
else
  legacy=$ZANIK_DIR/assets/build-pre-delta-$request_id
  if ! mv "$current" "$legacy" || ! mv "$target" "$current"; then
    rm -rf "$current"
    [ ! -d "$legacy" ] || mv "$legacy" "$current"
    write_report failed 0 0 0 activation-migration; exit 0
  fi
fi
removed=$(wc -l <"$stage/removed.list" | tr -d ' ')
count=$(wc -l <"$stage/target-files.list" | tr -d ' ')
count=$((count - 1))
write_report activated "$changed" "$removed" "$count"
