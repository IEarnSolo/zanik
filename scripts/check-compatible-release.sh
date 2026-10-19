#!/bin/sh
set -eu

sha=
output=
while [ $# -gt 0 ]; do
  case "$1" in
    --executable-sha256) [ $# -ge 2 ] || exit 2; sha=$2; shift 2;;
    --output) [ $# -ge 2 ] || exit 2; output=$2; shift 2;;
    *) exit 2;;
  esac
done
case "$sha" in ????????????????????????????????????????????????????????????????) ;; *) exit 2;; esac
case "$sha" in *[!0-9a-f]*) exit 2;; esac
case "$output" in /*) ;; *) exit 2;; esac

mkdir -p "$(dirname "$output")"
temporary=$output.new.$$
cleanup(){ rm -f "$temporary" "$temporary.release" "$temporary.manifest"; }
trap cleanup EXIT INT TERM
checked=$(date -u +%Y-%m-%dT%H:%M:%SZ)
write(){
  printf '{"schemaVersion":1,"checkedAt":"%s","status":"%s","version":"%s","releaseUrl":"%s"}\n' \
    "$checked" "$1" "$2" "$3" >"$temporary"
  chmod 600 "$temporary"
  mv -f "$temporary" "$output"
}

if ! curl -fsS --connect-timeout 10 --max-time 30 -H 'Accept: application/vnd.github+json' \
    -H 'User-Agent: Zanik-maintenance-checker' \
    https://api.github.com/repos/IEarnSolo/zanik/releases/latest >"$temporary.release"; then
  write check-failed '' ''
  exit 0
fi
tag=$(sed -n 's/^[[:space:]]*"tag_name":[[:space:]]*"\(v[0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\)".*/\1/p' "$temporary.release" | head -n 1)
case "$tag" in v[0-9]*.[0-9]*.[0-9]*) ;; *) write check-failed '' ''; exit 0;; esac
version=${tag#v}
release_url=https://github.com/IEarnSolo/zanik/releases/tag/$tag
if ! curl -fsS --connect-timeout 10 --max-time 30 \
    "https://github.com/IEarnSolo/zanik/releases/download/$tag/release-manifest.json" >"$temporary.manifest"; then
  write check-failed '' ''
  exit 0
fi
manifest_tag=$(sed -n 's/^[[:space:]]*"tag":[[:space:]]*"\([^"]*\)".*/\1/p' "$temporary.manifest" | head -n 1)
[ "$manifest_tag" = "$tag" ] || { write check-failed '' ''; exit 0; }
if awk '/"gameCompatibility"[[:space:]]*:/{section=1} section{print} section&&/][,]?$/{exit}' "$temporary.manifest" | grep -Fq "\"$sha\""; then
  write compatible-release-available "$version" "$release_url"
else
  write no-compatible-release "$version" "$release_url"
fi
