#!/bin/sh
# ZANIK_COMPAT_LAUNCHER=1
set -eu
game_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
server_root=$(CDPATH= cd -- "$game_dir/../../.." && pwd)
export ZANIK_INSTALL_MODE=replace-executable
exec "$server_root/zanik/bin/zanik-server" --server-root "$server_root" -- "$@"
