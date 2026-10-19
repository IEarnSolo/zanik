#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
mkdir -p "$tmp/bin"
cat >"$tmp/bin/curl" <<'EOF'
#!/bin/sh
for value in "$@"; do url=$value; done
case "$url" in
  */releases/latest) printf '%s\n' '{' '  "tag_name": "v9.8.7",' '  "draft": false' '}' ;;
  */release-manifest.json) printf '%s\n' '{' '  "tag": "v9.8.7",' '  "gameCompatibility": {' '    "schemaVersion": 1,' '    "linuxExecutableSha256": [' '      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"' '    ]' '  }' '}' ;;
  *) exit 22;;
esac
EOF
chmod +x "$tmp/bin/curl"
PATH="$tmp/bin:$PATH" "$root/scripts/check-compatible-release.sh" \
  --executable-sha256 aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa --output "$tmp/available.json"
grep -q '"status":"compatible-release-available"' "$tmp/available.json"
PATH="$tmp/bin:$PATH" "$root/scripts/check-compatible-release.sh" \
  --executable-sha256 bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb --output "$tmp/unavailable.json"
grep -q '"status":"no-compatible-release"' "$tmp/unavailable.json"
echo 'compatible release checker: ok'
