#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
RELEASE=$TMP/release; SERVER=$TMP/server
mkdir -p "$RELEASE/bin" "$RELEASE/lib" "$RELEASE/config" "$RELEASE/compatibility" "$RELEASE/scripts" "$RELEASE/tools" "$SERVER/RSDragonwilds/Binaries/Linux"
cp "$ROOT/dist/libdwstatus.so" "$RELEASE/bin/libzanik.so"
cp "$ROOT/dist/zanik-compat-analyzer" "$RELEASE/bin/zanik-compat-analyzer"
cp "$ROOT/scripts/zanik-server" "$RELEASE/bin/zanik-server"
cp "$ROOT/scripts/lib/zanik-common.sh" "$RELEASE/lib/zanik-common.sh"
cp "$ROOT/config/zanik.conf.example" "$RELEASE/config/zanik.conf.example"
cp "$ROOT/config/zanik-config.schema.json" "$RELEASE/config/zanik-config.schema.json"
cp "$ROOT/deploy/compatibility.tsv" "$RELEASE/compatibility/compatibility.tsv"
cp "$ROOT/tools/zanik_assets.py" "$RELEASE/tools/zanik_assets.py"
cp "$ROOT/VERSION" "$RELEASE/VERSION"
for f in install update uninstall doctor init-assets compat-launcher verify-assets check-game-update check-compatible-release update-game apply-asset-delta; do cp "$ROOT/scripts/$f.sh" "$RELEASE/scripts/$f.sh"; done
python3 "$ROOT/manager/zanik_manager/payload_contract.py" --root "$RELEASE" --release-version "$(cat "$ROOT/VERSION")" --output "$RELEASE/payload-contract.json" >/dev/null
GAME=$SERVER/RSDragonwilds/Binaries/Linux/RSDragonwildsServer-Linux-Shipping
printf '#!/bin/sh\nprintf "%%s|%%s\\n" "${LD_PRELOAD-}" "$*" > "$0.ran"\n' > "$GAME"; chmod +x "$GAME"
ORIGINAL=$(sha256sum "$GAME" | awk '{print $1}')

"$RELEASE/scripts/install.sh" --server-root "$SERVER" --mode wrapper
[ "$(sha256sum "$GAME" | awk '{print $1}')" = "$ORIGINAL" ]
grep -q '"mode":"wrapper"' "$SERVER/zanik/install-state.json"
grep -q '"payloadContractSchemaVersion":1' "$SERVER/zanik/install-state.json"
[ -f "$SERVER/zanik/config/zanik-config.schema.json" ]
[ "$(stat -c %a "$SERVER/zanik-data/console-admin.key")" = 600 ]
"$SERVER/zanik/scripts/doctor.sh" --server-root "$SERVER" --json | grep -q '"healthy":true'
"$RELEASE/scripts/install.sh" --server-root "$SERVER" --mode wrapper
BAD=$TMP/bad-release; cp -a "$RELEASE" "$BAD"; rm "$BAD/bin/libzanik.so"
if "$BAD/scripts/update.sh" --server-root "$SERVER" >/dev/null 2>&1; then exit 1; fi
[ -s "$SERVER/zanik/bin/libzanik.so" ]
grep -q '"mode":"wrapper"' "$SERVER/zanik/install-state.json"
printf 'BROKEN_KEY=secret-value\n' > "$SERVER/zanik/config/zanik.conf"
"$SERVER/zanik/bin/zanik-server" --server-root "$SERVER" -- hello 2> "$TMP/invalid.log"
grep -q '^|hello$' "$GAME.ran"
grep -q 'Unknown configuration key' "$TMP/invalid.log"
! grep -q 'secret-value' "$TMP/invalid.log"
cp "$RELEASE/config/zanik.conf.example" "$SERVER/zanik/config/zanik.conf"

# The master switch must bypass LD_PRELOAD and every Zanik runtime side effect
# while preserving the installation and official-server startup.
sed -i 's/^ZANIK_ENABLED=true$/ZANIK_ENABLED=false/' "$SERVER/zanik/config/zanik.conf"
"$SERVER/zanik/bin/zanik-server" --server-root "$SERVER" -- disabled > /dev/null 2> "$TMP/disabled.log"
grep -q '^|disabled$' "$GAME.ran"
grep -q 'disabled by configuration' "$TMP/disabled.log"
"$SERVER/zanik/scripts/doctor.sh" --server-root "$SERVER" --json | grep -q '"zanikEnabled":false'
sed -i 's/^ZANIK_ENABLED=false$/ZANIK_ENABLED=true/' "$SERVER/zanik/config/zanik.conf"

# The canonical launcher must retain Pterodactyl's console login proxy while
# forwarding all ordinary commands unchanged to the official server process.
FAKEBIN=$TMP/fake-bin; mkdir -p "$FAKEBIN"
cat > "$FAKEBIN/env" <<'EOF'
#!/bin/sh
case "${1-}" in LD_PRELOAD=*) shift;; esac
exec /usr/bin/env "$@"
EOF
cat > "$FAKEBIN/curl" <<'EOF'
#!/bin/sh
case "$*" in *console-player-ids*) printf '%s\n' 'Tester: public-character-id'; exit 0;; esac
printf '%s\n' '{"url":"http://example.invalid/admin/claim#token=test-token","expiresIn":300,"characterName":"Tester"}'
EOF
chmod +x "$FAKEBIN/env" "$FAKEBIN/curl"
printf '#!/bin/sh\nIFS= read -r line; printf "%%s\\n" "$line" > "$0.console"\n' > "$GAME"; chmod +x "$GAME"
printf 'zanik-player-id Tester\nzanik-admin public-character-id\nstatus\n' | PATH="$FAKEBIN:$PATH" "$SERVER/zanik/bin/zanik-server" --server-root "$SERVER" -- > "$TMP/proxy.out" 2>&1
grep -q 'Tester: public-character-id' "$TMP/proxy.out"
grep -q 'expires in 5 minutes' "$TMP/proxy.out"
grep -q '^status$' "$GAME.console"
! grep -q 'zanik-admin' "$GAME.console"
! grep -q 'zanik-player-id' "$GAME.console"
grep -q '"schemaVersion":1' "$SERVER/zanik/state/runtime-game.json"
grep -q '"gameBuild":"unknown"' "$SERVER/zanik/state/runtime-game.json"

mkdir -p "$SERVER/zanik/assets/current"
printf 'map-data' > "$SERVER/zanik/assets/current/world-map.jpg"
MAP_HASH=$(sha256sum "$SERVER/zanik/assets/current/world-map.jpg" | awk '{print $1}')
printf '{\n  "files": [\n    {\n      "path": "world-map.jpg",\n      "sha256": "%s",\n      "size": 8\n    }\n  ]\n}\n' "$MAP_HASH" > "$SERVER/zanik/assets/current/asset-manifest.json"
printf 'zanik-doctor-assets abc123\nstatus\n' | PATH="$FAKEBIN:$PATH" "$SERVER/zanik/bin/zanik-server" --server-root "$SERVER" -- > "$TMP/asset-proxy.out" 2>&1
REPORT=$SERVER/zanik/state/asset-verification-abc123.json
attempt=0; while [ ! -s "$REPORT" ] && [ "$attempt" -lt 50 ]; do sleep .1; attempt=$((attempt+1)); done
grep -q '"status":"valid"' "$REPORT"
grep -q '^status$' "$GAME.console"
! grep -q 'zanik-doctor-assets' "$GAME.console"

"$RELEASE/scripts/install.sh" --server-root "$SERVER" --mode replace-executable --acknowledge-core-file-replacement
grep -q 'ZANIK_COMPAT_LAUNCHER=1' "$GAME"
[ -x "$GAME.zanik-original" ]
printf '\177ELFsteam-new\n' > "$GAME"; chmod +x "$GAME"
"$RELEASE/scripts/install.sh" --server-root "$SERVER" --mode replace-executable --acknowledge-core-file-replacement
grep -q 'steam-new' "$GAME.zanik-original"
find "$SERVER/zanik-data/update-history" -name official-before-steam-update | grep -q .

"$SERVER/zanik/scripts/uninstall.sh" --server-root "$SERVER"
grep -q 'steam-new' "$GAME"
[ -d "$SERVER/zanik-data" ]
[ ! -e "$SERVER/zanik" ]
"$RELEASE/scripts/install.sh" --server-root "$SERVER" --mode wrapper
"$SERVER/zanik/scripts/uninstall.sh" --server-root "$SERVER" --purge-data
[ ! -e "$SERVER/zanik-data" ]

# Safely migrate the exact pre-0.1.0 Pterodactyl launcher without replacing
# the preserved ELF with that shell script.
LEGACY_SERVER=$TMP/legacy-server
mkdir -p "$LEGACY_SERVER/RSDragonwilds/Binaries/Linux" "$LEGACY_SERVER/zanik"
LEGACY_GAME=$LEGACY_SERVER/RSDragonwilds/Binaries/Linux/RSDragonwildsServer-Linux-Shipping
printf '\177ELFlegacy-official\n' > "$LEGACY_GAME.zanik-original"; chmod +x "$LEGACY_GAME.zanik-original"
printf '#!/bin/sh\nexec /bin/sh %s/zanik/zanik-bootstrap.sh "$@"\n' "$LEGACY_SERVER" > "$LEGACY_GAME"; chmod +x "$LEGACY_GAME"
printf '#!/bin/sh\nexit 0\n' > "$LEGACY_SERVER/zanik/zanik-bootstrap.sh"
LEGACY_SHA=$(sha256sum "$LEGACY_GAME.zanik-original" | awk '{print $1}')
"$RELEASE/scripts/install.sh" --server-root "$LEGACY_SERVER" --mode replace-executable --acknowledge-core-file-replacement
grep -q 'ZANIK_COMPAT_LAUNCHER=1' "$LEGACY_GAME"
[ "$(sha256sum "$LEGACY_GAME.zanik-original" | awk '{print $1}')" = "$LEGACY_SHA" ]
find "$LEGACY_SERVER/zanik-data/update-history" -name legacy-launcher | grep -q .
"$LEGACY_SERVER/zanik/scripts/uninstall.sh" --server-root "$LEGACY_SERVER"
[ "$(sha256sum "$LEGACY_GAME" | awk '{print $1}')" = "$LEGACY_SHA" ]

# A similar but unrecognized script must fail closed.
AMBIG=$TMP/ambiguous-server
mkdir -p "$AMBIG/RSDragonwilds/Binaries/Linux"
AMBIG_GAME=$AMBIG/RSDragonwilds/Binaries/Linux/RSDragonwildsServer-Linux-Shipping
printf '#!/bin/sh\nexec /some/other/launcher "$@"\n' > "$AMBIG_GAME"; chmod +x "$AMBIG_GAME"
printf '\177ELFreal-original\n' > "$AMBIG_GAME.zanik-original"; chmod +x "$AMBIG_GAME.zanik-original"
if "$RELEASE/scripts/install.sh" --server-root "$AMBIG" --mode replace-executable --acknowledge-core-file-replacement >/dev/null 2>&1; then exit 1; fi
grep -q '/some/other/launcher' "$AMBIG_GAME"
grep -q 'real-original' "$AMBIG_GAME.zanik-original"
echo 'installer lifecycle tests passed'
