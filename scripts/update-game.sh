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
steamcmd=$server_root/steamcmd/steamcmd.sh
backup_root=$server_root/zanik-backups/game-update/$(date -u +%Y%m%dT%H%M%SZ)
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

mkdir -p "$zanik/state" "$backup_root"
[ -x "$steamcmd" ] || { restore_launcher || true; write_report steamcmd-unavailable "$(build)" "$(build)" '' '' ''; exit 1; }
is_elf "$original" || { restore_launcher || true; write_report original-invalid "$(build)" "$(build)" '' '' ''; exit 1; }
grep -q 'ZANIK_COMPAT_LAUNCHER=1' "$launcher_backup" || { restore_launcher || true; write_report launcher-backup-invalid "$(build)" "$(build)" '' '' ''; exit 1; }

before_build=$(build)
before_save=$(save_fingerprint)
cp -p "$original" "$backup_root/official-before-update"
cp -p "$launcher_backup" "$backup_root/compat-launcher"
cp -p "$server_root/steamapps/appmanifest_4019830.acf" "$backup_root/appmanifest-before.acf" 2>/dev/null || true

# SteamCMD replaces this running script's pathname with the new official ELF.
# The shell already has the script open, so it can safely finish the transaction.
if ! timeout 900 "$steamcmd" +force_install_dir "$server_root" +login anonymous +app_update 4019830 +quit >"$backup_root/steamcmd.log" 2>&1; then
  restore_launcher || true
  write_report steamcmd-failed "$before_build" "$(build)" '' "$before_save" "$(save_fingerprint)"
  exit 1
fi
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
