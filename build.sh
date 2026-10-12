#!/bin/sh
set -eu
cd "$(dirname "$0")"
# Dockerfile.build constructs only the reusable compiler environment. Project
# files are provided by the bind mount below, so .dockerignore excludes the
# repository from `docker build`. If Dockerfile.build ever gains a COPY or ADD
# instruction, update .dockerignore with the required allowlist exceptions.
docker build -f Dockerfile.build -t dw-status-builder .
docker run --rm -v "$(pwd):/source" dw-status-builder
