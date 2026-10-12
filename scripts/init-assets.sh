#!/bin/sh
set -eu
SELF_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
game_root=
tool_path=
bundle=
while [ $# -gt 0 ]; do
  case "$1" in
    --game-root) [ $# -ge 2 ] || exit 2; game_root=$2; shift 2;;
    --tool-path) [ $# -ge 2 ] || exit 2; tool_path=$2; shift 2;;
    --install-bundle) [ $# -ge 2 ] || exit 2; bundle=$2; shift 2;;
    *) printf '[Zanik] Unknown asset option: %s\n' "$1" >&2; exit 2;;
  esac
done
[ -n "$game_root" ] || { printf '[Zanik] --game-root is required\n' >&2; exit 2; }
server_root=$game_root
mkdir -p "$server_root/zanik/state" "$server_root/zanik/assets"
asset_tool=$SELF_DIR/tools/zanik_assets.py
[ -f "$asset_tool" ] || asset_tool=$SELF_DIR/../tools/zanik_assets.py
python=${PYTHON:-python3}
command -v "$python" >/dev/null 2>&1 || { printf '[Zanik] Python 3 is required for asset initialization\n' >&2; exit 1; }
if [ -n "$bundle" ]; then
  exec "$python" "$asset_tool" install --bundle "$bundle" --destination "$server_root/zanik/assets"
fi
generated=$server_root/zanik-generated-web
"$python" "$asset_tool" inventory --game-root "$game_root" --generated-root "$generated" --output "$server_root/zanik/state/asset-inventory.json" || true
if [ -n "$tool_path" ]; then
  [ -x "$tool_path" ] || { printf '[Zanik] --tool-path is not executable: %s\n' "$tool_path" >&2; exit 1; }
  "$tool_path" --game-root "$game_root" --output "$generated"
fi
if [ ! -d "$generated" ]; then
  printf '[Zanik] Container inventory completed, but no generated web tree exists at %s.\n' "$generated" >&2
  printf '[Zanik] Use the separate asset tools bundle or pass an executable converter with --tool-path, then rerun this command.\n' >&2
  exit 1
fi
temporary=$server_root/zanik/state/zanik-assets-local.zip
"$python" "$asset_tool" build --game-root "$game_root" --generated-root "$generated" --output "$temporary"
exec "$python" "$asset_tool" install --bundle "$temporary" --destination "$server_root/zanik/assets"
