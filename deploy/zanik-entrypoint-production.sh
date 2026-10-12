#!/bin/sh
set -eu

export ZANIK_STATUS_PORT=29103
exec /bin/sh /home/container/zanik/zanik-bootstrap.sh "$@"
