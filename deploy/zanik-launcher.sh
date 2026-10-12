#!/bin/sh
set -eu

export DW_STATUS_PORT=29105
mkdir -p /home/container/zanik-data
export DW_STATUS_DATA=/home/container/zanik-data/players.db
export LD_PRELOAD=/home/container/libdwstatus.so

exec /home/container/RSDragonwilds/Binaries/Linux/RSDragonwildsServer-Linux-Shipping.zanik-original "$@"
