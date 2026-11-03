#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
server=$tmp/server
game=$server/RSDragonwilds/Binaries/Linux/RSDragonwildsServer-Linux-Shipping
mkdir -p "$(dirname "$game")" "$server/zanik/state" "$server/steamcmd" "$server/steamapps" "$server/RSDragonwilds/Saved/SaveGames"
cp "$root/scripts/update-game.sh" "$game"; chmod +x "$game"
printf '\177ELFold' >"$game.zanik-original"; chmod +x "$game.zanik-original"
cp "$root/scripts/compat-launcher.sh" "$server/zanik/state/game-update-launcher"; chmod +x "$server/zanik/state/game-update-launcher"
launcher_sha=$(sha256sum "$server/zanik/state/game-update-launcher" | awk '{print $1}')
operation=0123456789abcdef0123456789abcdef
backup=$server/zanik-backups/game-update/20261019T000000Z-01234567
mkdir -p "$backup"
write_request(){
  expected=$1
  printf '%s\n' "{\"schemaVersion\":2,\"operationId\":\"$operation\",\"backup\":\"$backup\",\"expectedBuild\":\"$expected\",\"launcherSha256\":\"$launcher_sha\"}" >"$server/zanik/state/game-update-request.json"
}
printf '%s\n' "{\"schemaVersion\":1,\"owner\":\"Zanik Manager\",\"family\":\"game-update\",\"operationId\":\"$operation\",\"state\":\"preparing\"}" >"$backup/.zanik-backup.json"
write_request 200
printf 'world-data' >"$server/RSDragonwilds/Saved/SaveGames/world.sav"
printf '"buildid" "100"\n' >"$server/steamapps/appmanifest_4019830.acf"
cat >"$server/steamcmd/steamcmd.sh" <<EOF
#!/bin/sh
printf '%s\n' "\$*" >'$server/steamcmd.args'
printf '\177ELFnew' >'$game'
chmod +x '$game'
printf '"buildid" "200"\n' >'$server/steamapps/appmanifest_4019830.acf'
EOF
chmod +x "$server/steamcmd/steamcmd.sh"
"$game"
grep -q 'app_update 4019830 validate' "$server/steamcmd.args"
grep -q 'ZANIK_COMPAT_LAUNCHER=1' "$game"
[ "$(od -An -tx1 -N4 "$game.zanik-original" | tr -d ' \n')" = 7f454c46 ]
grep -q '"status":"complete"' "$server/zanik/state/game-update-result.json"
grep -q "\"operationId\":\"$operation\"" "$server/zanik/state/game-update-result.json"
grep -q "\"launcherSha256\":\"$launcher_sha\"" "$server/zanik/state/game-update-result.json"
grep -q '"installedBuild":"200"' "$server/zanik/state/game-update-result.json"
[ "$(cat "$server/RSDragonwilds/Saved/SaveGames/world.sav")" = world-data ]
[ -f "$backup/official-previous" ]
[ ! -e "$backup/official-before-update" ]
[ "$(find "$server/zanik-backups/game-update" -mindepth 1 -maxdepth 1 -type d | wc -l)" -eq 1 ]

# Every updater preflight failure must put the small launcher back at the fixed
# executable path instead of stranding the server on the one-shot script.
cp "$root/scripts/update-game.sh" "$game"; chmod +x "$game"
printf 'not-an-elf' >"$game.zanik-original"; chmod +x "$game.zanik-original"
write_request 200
if "$game"; then echo 'invalid original unexpectedly succeeded' >&2; exit 1; fi
grep -q 'ZANIK_COMPAT_LAUNCHER=1' "$game"
grep -q '"status":"original-invalid"' "$server/zanik/state/game-update-result.json"

# A Steam failure restores the exact launcher and leaves the original and save untouched.
cp "$root/scripts/update-game.sh" "$game"; chmod +x "$game"
printf '\177ELFold-again' >"$game.zanik-original"; chmod +x "$game.zanik-original"
cat >"$server/steamcmd/steamcmd.sh" <<'EOF'
#!/bin/sh
exit 1
EOF
chmod +x "$server/steamcmd/steamcmd.sh"
write_request 300
if "$game"; then echo 'failed Steam update unexpectedly succeeded' >&2; exit 1; fi
grep -q 'ZANIK_COMPAT_LAUNCHER=1' "$game"
[ "$(sha256sum "$game" | awk '{print $1}')" = "$launcher_sha" ]
grep -q '"status":"steamcmd-failed"' "$server/zanik/state/game-update-result.json"
[ "$(cat "$server/RSDragonwilds/Saved/SaveGames/world.sav")" = world-data ]

# Save drift is detected after a valid download and leaves the server stopped with recovery evidence.
cp "$root/scripts/update-game.sh" "$game"; chmod +x "$game"
cat >"$server/steamcmd/steamcmd.sh" <<EOF
#!/bin/sh
printf '\177ELFsave-drift' >'$game'
chmod +x '$game'
printf '"buildid" "300"\n' >'$server/steamapps/appmanifest_4019830.acf'
printf 'changed-world-data' >'$server/RSDragonwilds/Saved/SaveGames/world.sav'
EOF
chmod +x "$server/steamcmd/steamcmd.sh"
write_request 300
if "$game"; then echo 'save-drift update unexpectedly succeeded' >&2; exit 1; fi
grep -q 'ZANIK_COMPAT_LAUNCHER=1' "$game"
grep -q '"status":"save-tree-changed"' "$server/zanik/state/game-update-result.json"
grep -q '"saveFingerprintBefore":"[^"]*","saveFingerprintAfter":"[^"]*"' "$server/zanik/state/game-update-result.json"

# An unexpected build is never accepted as the requested transaction and its ELF is retained for review.
printf 'world-data' >"$server/RSDragonwilds/Saved/SaveGames/world.sav"
cp "$root/scripts/update-game.sh" "$game"; chmod +x "$game"
cat >"$server/steamcmd/steamcmd.sh" <<EOF
#!/bin/sh
printf '\177ELFunexpected' >'$game'
chmod +x '$game'
printf '"buildid" "401"\n' >'$server/steamapps/appmanifest_4019830.acf'
EOF
chmod +x "$server/steamcmd/steamcmd.sh"
write_request 400
if "$game"; then echo 'unexpected build update unexpectedly succeeded' >&2; exit 1; fi
grep -q 'ZANIK_COMPAT_LAUNCHER=1' "$game"
grep -q '"status":"unexpected-build"' "$server/zanik/state/game-update-result.json"
[ -f "$backup/unexpected-official" ]

# The modern end-user updater must never contain or invoke compatibility research tooling.
! grep -q 'compat-analyzer\|compatibility.new' "$root/scripts/update-game.sh"
echo 'launcher-aware game update: ok'
