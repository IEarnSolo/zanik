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

"$RELEASE/scripts/install.sh" --server-root "$SERVER" --mode replace-executable --acknowledge-core-file-replacement
grep -q 'ZANIK_COMPAT_LAUNCHER=1' "$GAME"
[ -x "$GAME.zanik-original" ]
printf '#!/bin/sh\n# steam-new\nexit 0\n' > "$GAME"; chmod +x "$GAME"
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
echo 'installer lifecycle tests passed'
