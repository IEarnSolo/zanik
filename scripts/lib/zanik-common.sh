#!/bin/sh

# Shared POSIX-shell helpers. Configuration is parsed explicitly and is never
# evaluated as shell code.

zanik_log(){ printf '[Zanik] %s\n' "$*" >&2; }
zanik_die(){ zanik_log "$*"; return 1; }
zanik_trim(){ printf '%s' "$1" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//'; }
zanik_bool(){ case "$1" in 1|true|TRUE|yes|YES|on|ON) printf 1;; 0|false|FALSE|no|NO|off|OFF) printf 0;; *) return 1;; esac; }
zanik_sha256(){ sha256sum "$1" | awk '{print $1}'; }
zanik_json_escape(){ printf '%s' "$1" | sed 's/\\/\\\\/g;s/"/\\"/g;s/[[:cntrl:]]/?/g'; }

zanik_defaults(){
  CFG_STATUS_PORT=${DW_STATUS_PORT:-29105}
  CFG_PUBLIC_BASE_URL=${DW_PUBLIC_BASE_URL:-http://127.0.0.1:$CFG_STATUS_PORT}
  CFG_ADMIN_COOKIE_NAME=${DW_ADMIN_COOKIE_NAME:-zanik_admin}
  CFG_FEATURE_PLAYER_STATUS=1
  CFG_FEATURE_KILL_TRACKING=1
  CFG_FEATURE_MAP=1
  CFG_FEATURE_RESOURCE_TRACKING=1
  CFG_FEATURE_NPC_TRACKING=1
  CFG_FEATURE_PLAYER_RECIPE_TRACKING=1
  CFG_FEATURE_PLAYER_QUEST_TRACKING=1
  CFG_FEATURE_ADMIN_READ=1
  CFG_FEATURE_ADMIN_RECIPE_SETTINGS=0
  CFG_FEATURE_ADMIN_TELEPORT=0
  CFG_FEATURE_ADMIN_STORAGE_MUTATION=0
  CFG_FEATURE_ADMIN_GRAVESTONE_MUTATION=0
  CFG_FEATURE_QOL_AUTO_COMPOST=0
}

zanik_set_config(){
  key=$1 value=$2
  case "$key" in
    STATUS_PORT)
      case "$value" in ''|*[!0-9]*) return 1;; esac
      [ "$value" -ge 1 ] 2>/dev/null && [ "$value" -le 65535 ] 2>/dev/null || return 1
      CFG_STATUS_PORT=$value;;
    PUBLIC_BASE_URL) case "$value" in http://*|https://*) CFG_PUBLIC_BASE_URL=$value;; *) return 1;; esac;;
    ADMIN_COOKIE_NAME) case "$value" in ''|*[!A-Za-z0-9_]*) return 1;; *) [ ${#value} -le 64 ] || return 1; CFG_ADMIN_COOKIE_NAME=$value;; esac;;
    FEATURE_PLAYER_STATUS) CFG_FEATURE_PLAYER_STATUS=$(zanik_bool "$value") || return 1;;
    FEATURE_KILL_TRACKING) CFG_FEATURE_KILL_TRACKING=$(zanik_bool "$value") || return 1;;
    FEATURE_MAP) CFG_FEATURE_MAP=$(zanik_bool "$value") || return 1;;
    FEATURE_RESOURCE_TRACKING) CFG_FEATURE_RESOURCE_TRACKING=$(zanik_bool "$value") || return 1;;
    FEATURE_NPC_TRACKING) CFG_FEATURE_NPC_TRACKING=$(zanik_bool "$value") || return 1;;
    FEATURE_PLAYER_RECIPE_TRACKING) CFG_FEATURE_PLAYER_RECIPE_TRACKING=$(zanik_bool "$value") || return 1;;
    FEATURE_PLAYER_QUEST_TRACKING) CFG_FEATURE_PLAYER_QUEST_TRACKING=$(zanik_bool "$value") || return 1;;
    FEATURE_ADMIN_READ) CFG_FEATURE_ADMIN_READ=$(zanik_bool "$value") || return 1;;
    FEATURE_ADMIN_RECIPE_SETTINGS) CFG_FEATURE_ADMIN_RECIPE_SETTINGS=$(zanik_bool "$value") || return 1;;
    FEATURE_ADMIN_TELEPORT) CFG_FEATURE_ADMIN_TELEPORT=$(zanik_bool "$value") || return 1;;
    FEATURE_ADMIN_STORAGE_MUTATION) CFG_FEATURE_ADMIN_STORAGE_MUTATION=$(zanik_bool "$value") || return 1;;
    FEATURE_ADMIN_GRAVESTONE_MUTATION) CFG_FEATURE_ADMIN_GRAVESTONE_MUTATION=$(zanik_bool "$value") || return 1;;
    FEATURE_QOL_AUTO_COMPOST) CFG_FEATURE_QOL_AUTO_COMPOST=$(zanik_bool "$value") || return 1;;
    *) return 2;;
  esac
}

zanik_load_config(){
  config_path=$1
  [ -f "$config_path" ] || return 0
  line_number=0
  while IFS= read -r raw || [ -n "$raw" ]; do
    line_number=$((line_number+1))
    raw=$(printf '%s' "$raw" | tr -d '\r')
    trimmed=$(zanik_trim "$raw")
    case "$trimmed" in ''|'#'*) continue;; esac
    case "$trimmed" in *=*) ;; *) zanik_log "Invalid configuration line $line_number in $config_path"; return 1;; esac
    key=$(zanik_trim "${trimmed%%=*}")
    value=$(zanik_trim "${trimmed#*=}")
    zanik_set_config "$key" "$value"; result=$?
    if [ "$result" -eq 2 ]; then zanik_log "Unknown configuration key '$key' at $config_path:$line_number"; return 1; fi
    if [ "$result" -ne 0 ]; then zanik_log "Invalid value for '$key' at $config_path:$line_number"; return 1; fi
  done < "$config_path"
}

zanik_export_runtime(){
  server_root=$1 install_mode=$2
  export DW_STATUS_PORT=$CFG_STATUS_PORT
  export DW_STATUS_DATA=${DW_STATUS_DATA:-$server_root/zanik-data/players.db}
  export DW_ZANIK_ROOT=${DW_ZANIK_ROOT:-$server_root/zanik}
  export DW_DATA_ROOT=${DW_DATA_ROOT:-$server_root/zanik-data}
  export DW_WEB_ROOT=${DW_WEB_ROOT:-$server_root/zanik/assets/current}
  export DW_PUBLIC_BASE_URL=$CFG_PUBLIC_BASE_URL
  export DW_ADMIN_COOKIE_NAME=$CFG_ADMIN_COOKIE_NAME
  export DW_COMPATIBILITY_FILE=${DW_COMPATIBILITY_FILE:-$server_root/zanik/compatibility.tsv}
  export DW_UPDATE_STATE=${DW_UPDATE_STATE:-$server_root/zanik/state/update-status.json}
  export DW_GAME_UPDATE_STATE=${DW_GAME_UPDATE_STATE:-$server_root/zanik/state/game-update-status.json}
  export DW_ASSET_MANIFEST=${DW_ASSET_MANIFEST:-$server_root/zanik/assets/current/asset-manifest.json}
  export DW_RESOURCE_IDENTITIES=${DW_RESOURCE_IDENTITIES:-$server_root/zanik/assets/current/resource-identities.tsv}
  export DW_NPC_IDENTITIES=${DW_NPC_IDENTITIES:-$server_root/zanik/assets/current/npc-identities.tsv}
  export ZANIK_INSTALL_MODE=$install_mode
  export ZANIK_CONFIG_VALID=1
  export ZANIK_FEATURE_PLAYER_STATUS=$CFG_FEATURE_PLAYER_STATUS
  export ZANIK_FEATURE_KILL_TRACKING=$CFG_FEATURE_KILL_TRACKING
  export ZANIK_FEATURE_MAP=$CFG_FEATURE_MAP
  export ZANIK_FEATURE_RESOURCE_TRACKING=$CFG_FEATURE_RESOURCE_TRACKING
  export ZANIK_FEATURE_NPC_TRACKING=$CFG_FEATURE_NPC_TRACKING
  export ZANIK_FEATURE_PLAYER_RECIPE_TRACKING=$CFG_FEATURE_PLAYER_RECIPE_TRACKING
  export ZANIK_FEATURE_PLAYER_QUEST_TRACKING=$CFG_FEATURE_PLAYER_QUEST_TRACKING
  export ZANIK_FEATURE_ADMIN_READ=$CFG_FEATURE_ADMIN_READ
  export ZANIK_FEATURE_ADMIN_RECIPE_SETTINGS=$CFG_FEATURE_ADMIN_RECIPE_SETTINGS
  export ZANIK_FEATURE_ADMIN_TELEPORT=$CFG_FEATURE_ADMIN_TELEPORT
  export ZANIK_FEATURE_ADMIN_STORAGE_MUTATION=$CFG_FEATURE_ADMIN_STORAGE_MUTATION
  export ZANIK_FEATURE_ADMIN_GRAVESTONE_MUTATION=$CFG_FEATURE_ADMIN_GRAVESTONE_MUTATION
  export ZANIK_FEATURE_QOL_AUTO_COMPOST=$CFG_FEATURE_QOL_AUTO_COMPOST
}

zanik_make_secret(){
  secret_file=$1
  [ -s "$secret_file" ] || { umask 077; od -An -N32 -tx1 /dev/urandom | tr -d ' \n' > "$secret_file.tmp" && mv "$secret_file.tmp" "$secret_file"; }
  chmod 600 "$secret_file"
}

zanik_game_path(){ printf '%s/RSDragonwilds/Binaries/Linux/RSDragonwildsServer-Linux-Shipping' "$1"; }
