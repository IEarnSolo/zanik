#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
VERSION=$(tr -d '\r\n' < "$ROOT/VERSION")
OUT=${1:-$ROOT/dist/release}
mkdir -p "$OUT"
OUT=$(CDPATH= cd -- "$OUT" && pwd)
WORK=$OUT/.work
SOURCE_DATE_EPOCH=${SOURCE_DATE_EPOCH:-315532800}; export SOURCE_DATE_EPOCH
RELEASE_TAG=${RELEASE_TAG:-}
RELEASE_COMMIT=${RELEASE_COMMIT:-}
rm -rf "$WORK"
mkdir -p "$WORK/server/bin" "$WORK/server/lib" "$WORK/server/config" "$WORK/server/compatibility" "$WORK/server/scripts" "$WORK/server/tools" "$WORK/server/docs"

install -m 0755 "$ROOT/dist/libdwstatus.so" "$WORK/server/bin/libzanik.so"
install -m 0755 "$ROOT/dist/zanik-compat-analyzer" "$WORK/server/bin/zanik-compat-analyzer"
install -m 0755 "$ROOT/scripts/zanik-server" "$WORK/server/bin/zanik-server"
install -m 0644 "$ROOT/scripts/lib/zanik-common.sh" "$WORK/server/lib/zanik-common.sh"
install -m 0644 "$ROOT/config/zanik.conf.example" "$WORK/server/config/zanik.conf.example"
install -m 0644 "$ROOT/deploy/compatibility.tsv" "$WORK/server/compatibility/compatibility.tsv"
for file in install update uninstall doctor init-assets compat-launcher; do install -m 0755 "$ROOT/scripts/$file.sh" "$WORK/server/scripts/$file.sh"; done
install -m 0644 "$ROOT/tools/zanik_assets.py" "$WORK/server/tools/zanik_assets.py"
install -m 0644 "$ROOT/VERSION" "$ROOT/CHANGELOG.md" "$ROOT/LICENSE" "$ROOT/THIRD_PARTY_NOTICES.md" "$ROOT/README.md" "$WORK/server/"
for file in installation.md linux.md pterodactyl.md docker.md upgrades.md uninstall.md configuration.md assets.md security.md hosting.md curseforge.md releasing.md contributor-architecture.md; do install -m 0644 "$ROOT/docs/$file" "$WORK/server/docs/$file"; done

normalize(){ find "$1" -type f -exec touch -h -d "@$SOURCE_DATE_EPOCH" {} +; find "$1" -type d -exec touch -h -d "@$SOURCE_DATE_EPOCH" {} +; }
normalize "$WORK/server"
SERVER_TAR=$OUT/zanik-server-linux-x86_64-$VERSION.tar.gz
SERVER_ZIP=$OUT/zanik-server-linux-x86_64-$VERSION.zip
tar --sort=name --mtime="@$SOURCE_DATE_EPOCH" --owner=0 --group=0 --numeric-owner -C "$WORK/server" -czf "$SERVER_TAR" .
(cd "$WORK/server" && find . -type f -o -type l | LC_ALL=C sort | zip -X -q "$SERVER_ZIP" -@)

python3 - "$ROOT/asset-tools/tools.lock.json" "$WORK" <<'PY'
import json, pathlib, sys, urllib.request, hashlib
lock=pathlib.Path(sys.argv[1]); work=pathlib.Path(sys.argv[2]); data=json.loads(lock.read_text())['tools']['retoc']
for platform, suffix in [('linux-x86_64','tar.gz'),('windows-x86_64','zip')]:
    record=data[platform]; target=work/f'retoc-{platform}.{suffix}'
    urllib.request.urlretrieve(record['url'], target)
    actual=hashlib.sha256(target.read_bytes()).hexdigest()
    if actual != record['sha256']: raise SystemExit(f'checksum mismatch for {platform}: {actual}')
PY
mkdir -p "$WORK/asset-linux/tools" "$WORK/asset-windows/tools"
tar -xJf "$WORK/retoc-linux-x86_64.tar.xz" -C "$WORK/asset-linux"
unzip -q "$WORK/retoc-windows-x86_64.zip" -d "$WORK/asset-windows"
for dir in asset-linux asset-windows; do
  install -m 0644 "$ROOT/tools/zanik_assets.py" "$WORK/$dir/tools/zanik_assets.py"
  install -m 0644 "$ROOT/asset-tools/tools.lock.json" "$WORK/$dir/tools.lock.json"
  install -m 0644 "$ROOT/THIRD_PARTY_NOTICES.md" "$WORK/$dir/THIRD_PARTY_NOTICES.md"
done
install -m 0755 "$ROOT/scripts/init-assets.sh" "$WORK/asset-linux/init-assets.sh"
install -m 0644 "$ROOT/scripts/init-assets.ps1" "$WORK/asset-windows/init-assets.ps1"
normalize "$WORK/asset-linux"; normalize "$WORK/asset-windows"
tar --sort=name --mtime="@$SOURCE_DATE_EPOCH" --owner=0 --group=0 --numeric-owner -C "$WORK/asset-linux" -czf "$OUT/zanik-asset-tools-linux-x86_64-$VERSION.tar.gz" .
(cd "$WORK/asset-windows" && find . -type f | LC_ALL=C sort | zip -X -q "$OUT/zanik-asset-tools-windows-x86_64-$VERSION.zip" -@)
(cd "$OUT" && sha256sum zanik-*-$VERSION.* | LC_ALL=C sort > SHA256SUMS)
python3 "$ROOT/tools/release_manifest.py" --directory "$OUT" --version "$VERSION" --tag "$RELEASE_TAG" --commit "$RELEASE_COMMIT" --source-date-epoch "$SOURCE_DATE_EPOCH" --output "$OUT/release-manifest.json"
if [ -n "$RELEASE_TAG" ]; then
  python3 "$ROOT/tools/release_contract.py" --tag "$RELEASE_TAG" --notes-output "$OUT/release-notes.md"
fi
rm -rf "$WORK"
