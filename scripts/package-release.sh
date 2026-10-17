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
install -m 0644 "$ROOT/config/zanik-config.schema.json" "$WORK/server/config/zanik-config.schema.json"
install -m 0644 "$ROOT/deploy/compatibility.tsv" "$WORK/server/compatibility/compatibility.tsv"
for file in install update uninstall doctor init-assets compat-launcher verify-assets check-game-update; do install -m 0755 "$ROOT/scripts/$file.sh" "$WORK/server/scripts/$file.sh"; done
install -m 0644 "$ROOT/tools/zanik_assets.py" "$WORK/server/tools/zanik_assets.py"
install -m 0644 "$ROOT/tools/asset_filesystem.py" "$WORK/server/tools/asset_filesystem.py"
install -m 0644 "$ROOT/VERSION" "$ROOT/CHANGELOG.md" "$ROOT/LICENSE" "$ROOT/THIRD_PARTY_NOTICES.md" "$ROOT/README.md" "$WORK/server/"
for file in installation.md linux.md pterodactyl.md docker.md upgrades.md uninstall.md configuration.md assets.md security.md hosting.md curseforge.md releasing.md contributor-architecture.md manager.md; do install -m 0644 "$ROOT/docs/$file" "$WORK/server/docs/$file"; done
python3 "$ROOT/manager/zanik_manager/payload_contract.py" --root "$WORK/server" --release-version "$VERSION" --output "$WORK/server/payload-contract.json"

normalize(){ find "$1" -type f -exec touch -h -d "@$SOURCE_DATE_EPOCH" {} +; find "$1" -type d -exec touch -h -d "@$SOURCE_DATE_EPOCH" {} +; }
normalize "$WORK/server"
SERVER_TAR=$OUT/zanik-server-linux-x86_64-$VERSION.tar.gz
SERVER_ZIP=$OUT/zanik-server-linux-x86_64-$VERSION.zip
tar --sort=name --mtime="@$SOURCE_DATE_EPOCH" --owner=0 --group=0 --numeric-owner -C "$WORK/server" -czf "$SERVER_TAR" .
(cd "$WORK/server" && find . -type f -o -type l | LC_ALL=C sort | zip -X -q "$SERVER_ZIP" -@)

python3 - "$ROOT/asset-tools/tools.lock.json" "$WORK" <<'PY'
import json, pathlib, sys, urllib.request, hashlib
lock=pathlib.Path(sys.argv[1]); work=pathlib.Path(sys.argv[2]); data=json.loads(lock.read_text())['tools']
for name, record in data.items():
    target=work/f'{name}.zip'
    request=urllib.request.Request(record['url'], headers={'Accept':record.get('accept','application/octet-stream'), 'User-Agent':'Zanik-release-builder'})
    with urllib.request.urlopen(request) as response, target.open('wb') as output:
        while block := response.read(1024 * 1024): output.write(block)
    actual=hashlib.sha256(target.read_bytes()).hexdigest()
    if actual != record['sha256']: raise SystemExit(f'checksum mismatch for {name}: {actual}')
PY
python3 - "$ROOT/manager/tools.lock.json" "$WORK" <<'PY'
import json, pathlib, sys, urllib.request, hashlib
lock=pathlib.Path(sys.argv[1]); work=pathlib.Path(sys.argv[2]); data=json.loads(lock.read_text())['tools']
for name, record in data.items():
    if 'url' not in record: continue
    target=work/f'{name}.whl'
    request=urllib.request.Request(record['url'], headers={'Accept':'application/octet-stream','User-Agent':'Zanik-release-builder'})
    with urllib.request.urlopen(request) as response, target.open('wb') as output:
        while block := response.read(1024 * 1024): output.write(block)
    actual=hashlib.sha256(target.read_bytes()).hexdigest()
    if actual != record['sha256']: raise SystemExit(f'checksum mismatch for {name}: {actual}')
PY
mkdir -p "$WORK/asset-linux/tools" "$WORK/asset-windows/tools" "$WORK/asset-windows/runtime" "$WORK/asset-windows/third_party/uas" "$WORK/asset-windows/third_party/ue4ss" "$WORK/asset-windows/asset-tools" "$WORK/asset-windows/deploy"
unzip -q "$WORK/python-embed-windows-x86_64.zip" -d "$WORK/asset-windows/runtime"
unzip -q "$WORK/unreal-asset-scout-windows-x86_64.zip" -d "$WORK/asset-windows/third_party/uas"
unzip -q "$WORK/ue4ss-windows-x86_64.zip" -d "$WORK/asset-windows/third_party/ue4ss"
printf '../tools\n' >> "$WORK/asset-windows/runtime/python312._pth"
install -m 0644 "$ROOT/tools/zanik_assets.py" "$WORK/asset-linux/tools/zanik_assets.py"
install -m 0644 "$ROOT/tools/asset_filesystem.py" "$WORK/asset-linux/tools/asset_filesystem.py"
for file in zanik_assets windows_mapping windows_asset_pipeline generate_building_catalog generate_plan_item_catalog generate_world_catalogs generate_quest_catalog generate_web_assets generate_recipe_unlock_catalog analyze_quest_steps asset_filesystem quest_schema; do
  install -m 0644 "$ROOT/tools/$file.py" "$WORK/asset-windows/tools/$file.py"
done
install -m 0644 "$ROOT/asset-tools/tools.lock.json" "$ROOT/asset-tools/extraction-recipes.json" "$ROOT/asset-tools/catalog-rules.json" "$WORK/asset-windows/asset-tools/"
install -m 0644 "$ROOT/asset-tools/LICENSE-UnrealAssetScout.txt" "$WORK/asset-windows/third_party/uas/LICENSE.txt"
install -m 0644 "$ROOT/deploy/quest-overrides.json" "$ROOT/deploy/quest-step-overrides.json" "$WORK/asset-windows/deploy/"
for dir in asset-linux asset-windows; do
  install -m 0644 "$ROOT/asset-tools/tools.lock.json" "$WORK/$dir/tools.lock.json"
  install -m 0644 "$ROOT/THIRD_PARTY_NOTICES.md" "$WORK/$dir/THIRD_PARTY_NOTICES.md"
done
install -m 0755 "$ROOT/scripts/init-assets.sh" "$WORK/asset-linux/init-assets.sh"
install -m 0644 "$ROOT/scripts/init-assets.ps1" "$WORK/asset-windows/init-assets.ps1"
normalize "$WORK/asset-linux"; normalize "$WORK/asset-windows"
tar --sort=name --mtime="@$SOURCE_DATE_EPOCH" --owner=0 --group=0 --numeric-owner -C "$WORK/asset-linux" -czf "$OUT/zanik-asset-tools-linux-x86_64-$VERSION.tar.gz" .
(cd "$WORK/asset-windows" && find . -type f | LC_ALL=C sort | zip -X -q "$OUT/zanik-asset-tools-windows-x86_64-$VERSION.zip" -@)

# Build the portable Windows manager from the exact server and extractor
# payloads published alongside it.
mkdir -p "$WORK/manager/app" "$WORK/manager/runtime/Lib/site-packages" "$WORK/manager/payload" "$WORK/manager/config"
cp -a "$WORK/asset-windows" "$WORK/manager/asset-tools"
cp -a "$ROOT/manager/zanik_manager" "$WORK/manager/app/"
find "$WORK/manager/app" -type d -name __pycache__ -prune -exec rm -rf {} +
find "$WORK/manager/app" -type f -name '*.pyc' -delete
unzip -q "$WORK/python-embed-windows-x86_64.zip" -d "$WORK/manager/runtime"
unzip -q "$WORK/pyside6-essentials-windows-x86_64.whl" -d "$WORK/manager/runtime/Lib/site-packages"
unzip -q "$WORK/shiboken6-windows-x86_64.whl" -d "$WORK/manager/runtime/Lib/site-packages"
python3 "$ROOT/tools/prune_pyside_runtime.py" "$WORK/manager/runtime/Lib/site-packages"
cat > "$WORK/manager/runtime/python312._pth" <<'EOF'
python312.zip
.
Lib/site-packages
../app
import site
EOF
cp -p "$SERVER_ZIP" "$WORK/manager/payload/server.zip"
cp -p "$ROOT/config/zanik-config.schema.json" "$WORK/manager/config/zanik-config.schema.json"
cp -p "$ROOT/config/server-payload-legacy-contracts.json" "$WORK/manager/config/server-payload-legacy-contracts.json"
cp -p "$ROOT/manager/README.md" "$ROOT/CHANGELOG.md" "$ROOT/LICENSE" "$ROOT/THIRD_PARTY_NOTICES.md" "$ROOT/manager/tools.lock.json" "$WORK/manager/"
cp -p "$ROOT/VERSION" "$WORK/manager/VERSION"
cp -p /usr/share/common-licenses/LGPL-3 "$WORK/manager/LICENSE-LGPL-3.0.txt"
x86_64-w64-mingw32-gcc -municode -mwindows -Os -s "$ROOT/manager/launcher.c" -o "$WORK/manager/ZanikManager.exe"
x86_64-w64-mingw32-gcc -municode -mwindows -Os -s "$ROOT/manager/updater.c" -lshell32 -o "$WORK/manager/ZanikManagerUpdater.exe"
if [ -n "${WINDOWS_SIGNING_CERT_BASE64:-}" ]; then
  [ -n "${WINDOWS_SIGNING_CERT_PASSWORD:-}" ] || { printf 'WINDOWS_SIGNING_CERT_PASSWORD is required when signing\n' >&2; exit 1; }
  printf '%s' "$WINDOWS_SIGNING_CERT_BASE64" | base64 -d > "$WORK/manager-signing.p12"
  for executable in ZanikManager ZanikManagerUpdater; do
    osslsigncode sign -pkcs12 "$WORK/manager-signing.p12" -pass "$WINDOWS_SIGNING_CERT_PASSWORD" \
      -n 'Zanik Manager' -i 'https://github.com/IEarnSolo/zanik' \
      -in "$WORK/manager/$executable.exe" -out "$WORK/manager/$executable.signed.exe"
    mv "$WORK/manager/$executable.signed.exe" "$WORK/manager/$executable.exe"
  done
  rm -f "$WORK/manager-signing.p12"
fi
python3 - "$WORK/manager" "$VERSION" <<'PY'
import hashlib,json,pathlib,sys,zipfile
root=pathlib.Path(sys.argv[1]); version=sys.argv[2]
def digest(path): return hashlib.sha256(path.read_bytes()).hexdigest()
with zipfile.ZipFile(root/'payload/server.zip') as archive:
    files=[]
    for item in archive.infolist():
        if item.is_dir(): continue
        data=archive.read(item); name=item.filename.lstrip('./')
        files.append({'path':name,'size':len(data),'sha256':hashlib.sha256(data).hexdigest(),'mode':(item.external_attr>>16)&0o777 or 0o644})
    contract=json.loads(archive.read('payload-contract.json'))
(root/'payload/server-payload.json').write_text(json.dumps({'schemaVersion':2,'version':version,'archive':'server.zip','archiveSha256':digest(root/'payload/server.zip'),'payloadContractSchemaVersion':contract['schemaVersion'],'payloadContractSha256':contract['contractSha256'],'files':sorted(files,key=lambda x:x['path'])},indent=2,sort_keys=True)+'\n')
asset_files=[]
for path in sorted((root/'asset-tools').rglob('*')):
    if path.is_file(): asset_files.append({'path':path.relative_to(root/'asset-tools').as_posix(),'size':path.stat().st_size,'sha256':digest(path)})
(root/'payload/payloads.json').write_text(json.dumps({'schemaVersion':2,'version':version,'serverSha256':digest(root/'payload/server.zip'),'serverReleaseVersion':contract['releaseVersion'],'serverPayloadContractSchemaVersion':contract['schemaVersion'],'serverPayloadContractSha256':contract['contractSha256'],'assetToolsFiles':asset_files},indent=2,sort_keys=True)+'\n')
PY
normalize "$WORK/manager"
MANAGER_ZIP=$OUT/zanik-manager-windows-x86_64-$VERSION.zip
(cd "$WORK/manager" && find . -type f | LC_ALL=C sort | zip -X -q "$MANAGER_ZIP" -@)
(cd "$OUT" && sha256sum zanik-*-$VERSION.* | LC_ALL=C sort > SHA256SUMS)
python3 "$ROOT/tools/release_manifest.py" --directory "$OUT" --version "$VERSION" --tag "$RELEASE_TAG" --commit "$RELEASE_COMMIT" --source-date-epoch "$SOURCE_DATE_EPOCH" --output "$OUT/release-manifest.json"
if [ -n "$RELEASE_TAG" ]; then
  python3 "$ROOT/tools/release_contract.py" --tag "$RELEASE_TAG" --notes-output "$OUT/release-notes.md"
fi
rm -rf "$WORK"
