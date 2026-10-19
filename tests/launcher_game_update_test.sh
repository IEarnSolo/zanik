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
backup=$server/zanik-backups/game-update/20261019T000000Z-01234567
mkdir -p "$backup"
printf '%s\n' '{"schemaVersion":1,"owner":"Zanik Manager","family":"game-update","operationId":"0123456789abcdef","state":"preparing"}' >"$backup/.zanik-backup.json"
printf '%s\n' "{\"schemaVersion\":1,\"operationId\":\"0123456789abcdef\",\"backup\":\"$backup\"}" >"$server/zanik/state/game-update-request.json"
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
grep -q '"installedBuild":"200"' "$server/zanik/state/game-update-result.json"
[ "$(cat "$server/RSDragonwilds/Saved/SaveGames/world.sav")" = world-data ]
[ -f "$backup/official-previous" ]
[ ! -e "$backup/official-before-update" ]
[ "$(find "$server/zanik-backups/game-update" -mindepth 1 -maxdepth 1 -type d | wc -l)" -eq 1 ]

# Every updater preflight failure must put the small launcher back at the fixed
# executable path instead of stranding the server on the one-shot script.
cp "$root/scripts/update-game.sh" "$game"; chmod +x "$game"
printf 'not-an-elf' >"$game.zanik-original"; chmod +x "$game.zanik-original"
if "$game"; then echo 'invalid original unexpectedly succeeded' >&2; exit 1; fi
grep -q 'ZANIK_COMPAT_LAUNCHER=1' "$game"
grep -q '"status":"original-invalid"' "$server/zanik/state/game-update-result.json"
echo 'launcher-aware game update: ok'
