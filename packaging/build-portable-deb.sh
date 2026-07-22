#!/bin/bash
# Build on Debian 11's older userspace, then assemble an amd64 .deb. The
# resulting ABI baseline works across current Debian, Ubuntu and Linux Mint.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VERSION="${1:-1.0.0}"
IMAGE="${TEPHRON_BUILD_IMAGE:-debian:11-slim}"
mkdir -p "$ROOT/dist"

docker run --rm \
    -e VERSION="$VERSION" -e HOST_UID="$(id -u)" -e HOST_GID="$(id -g)" \
    -v "$ROOT:/source:ro" -v "$ROOT/dist:/output" \
    "$IMAGE" bash -euxo pipefail -c '
        export DEBIAN_FRONTEND=noninteractive
        apt-get update
        apt-get install -y --no-install-recommends \
            build-essential pkg-config libvulkan-dev libsdl2-dev \
            libsdl2-ttf-dev glslang-tools ca-certificates
        cp -a /source /tmp/tephron
        cd /tmp/tephron
        ./build.sh
        OUT_DIR=/output ./packaging/build-deb.sh "$VERSION" amd64
        chown "$HOST_UID:$HOST_GID" "/output/tephron_${VERSION}_amd64.deb"
    '

echo "$ROOT/dist/tephron_${VERSION}_amd64.deb"
