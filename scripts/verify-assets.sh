#!/bin/sh
set -eu

[ "$#" -eq 3 ] || { printf 'usage: verify-assets.sh ASSET_ROOT REPORT REQUEST_ID\n' >&2; exit 2; }
asset_root=$1
report=$2
request_id=$3
case "$request_id" in ''|*[!0-9a-f]*) printf 'invalid request id\n' >&2; exit 2;; esac
[ ${#request_id} -le 64 ] || { printf 'invalid request id\n' >&2; exit 2; }

manifest=$asset_root/asset-manifest.json
[ -f "$manifest" ] && [ ! -L "$manifest" ] || { printf 'asset manifest is missing\n' >&2; exit 1; }
command -v sha256sum >/dev/null 2>&1 || { printf 'sha256sum is unavailable\n' >&2; exit 1; }
command -v xargs >/dev/null 2>&1 || { printf 'xargs is unavailable\n' >&2; exit 1; }

umask 077
report_dir=$(dirname -- "$report")
mkdir -p "$report_dir"
work=$(mktemp -d "$report_dir/.asset-verify.XXXXXX")
cleanup(){ rm -rf "$work"; }
trap cleanup EXIT INT TERM

# Zanik writes one path and one SHA-256 property per line in each manifest
# file record. Reject anything outside that deliberately narrow format.
awk '
  /^[[:space:]]*"path": "/ {
    value=$0; sub(/^[[:space:]]*"path": "/,"",value); sub(/",[[:space:]]*$/,"",value); path=value; next
  }
  /^[[:space:]]*"sha256": "/ && path != "" {
    value=$0; sub(/^[[:space:]]*"sha256": "/,"",value); sub(/",[[:space:]]*$/,"",value)
    if (length(value)==64) print value "\t" path
    path=""
  }
' "$manifest" > "$work/expected.tsv"

checked=0
missing=0
mismatches=0
: > "$work/paths.bin"
tab=$(printf '\t')
while IFS="$tab" read -r expected relative; do
  [ -n "$expected" ] && [ -n "$relative" ] || continue
  case "$relative" in /*|*..*|*\\*|*\"*) missing=$((missing+1)); continue;; esac
  path=$asset_root/$relative
  if [ ! -f "$path" ] || [ -L "$path" ]; then
    missing=$((missing+1))
    continue
  fi
  printf '%s\0' "$relative" >> "$work/paths.bin"
done < "$work/expected.tsv"

expected_count=$(wc -l < "$work/expected.tsv" | tr -d ' ')
[ "$expected_count" -gt 0 ] || { printf 'asset manifest contains no file records\n' >&2; exit 1; }
: > "$work/computed.raw"
if [ -s "$work/paths.bin" ]; then
  (CDPATH= cd -- "$asset_root" && xargs -0 sha256sum -- < "$work/paths.bin") > "$work/computed.raw" 2>/dev/null || true
fi
awk '{ hash=$1; sub(/^[^ ]+  /, ""); print hash "\t" $0 }' "$work/computed.raw" > "$work/computed.tsv"
set -- $(awk -F '\t' '
  NR==FNR { expected[$2]=$1; next }
  { checked++; if (!($2 in expected) || expected[$2]!=$1) mismatches++ }
  END { print checked+0, mismatches+0 }
' "$work/expected.tsv" "$work/computed.tsv")
checked=$1
mismatches=$2
aggregate=$(sha256sum "$work/computed.tsv" | awk '{print $1}')
manifest_hash=$(sha256sum "$manifest" | awk '{print $1}')
status=valid
[ "$checked" -eq "$expected_count" ] && [ "$missing" -eq 0 ] && [ "$mismatches" -eq 0 ] || status=invalid
temporary=$report.new.$$
printf '{"schemaVersion":1,"requestId":"%s","status":"%s","expected":%s,"checked":%s,"missing":%s,"mismatches":%s,"aggregateSha256":"%s","manifestSha256":"%s"}\n' \
  "$request_id" "$status" "$expected_count" "$checked" "$missing" "$mismatches" "$aggregate" "$manifest_hash" > "$temporary"
mv -f "$temporary" "$report"
trap - EXIT INT TERM
cleanup
