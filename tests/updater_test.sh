#!/bin/sh
set -eu
root=$(cd "$(dirname "$0")/.." && pwd)
test_root=$(mktemp -d)
trap 'rm -rf "$test_root"' EXIT
mkdir -p "$test_root/zanik/state" "$test_root/zanik-data" \
  "$test_root/RSDragonwilds/Binaries/Linux" "$test_root/steamapps" "$test_root/steamcmd"
cp "$root/deploy/zanik-bootstrap.sh" "$test_root/zanik/"
cp "$root/deploy/compatibility.tsv" "$test_root/zanik/"
cp "$root/dist/libdwstatus.so" "$test_root/libdwstatus.so"
cp /bin/true "$test_root/RSDragonwilds/Binaries/Linux/RSDragonwildsServer-Linux-Shipping.zanik-original"
cat >"$test_root/steamapps/appmanifest_4019830.acf" <<EOF
"AppState"
{
  "buildid" "25465077"
}
EOF
cat >"$test_root/steamcmd/steamcmd.sh" <<'EOF'
#!/bin/sh
printf '%s\n' '"public"' '"buildid" "25465077"'
EOF
chmod +x "$test_root/steamcmd/steamcmd.sh"
ZANIK_ROOT="$test_root" "$test_root/zanik/zanik-bootstrap.sh"
grep -q '"lastResult":"no-update"' "$test_root/zanik/state/update-status.json"
if find "$test_root/zanik-data/update-history" -type f | grep -q .; then
  echo "unchanged restart created an update report" >&2
  exit 1
fi

cat >"$test_root/steamcmd/steamcmd.sh" <<'EOF'
#!/bin/sh
exit 1
EOF
chmod +x "$test_root/steamcmd/steamcmd.sh"
ZANIK_ROOT="$test_root" "$test_root/zanik/zanik-bootstrap.sh"
grep -q '"lastResult":"check-failed-started-installed"' \
  "$test_root/zanik/state/update-status.json"

# A changed build that cannot pass live validation must restore the prior files.
rm -rf "$test_root"
test_root=$(mktemp -d)
mkdir -p "$test_root/zanik/state" "$test_root/zanik-data" \
  "$test_root/RSDragonwilds/Binaries/Linux" "$test_root/steamapps" "$test_root/steamcmd"
cp "$root/deploy/zanik-bootstrap.sh" "$test_root/zanik/"
cp "$root/deploy/zanik-entrypoint.sh" "$test_root/zanik/launcher.sh"
cp "$root/deploy/compatibility.tsv" "$test_root/zanik/"
cp "$root/dist/libdwstatus.so" "$test_root/libdwstatus.so"
cp /bin/true "$test_root/RSDragonwilds/Binaries/Linux/RSDragonwildsServer-Linux-Shipping.zanik-original"
printf old-symbols >"$test_root/RSDragonwilds/Binaries/Linux/RSDragonwildsServer-Linux-Shipping.sym"
printf database >"$test_root/zanik-data/players.db"
cat >"$test_root/steamapps/appmanifest_4019830.acf" <<EOF
"AppState"
{
  "buildid" "1"
}
EOF
cat >"$test_root/zanik/zanik-compat-analyzer" <<'EOF'
#!/bin/sh
printf '%s\t%s\t%s\t%s\t%s\n' "$(sha256sum "$1" | awk '{print $1}')" 0x10 0x20 0x30 0x40
EOF
cat >"$test_root/steamcmd/steamcmd.sh" <<'EOF'
#!/bin/sh
case " $* " in
  *" +app_update "*)
    cp /bin/false "$FAKE_ROOT/RSDragonwilds/Binaries/Linux/RSDragonwildsServer-Linux-Shipping"
    printf new-symbols >"$FAKE_ROOT/RSDragonwilds/Binaries/Linux/RSDragonwildsServer-Linux-Shipping.sym"
    printf '%s\n' '"AppState"' '{' '"buildid" "2"' '}' >"$FAKE_ROOT/steamapps/appmanifest_4019830.acf" ;;
  *) printf '%s\n' '"public"' '"buildid" "2"' ;;
esac
EOF
chmod +x "$test_root/steamcmd/steamcmd.sh" "$test_root/zanik/zanik-compat-analyzer"
FAKE_ROOT="$test_root" ZANIK_ROOT="$test_root" ZANIK_VALIDATE_ATTEMPTS=1 \
  ZANIK_STATUS_PORT=39105 "$test_root/zanik/zanik-bootstrap.sh"
grep -q '"lastResult":"rolled-back-after-validation"' \
  "$test_root/zanik/state/update-status.json"
test "$("$test_root/RSDragonwilds/Binaries/Linux/RSDragonwildsServer-Linux-Shipping.zanik-original"; echo $?)" = 0
test "$(find "$test_root/zanik-data/update-history" -name '*.md' | wc -l)" = 1
test "$(find "$test_root/zanik-data/update-history" -name '*.json' | wc -l)" = 1
