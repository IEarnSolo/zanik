#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
mkdir -p "$TMP/server/zanik/scripts" "$TMP/server/zanik/lib" "$TMP/server/steamapps" "$TMP/server/steamcmd"
cp "$ROOT/scripts/check-game-update.sh" "$TMP/server/zanik/scripts/"
cp "$ROOT/scripts/lib/zanik-common.sh" "$TMP/server/zanik/lib/"
printf '"AppState"\n{\n  "buildid" "12345"\n}\n' > "$TMP/server/steamapps/appmanifest_4019830.acf"
cat > "$TMP/server/steamcmd/steamcmd.sh" <<'EOF'
#!/bin/sh
printf '%s\n' "$@" > "$(dirname "$0")/arguments"
cat <<'OUTPUT'
"branches"
{
  "public"
  {
    "buildid" "12345"
  }
}
OUTPUT
EOF
chmod +x "$TMP/server/steamcmd/steamcmd.sh"
"$TMP/server/zanik/scripts/check-game-update.sh" --server-root "$TMP/server" --request-id 0123456789abcdef
grep -q '"status":"current"' "$TMP/server/zanik/state/game-update-status.json"
grep -q 'app_info_print' "$TMP/server/steamcmd/arguments"
! grep -q 'app_update' "$TMP/server/steamcmd/arguments"
sed -i 's/"12345"/"99999"/' "$TMP/server/steamcmd/steamcmd.sh"
"$TMP/server/zanik/scripts/check-game-update.sh" --server-root "$TMP/server" --request-id fedcba9876543210
grep -q '"status":"update-available"' "$TMP/server/zanik/state/game-update-status.json"
echo 'server game update tests passed'
