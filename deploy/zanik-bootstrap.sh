#!/bin/sh
set -u

ROOT=${ZANIK_ROOT:-/home/container}
ZANIK="$ROOT/zanik"
STATE="$ZANIK/state"
DATA="$ROOT/zanik-data"
HISTORY="$DATA/update-history"
GAME_DIR="$ROOT/RSDragonwilds/Binaries/Linux"
GAME="$GAME_DIR/RSDragonwildsServer-Linux-Shipping.zanik-original"
MANIFEST="$ROOT/steamapps/appmanifest_4019830.acf"
COMPAT="$ZANIK/compatibility.tsv"
STATUS="$STATE/update-status.json"
OPS="$STATE/operations.log"
APP_ID=4019830
STATUS_PORT=${ZANIK_STATUS_PORT:-29105}
VALIDATE_ATTEMPTS=${ZANIK_VALIDATE_ATTEMPTS:-60}
mkdir -p "$STATE" "$HISTORY"
chmod 755 "$ZANIK/zanik-compat-analyzer" "$ZANIK/launcher.sh" 2>/dev/null || true

log(){ printf '%s %s\n' "$(date -u +%FT%TZ)" "$*" >>"$OPS"; }
build_id(){ awk -F'"' '/"buildid"/{print $4;exit}' "$1" 2>/dev/null; }
json_status(){
  printf '{"installedBuild":"%s","remoteBuild":"%s","executableSha256":"%s","lastResult":"%s","lastUpdate":"%s","backupPath":"%s","validation":"%s"}' \
    "$1" "$2" "$3" "$4" "$5" "$6" "$7" >"$STATUS.tmp" && mv "$STATUS.tmp" "$STATUS"
}
prepare_env(){
  export DW_STATUS_PORT="$STATUS_PORT" DW_STATUS_DATA="$DATA/players.db"
  export DW_COMPATIBILITY_FILE="$COMPAT" DW_UPDATE_STATE="$STATUS"
  export DW_PUBLIC_BASE_URL="${DW_PUBLIC_BASE_URL:-http://game.mcgalaxy.com:$STATUS_PORT}"
  if [ -z "${DW_ADMIN_COOKIE_NAME:-}" ]; then
    if [ "$STATUS_PORT" = "29105" ]; then DW_ADMIN_COOKIE_NAME=zanik_admin_dev; else DW_ADMIN_COOKIE_NAME=zanik_admin; fi
  fi
  export DW_ADMIN_COOKIE_NAME
  secret_file="$DATA/console-admin.key"
  if [ ! -s "$secret_file" ]; then
    umask 077
    od -An -N32 -tx1 /dev/urandom | tr -d ' \n' >"$secret_file.tmp" && mv "$secret_file.tmp" "$secret_file"
  fi
  DW_CONSOLE_SECRET=$(cat "$secret_file" 2>/dev/null || true); export DW_CONSOLE_SECRET
}
run_game(){ env LD_PRELOAD="$ROOT/libdwstatus.so" "$GAME" "$@"; }
start_game(){
  prepare_env
  fifo="$STATE/console-input.$$"
  rm -f "$fifo"; mkfifo "$fifo" || exec env LD_PRELOAD="$ROOT/libdwstatus.so" "$GAME" "$@"
  exec 3<>"$fifo"
  run_game "$@" <&3 & child=$!
  trap 'kill "$child" 2>/dev/null || true; wait "$child" 2>/dev/null || true; rm -f "$fifo"' INT TERM EXIT
  while kill -0 "$child" 2>/dev/null && IFS= read -r command; do
      case "$command" in
        zanik-player-id|zanik-player-id\ *)
          requested=${command#zanik-player-id}; requested=${requested# }
          response=$(curl -fsS --max-time 5 -X POST -H "X-Zanik-Console-Key: $DW_CONSOLE_SECRET" --data-binary "$requested" "http://127.0.0.1:$STATUS_PORT/api/admin/console-player-ids" 2>/dev/null || true)
          if [ -n "$response" ]; then printf '%s\n' "$response" | while IFS= read -r line; do printf '[Zanik] %s\n' "$line"; done; else printf '[Zanik] Could not read known player IDs.\n'; fi
          ;;
        zanik-admin|zanik-admin\ *)
          requested=${command#zanik-admin}; requested=${requested# }
          response=$(curl -fsS --max-time 5 -X POST -H "X-Zanik-Console-Key: $DW_CONSOLE_SECRET" --data-binary "$requested" "http://127.0.0.1:$STATUS_PORT/api/admin/console-claim" 2>/dev/null || true)
          url=$(printf '%s' "$response" | sed -n 's/.*"url":"\([^"]*\)".*/\1/p')
          if [ -n "$url" ]; then printf '[Zanik] Admin login for %s (expires in 5 minutes): %s\n' "${requested:-the authorized character}" "$url"; else printf '[Zanik] Could not create admin login link: %s\n' "$response"; fi
          ;;
        *) printf '%s\n' "$command" >&3 ;;
      esac
  done
  wait "$child"; result=$?
  trap - INT TERM EXIT; rm -f "$fifo"; exit "$result"
}

installed=$(build_id "$MANIFEST")
remote_log="$STATE/steam-app-info.tmp"
remote=""
if timeout 90 "$ROOT/steamcmd/steamcmd.sh" +login anonymous +app_info_update 1 +app_info_print "$APP_ID" +quit >"$remote_log" 2>&1; then
  remote=$(awk '/"public"/{p=1;next} p&&/"buildid"/{gsub(/[^0-9]/,"");print;exit}' "$remote_log")
fi
sha=$(sha256sum "$GAME" 2>/dev/null | awk '{print $1}')
old_size=$(stat -c %s "$GAME" 2>/dev/null || printf 0)
if [ -z "$remote" ]; then
  log "Steam build query failed; starting installed build $installed"
  json_status "$installed" "" "$sha" "check-failed-started-installed" "" "" "not-run"
  start_game "$@"
fi
if [ "$remote" = "$installed" ]; then
  log "No update: build $installed"
  json_status "$installed" "$remote" "$sha" "no-update" "" "" "not-required"
  start_game "$@"
fi

stamp=$(date -u +%Y%m%dT%H%M%SZ)
backup="$HISTORY/${stamp}-${installed:-unknown}-to-$remote"
mkdir -p "$backup"
for source in "$DATA/players.db" "$DATA/player-recipe-snapshots.db" "$DATA/player-quest-snapshots.db" "$DATA/admin-sessions.db" "$DATA/admin-audit.log" "$DATA/console-admin.key" "$DATA/recipe-unlock-settings.db" "$DATA/grave-respawn-capture.bin" "$DATA/spud-state-index-capture.bin" "$ZANIK/admins.txt" "$ROOT/libdwstatus.so" "$GAME" "$GAME_DIR/RSDragonwildsServer-Linux-Shipping.sym" "$MANIFEST" "$COMPAT" "$ZANIK/launcher.sh" "$ZANIK/zanik-bootstrap.sh" "$ZANIK/zanik-compat-analyzer" "$ZANIK/resource-identities.tsv" "$ZANIK/npc-identities.tsv" "$ZANIK/npc-exclusions.tsv"; do
  [ ! -f "$source" ] || cp -p "$source" "$backup/$(basename "$source")"
done
[ ! -d "$ZANIK/web" ] || cp -a "$ZANIK/web" "$backup/web"
log "Update detected: ${installed:-unknown} -> $remote; backup $backup"
# SteamCMD applies binary deltas against the official executable path. Restore
# the preserved game binary there temporarily; patching the small Zanik
# launcher as if it were the game executable is rejected as corrupt content.
official="$GAME_DIR/RSDragonwildsServer-Linux-Shipping"
cp -p "$GAME" "$official" && chmod 755 "$official"
if ! "$ROOT/steamcmd/steamcmd.sh" +force_install_dir "$ROOT" +login anonymous +app_update "$APP_ID" +quit >>"$OPS" 2>&1; then
  cp -p "$ZANIK/launcher.sh" "$official"; chmod 755 "$official"
  json_status "$installed" "$remote" "$sha" "update-download-failed" "$stamp" "$backup" "failed"
  log "Steam update failed; starting previous build"
  start_game "$@"
fi

new_build=$(build_id "$MANIFEST")
if [ ! -s "$official" ] || [ ! -s "$GAME_DIR/RSDragonwildsServer-Linux-Shipping.sym" ]; then
  json_status "$installed" "$remote" "$sha" "update-files-missing" "$stamp" "$backup" "failed"
  log "Updated executable or symbols missing; restoring previous build"
  cp -p "$backup/$(basename "$GAME")" "$GAME"
  cp -p "$ZANIK/launcher.sh" "$official"; chmod 755 "$official"
  start_game "$@"
fi
mv -f "$official" "$GAME"
chmod 755 "$GAME"
cp -p "$ZANIK/launcher.sh" "$official"; chmod 755 "$official"
new_sha=$(sha256sum "$GAME" | awk '{print $1}')
new_size=$(stat -c %s "$GAME" 2>/dev/null || printf 0)
# Preserve the exact downloaded analysis pair outside Steam-managed paths.
# Rollback may restore the previous live files, but it must never destroy the
# evidence required to produce a compatibility entry for the new build.
analysis_dir="$HISTORY/analysis-$new_build-$new_sha"
mkdir -p "$analysis_dir"
cp -p "$GAME" "$analysis_dir/RSDragonwildsServer-Linux-Shipping.zanik-original"
cp -p "$GAME_DIR/RSDragonwildsServer-Linux-Shipping.sym" "$analysis_dir/RSDragonwildsServer-Linux-Shipping.sym"
analysis=failed
entry="$STATE/compatibility.new"
if "$ZANIK/zanik-compat-analyzer" "$GAME" "$GAME_DIR/RSDragonwildsServer-Linux-Shipping.sym" >"$entry" 2>"$STATE/compatibility-error.txt"; then
  grep -v "^$new_sha[[:space:]]" "$COMPAT" >"$COMPAT.tmp" 2>/dev/null || true
  cat "$entry" >>"$COMPAT.tmp" && mv "$COMPAT.tmp" "$COMPAT"
  analysis=passed
fi

report="$HISTORY/${stamp}-${new_build}.md"
cat >"$report" <<EOF
# Dragonwilds update $new_build

- Detected: $stamp
- Previous build: ${installed:-unknown}
- New build: $new_build
- Previous executable SHA-256: $sha
- New executable SHA-256: $new_sha
- Previous executable size: $old_size bytes
- New executable size: $new_size bytes
- Backup: $backup
- Compatibility analysis: $analysis
- Applied: official executable preserved as .zanik-original; Zanik launcher restored; compatibility manifest evaluated.
- Validation: pending startup checks
EOF
if [ -s "$entry" ]; then
  printf '\n## Generated compatibility entry\n\n```text\n' >>"$report"
  cat "$entry" >>"$report"
  printf '```\n' >>"$report"
fi
printf '{"detected":"%s","previousBuild":"%s","newBuild":"%s","previousSha256":"%s","newSha256":"%s","previousSize":%s,"newSize":%s,"backupPath":"%s","compatibility":"%s","validation":"pending"}' \
  "$stamp" "$installed" "$new_build" "$sha" "$new_sha" "$old_size" "$new_size" "$backup" "$analysis" >"$HISTORY/${stamp}-${new_build}.json"
json_status "$new_build" "$remote" "$new_sha" "updated" "$stamp" "$backup" "pending"
log "Update applied: build $new_build compatibility $analysis"
prepare_env
run_game "$@" & child=$!
trap 'kill "$child" 2>/dev/null || true; wait "$child" 2>/dev/null || true' INT TERM EXIT
validated=false
i=0
while [ "$i" -lt "$VALIDATE_ATTEMPTS" ]; do
  admin_id=$(awk '{sub(/#.*/,"");gsub(/^[ \t]+|[ \t]+$/,"");if(length){print;exit}}' "$ZANIK/admins.txt" 2>/dev/null)
  claim_response=$(curl -fsS --max-time 3 -X POST -H "X-Zanik-Console-Key: $DW_CONSOLE_SECRET" --data-binary "$admin_id" "http://127.0.0.1:$STATUS_PORT/api/admin/console-claim" 2>/dev/null || true)
  claim_token=$(printf '%s' "$claim_response" | sed -n 's/.*#token=\([0-9a-f][0-9a-f]*\)".*/\1/p')
  admin_cookie="$STATE/admin-validation-cookie.$$"
  rm -f "$admin_cookie"; umask 077
  claim_exchange=$(curl -fsS --max-time 3 -c "$admin_cookie" -X POST -H 'Content-Type: application/json' --data-binary "{\"token\":\"$claim_token\"}" "http://127.0.0.1:$STATUS_PORT/api/admin/claim" 2>/dev/null || true)
  admin_csrf=$(printf '%s' "$claim_exchange" | sed -n 's/.*"csrf":"\([^"]*\)".*/\1/p')
  session_status=$(curl -sS -o /dev/null -w '%{http_code}' --max-time 3 -b "$admin_cookie" "http://127.0.0.1:$STATUS_PORT/api/admin/session" 2>/dev/null || true)
  unlock_catalog=$(curl -fsS --max-time 8 -b "$admin_cookie" "http://127.0.0.1:$STATUS_PORT/api/admin/storage/catalog" 2>/dev/null || true)
  building_catalog=$(curl -fsS --max-time 12 -b "$admin_cookie" "http://127.0.0.1:$STATUS_PORT/api/admin/storage/recipes" 2>/dev/null || true)
  unlock_catalog_validation=failed
  if printf '%s' "$unlock_catalog" | grep -q 'A String of Sinew' && printf '%s' "$unlock_catalog" | grep -q 'MOUNT: Sunkissed Clawfoot'; then
    unlock_catalog_validation=passed
  fi
  building_catalog_validation=failed
  if printf '%s' "$building_catalog" | grep -q '"buildingTabs"' &&
     printf '%s' "$building_catalog" | grep -q '"expected":827' &&
     printf '%s' "$building_catalog" | grep -q '"references":844' &&
     printf '%s' "$building_catalog" | grep -q '"label":"Companions"'; then
    building_catalog_validation=passed
  fi
  csrf_status=$(curl -sS -o /dev/null -w '%{http_code}' --max-time 3 -b "$admin_cookie" -X POST -H 'Content-Type: application/json' --data-binary '{"csrf":"invalid"}' "http://127.0.0.1:$STATUS_PORT/api/admin/logout" 2>/dev/null || true)
  logout_status=$(curl -sS -o /dev/null -w '%{http_code}' --max-time 3 -b "$admin_cookie" -X POST -H 'Content-Type: application/json' --data-binary "{\"csrf\":\"$admin_csrf\"}" "http://127.0.0.1:$STATUS_PORT/api/admin/logout" 2>/dev/null || true)
  logged_out_status=$(curl -sS -o /dev/null -w '%{http_code}' --max-time 3 -b "$admin_cookie" "http://127.0.0.1:$STATUS_PORT/api/admin/session" 2>/dev/null || true)
  rm -f "$admin_cookie"
  unauth_status=$(curl -sS -o /dev/null -w '%{http_code}' --max-time 3 "http://127.0.0.1:$STATUS_PORT/api/admin/storage/catalog" 2>/dev/null || true)
  admin_session_validation=failed
  if [ ${#claim_token} -eq 64 ] && [ -n "$admin_csrf" ] && [ "$session_status" = 200 ] && [ "$csrf_status" = 403 ] && [ "$logout_status" = 200 ] && [ "$logged_out_status" = 401 ] && [ "$unauth_status" = 401 ] && [ "$unlock_catalog_validation" = passed ] && [ "$building_catalog_validation" = passed ]; then
    admin_session_validation=passed
  fi
  if body=$(curl -fsS --max-time 3 "http://127.0.0.1:$STATUS_PORT/api/update-status" 2>/dev/null) &&
     printf '%s' "$body" | grep -q '"website":true' &&
     printf '%s' "$body" | grep -q '"playerDataCompatibility":true' &&
     curl -fsS --max-time 3 "http://127.0.0.1:$STATUS_PORT/" >/dev/null 2>&1 &&
     curl -fsS --max-time 3 "http://127.0.0.1:$STATUS_PORT/api/players" >/dev/null 2>&1 &&
     curl -fsS --max-time 3 "http://127.0.0.1:$STATUS_PORT/api/known-players" >/dev/null 2>&1; then
    validated=true; break
  fi
  sleep 3; i=$((i+1))
done
if [ "$validated" = true ]; then
  optional="kill=$(printf '%s' "$body"|sed -n 's/.*\"killHookCompatibility\":\([-0-9]*\).*/\1/p') region=$(printf '%s' "$body"|sed -n 's/.*\"regionCompatibility\":\([-0-9]*\).*/\1/p') map=$(printf '%s' "$body"|sed -n 's/.*\"mapProjectionCompatibility\":\(true\|false\).*/\1/p') lodestones=$(printf '%s' "$body"|sed -n 's/.*\"lodestoneCompatibility\":\([-0-9]*\).*/\1/p') nodestones=$(printf '%s' "$body"|sed -n 's/.*\"nodestoneCompatibility\":\([-0-9]*\).*/\1/p') resources=$(printf '%s' "$body"|sed -n 's/.*\"resourceCompatibility\":\([-0-9]*\).*/\1/p') adminAuth=$(printf '%s' "$body"|sed -n 's/.*\"adminAuthorizationStorage\":\(true\|false\).*/\1/p') adminConsole=$(printf '%s' "$body"|sed -n 's/.*\"adminConsoleCompatibility\":\([-0-9]*\).*/\1/p') adminTeleport=$(printf '%s' "$body"|sed -n 's/.*\"adminTeleportCompatibility\":\([-0-9]*\).*/\1/p') adminStorage=$(printf '%s' "$body"|sed -n 's/.*\"adminStorageCompatibility\":\([-0-9]*\).*/\1/p') adminGraves=$(printf '%s' "$body"|sed -n 's/.*\"adminGravestoneCompatibility\":\([-0-9]*\).*/\1/p') onlinePlayers=$(printf '%s' "$body"|sed -n 's/.*\"onlinePlayers\":\([0-9]*\).*/\1/p') spawnedPawns=$(printf '%s' "$body"|sed -n 's/.*\"spawnedPawns\":\([0-9]*\).*/\1/p') worldLocations=$(printf '%s' "$body"|sed -n 's/.*\"worldLocations\":\([0-9]*\).*/\1/p')"
  admin_detail="adminSessionFlow=$admin_session_validation adminUnlockCatalog=$unlock_catalog_validation adminGround=$(printf '%s' "$body"|sed -n 's/.*\"adminTeleportGroundTraceCompatibility\":\([-0-9]*\).*/\1/p') adminStorageTeleport=$(printf '%s' "$body"|sed -n 's/.*\"adminStorageTeleportCompatibility\":\([-0-9]*\).*/\1/p') adminDispatcher=$(printf '%s' "$body"|sed -n 's/.*\"adminStorageDispatcherCompatibility\":\([-0-9]*\).*/\1/p') adminBatch=$(printf '%s' "$body"|sed -n 's/.*\"adminStorageBatchCompatibility\":\([-0-9]*\).*/\1/p') adminRename=$(printf '%s' "$body"|sed -n 's/.*\"adminStorageRenameCompatibility\":\([-0-9]*\).*/\1/p') adminRemoval=$(printf '%s' "$body"|sed -n 's/.*\"adminStorageRemovalCompatibility\":\([-0-9]*\).*/\1/p') adminUnlockCompatibility=$(printf '%s' "$body"|sed -n 's/.*\"adminStorageUnlockCatalogCompatibility\":\([-0-9]*\).*/\1/p') graveRegistry=$(printf '%s' "$body"|sed -n 's/.*\"adminGravestoneRegistryCompatibility\":\([-0-9]*\).*/\1/p') graveStreaming=$(printf '%s' "$body"|sed -n 's/.*\"adminGravestoneStreamingCompatibility\":\([-0-9]*\).*/\1/p') gravePersistence=$(printf '%s' "$body"|sed -n 's/.*\"adminGravestonePersistenceCompatibility\":\([-0-9]*\).*/\1/p')"
  admin_detail="$admin_detail adminBuildingCatalog=$building_catalog_validation buildingTaxonomy=$(printf '%s' "$body"|sed -n 's/.*\"adminBuildingTaxonomyCompatibility\":\([-0-9]*\).*/\1/p') buildingRequirements=$(printf '%s' "$body"|sed -n 's/.*\"adminBuildingRequirementsCompatibility\":\([-0-9]*\).*/\1/p') buildingIcons=$(printf '%s' "$body"|sed -n 's/.*\"adminBuildingIconsCompatibility\":\([-0-9]*\).*/\1/p') craftingRecipes=$(printf '%s' "$body"|sed -n 's/.*\"adminCraftingRecipeCompatibility\":\([-0-9]*\).*/\1/p') buildingResolved=$(printf '%s' "$body"|sed -n 's/.*\"adminBuildingResolvedPieces\":\([0-9]*\).*/\1/p') buildingUnavailable=$(printf '%s' "$body"|sed -n 's/.*\"adminBuildingUnavailablePieces\":\([0-9]*\).*/\1/p')"
  player_recipe_detail="playerRecipes=$(printf '%s' "$body"|sed -n 's/.*\"playerRecipeCompatibility\":\([-0-9]*\).*/\1/p') playerRecipeCatalog=$(printf '%s' "$body"|sed -n 's/.*\"playerRecipeCatalogCompatibility\":\([-0-9]*\).*/\1/p') playerRecipeLayout=$(printf '%s' "$body"|sed -n 's/.*\"playerRecipeProgressLayoutCompatibility\":\([-0-9]*\).*/\1/p') playerRecipeQuery=$(printf '%s' "$body"|sed -n 's/.*\"playerRecipeQueryCompatibility\":\([-0-9]*\).*/\1/p') playerRecipeSnapshots=$(printf '%s' "$body"|sed -n 's/.*\"playerRecipeSnapshots\":\([0-9]*\).*/\1/p')"
  optional="$optional $admin_detail $player_recipe_detail"
  npc_detail="npcCatalog=$(printf '%s' "$body"|sed -n 's/.*\"npcCatalogCompatibility\":\([-0-9]*\).*/\1/p') npcRegistry=$(printf '%s' "$body"|sed -n 's/.*\"npcRegistryCompatibility\":\([-0-9]*\).*/\1/p') npcIdentity=$(printf '%s' "$body"|sed -n 's/.*\"npcIdentityCompatibility\":\([-0-9]*\).*/\1/p') npcTransform=$(printf '%s' "$body"|sed -n 's/.*\"npcTransformCompatibility\":\([-0-9]*\).*/\1/p') npcLifecycle=$(printf '%s' "$body"|sed -n 's/.*\"npcLifecycleCompatibility\":\([-0-9]*\).*/\1/p') npcRespawn=$(printf '%s' "$body"|sed -n 's/.*\"npcRespawnCompatibility\":\([-0-9]*\).*/\1/p') npcDungeon=$(printf '%s' "$body"|sed -n 's/.*\"npcDungeonExclusionCompatibility\":\([-0-9]*\).*/\1/p') npcScanWorkUs=$(printf '%s' "$body"|sed -n 's/.*\"npcLastGenerationWorkMicroseconds\":\([0-9]*\).*/\1/p') npcFinalizeUs=$(printf '%s' "$body"|sed -n 's/.*\"npcLastFinalizationMicroseconds\":\([0-9]*\).*/\1/p') npcDutyBp=$(printf '%s' "$body"|sed -n 's/.*\"npcLastDutyCycleBasisPoints\":\([0-9]*\).*/\1/p')"
  optional="$optional $npc_detail"
  json_status "$new_build" "$remote" "$new_sha" "updated-validated" "$stamp" "$backup" "passed: $optional"
  sed -i 's/- Validation: pending startup checks/- Validation: passed/' "$report"
  printf '\n- Feature compatibility: %s\n' "$optional" >>"$report"
  printf '{"detected":"%s","previousBuild":"%s","newBuild":"%s","previousSha256":"%s","newSha256":"%s","previousSize":%s,"newSize":%s,"backupPath":"%s","compatibility":"%s","validation":"passed","featureCompatibility":"%s"}' \
    "$stamp" "$installed" "$new_build" "$sha" "$new_sha" "$old_size" "$new_size" "$backup" "$analysis" "$optional" >"$HISTORY/${stamp}-${new_build}.json"
  log "Updated build $new_build passed core startup validation; $optional"
  trap - INT TERM EXIT
  wait "$child"; exit $?
fi

log "Updated build $new_build failed live validation; rolling back"
kill "$child" 2>/dev/null || true; wait "$child" 2>/dev/null || true
trap - INT TERM EXIT
cp -p "$backup/RSDragonwildsServer-Linux-Shipping.zanik-original" "$GAME"
cp -p "$backup/RSDragonwildsServer-Linux-Shipping.sym" "$GAME_DIR/RSDragonwildsServer-Linux-Shipping.sym"
cp -p "$backup/appmanifest_4019830.acf" "$MANIFEST"
cp -p "$backup/libdwstatus.so" "$ROOT/libdwstatus.so"
cp -p "$backup/compatibility.tsv" "$COMPAT"
cp -p "$ZANIK/launcher.sh" "$official"; chmod 755 "$official" "$GAME"
json_status "$installed" "$remote" "$sha" "rolled-back-after-validation" "$stamp" "$backup" "failed"
sed -i 's/- Validation: pending startup checks/- Validation: failed; previous build restored/' "$report"
printf '{"detected":"%s","previousBuild":"%s","newBuild":"%s","previousSha256":"%s","newSha256":"%s","previousSize":%s,"newSize":%s,"backupPath":"%s","compatibility":"%s","validation":"failed-rolled-back"}' \
  "$stamp" "$installed" "$new_build" "$sha" "$new_sha" "$old_size" "$new_size" "$backup" "$analysis" >"$HISTORY/${stamp}-${new_build}.json"
start_game "$@"
