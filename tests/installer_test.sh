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
cp "$ROOT/deploy/compatibility.tsv" "$RELEASE/compatibility/compatibility.tsv"
cp "$ROOT/tools/zanik_assets.py" "$RELEASE/tools/zanik_assets.py"
cp "$ROOT/VERSION" "$RELEASE/VERSION"
for f in install update uninstall doctor init-assets compat-launcher; do cp "$ROOT/scripts/$f.sh" "$RELEASE/scripts/$f.sh"; done
GAME=$SERVER/RSDragonwilds/Binaries/Linux/RSDragonwildsServer-Linux-Shipping
printf '#!/bin/sh\nprintf "%%s|%%s\\n" "${LD_PRELOAD-}" "$*" > "$0.ran"\n' > "$GAME"; chmod +x "$GAME"
ORIGINAL=$(sha256sum "$GAME" | awk '{print $1}')

"$RELEASE/scripts/install.sh" --server-root "$SERVER" --mode wrapper
[ "$(sha256sum "$GAME" | awk '{print $1}')" = "$ORIGINAL" ]
grep -q '"mode":"wrapper"' "$SERVER/zanik/install-state.json"
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
printf '%s\n' '{"url":"http://example.invalid/admin/claim#token=test-token","expiresIn":300,"characterName":"Tester"}'
EOF
chmod +x "$FAKEBIN/env" "$FAKEBIN/curl"
printf '#!/bin/sh\nIFS= read -r line; printf "%%s\\n" "$line" > "$0.console"\n' > "$GAME"; chmod +x "$GAME"
printf 'zanik-admin public-character-id\nstatus\n' | PATH="$FAKEBIN:$PATH" "$SERVER/zanik/bin/zanik-server" --server-root "$SERVER" -- > "$TMP/proxy.out" 2>&1
grep -q 'expires in 5 minutes' "$TMP/proxy.out"
grep -q '^status$' "$GAME.console"
! grep -q 'zanik-admin' "$GAME.console"

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
