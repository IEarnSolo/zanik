#!/bin/sh
# ZANIK_GAME_UPDATER=1
set -eu

game_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
server_root=$(CDPATH= cd -- "$game_dir/../../.." && pwd)
zanik=$server_root/zanik
game=$server_root/RSDragonwilds/Binaries/Linux/RSDragonwildsServer-Linux-Shipping
original=$game.zanik-original
launcher_backup=$zanik/state/game-update-launcher
report=$zanik/state/game-update-result.json
request=$zanik/state/game-update-request.json
steamcmd=$server_root/steamcmd/steamcmd.sh
temporary=$report.new.$$

is_elf(){ [ -f "$1" ] && [ "$(od -An -tx1 -N4 "$1" 2>/dev/null | tr -d ' \n')" = 7f454c46 ]; }
sha(){ sha256sum "$1" | awk '{print $1}'; }
build(){ awk -F'"' '/"buildid"/{print $4;exit}' "$server_root/steamapps/appmanifest_4019830.acf" 2>/dev/null || true; }
save_fingerprint(){
  directory=$server_root/RSDragonwilds/Saved/SaveGames
  if [ ! -d "$directory" ]; then printf absent; return; fi
  find "$directory" -type f -print0 | LC_ALL=C sort -z | xargs -0 -r sha256sum | sha256sum | awk '{print $1}'
}
write_report(){
  printf '{"schemaVersion":1,"status":"%s","startedBuild":"%s","installedBuild":"%s","officialSha256":"%s","saveFingerprintBefore":"%s","saveFingerprintAfter":"%s","backup":"%s","completedAt":"%s"}\n' \
    "$1" "$2" "$3" "$4" "$5" "$6" "$backup_root" "$(date -u +%Y-%m-%dT%H:%M:%SZ)" >"$temporary"
  chmod 600 "$temporary"; mv -f "$temporary" "$report"
}
restore_launcher(){
  [ -f "$launcher_backup" ] || return 1
  grep -q 'ZANIK_COMPAT_LAUNCHER=1' "$launcher_backup" || return 1
  cp -p "$launcher_backup" "$game.restore"
  chmod 755 "$game.restore"
  mv -f "$game.restore" "$game"
}

backup_root=$(sed -n 's/.*"backup":"\([^"]*\)".*/\1/p' "$request" 2>/dev/null || true)
backup_name=${backup_root##*/}
case "$backup_root" in "$server_root"/zanik-backups/game-update/*) ;; *) exit 2;; esac
case "$backup_name" in *[!0-9TZabcdef-]*|*/*|'') exit 2;; esac
[ -d "$backup_root" ] && [ -f "$backup_root/.zanik-backup.json" ] || exit 2
mkdir -p "$zanik/state"
[ -x "$steamcmd" ] || { restore_launcher || true; write_report steamcmd-unavailable "$(build)" "$(build)" '' '' ''; exit 1; }
is_elf "$original" || { restore_launcher || true; write_report original-invalid "$(build)" "$(build)" '' '' ''; exit 1; }
grep -q 'ZANIK_COMPAT_LAUNCHER=1' "$launcher_backup" || { restore_launcher || true; write_report launcher-backup-invalid "$(build)" "$(build)" '' '' ''; exit 1; }

before_build=$(build)
before_save=$(save_fingerprint)
cp -p "$server_root/steamapps/appmanifest_4019830.acf" "$backup_root/appmanifest-before.acf" 2>/dev/null || true

# SteamCMD replaces this running script's pathname with the new official ELF.
# The shell already has the script open, so it can safely finish the transaction.
# Its delta pass can first reject the managed script at the official pathname as
# a corrupt old chunk. That marks the file for a full download, so allow one
# bounded retry inside the same journaled operation.
update_ok=false
attempt=1
while [ "$attempt" -le 2 ]; do
  printf '%s\n' "Zanik SteamCMD attempt $attempt" >>"$backup_root/steamcmd.log.full"
  if timeout 900 "$steamcmd" +force_install_dir "$server_root" +login anonymous +app_update 4019830 validate +quit >>"$backup_root/steamcmd.log.full" 2>&1; then
    update_ok=true
    break
  fi
  attempt=$((attempt + 1))
done
if [ "$update_ok" != true ]; then
  tail -c 1048576 "$backup_root/steamcmd.log.full" >"$backup_root/steamcmd.log" 2>/dev/null || true
  rm -f "$backup_root/steamcmd.log.full"
  restore_launcher || true
  write_report steamcmd-failed "$before_build" "$(build)" '' "$before_save" "$(save_fingerprint)"
  exit 1
fi
tail -c 1048576 "$backup_root/steamcmd.log.full" >"$backup_root/steamcmd.log" 2>/dev/null || true
rm -f "$backup_root/steamcmd.log.full"
if ! is_elf "$game"; then
  restore_launcher || true
  write_report updated-executable-invalid "$before_build" "$(build)" '' "$before_save" "$(save_fingerprint)"
  exit 1
fi

mv -f "$game" "$original.new"
chmod 755 "$original.new"
mv -f "$original" "$backup_root/official-previous"
mv -f "$original.new" "$original"
restore_launcher
after_save=$(save_fingerprint)
if [ "$before_save" != "$after_save" ]; then
  write_report save-tree-changed "$before_build" "$(build)" "$(sha "$original")" "$before_save" "$after_save"
  exit 1
fi
write_report complete "$before_build" "$(build)" "$(sha "$original")" "$before_save" "$after_save"
exit 0
