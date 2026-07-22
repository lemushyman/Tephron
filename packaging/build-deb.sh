#!/bin/bash
# Assemble a Debian package from an already-built Tephron tree.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VERSION="${1:-1.0.0}"
ARCH="${2:-amd64}"
OUT_DIR="${OUT_DIR:-$ROOT/dist}"
STAGE="$(mktemp -d)"
trap 'rm -rf "$STAGE"' EXIT

case "$VERSION" in
    *[!0-9A-Za-z.+:~-]*|'') echo "Invalid package version: $VERSION" >&2; exit 2 ;;
esac

for required in tephron LICENSE assets/tephron.desktop assets/icon-256.png; do
    if [[ ! -e "$ROOT/$required" ]]; then
        echo "Missing build input: $required" >&2
        exit 1
    fi
done
if ! compgen -G "$ROOT/shaders/*.spv" >/dev/null; then
    echo "No compiled SPIR-V shaders found; run ./build.sh first" >&2
    exit 1
fi

PKG="$STAGE/tephron_${VERSION}_${ARCH}"
install -d "$PKG/DEBIAN" "$PKG/usr/bin" "$PKG/usr/lib/tephron/shaders"
install -d "$PKG/usr/share/applications" "$PKG/usr/share/doc/tephron"

install -m755 "$ROOT/tephron" "$PKG/usr/lib/tephron/tephron"
install -m644 "$ROOT"/shaders/*.spv "$PKG/usr/lib/tephron/shaders/"
ln -s ../lib/tephron/tephron "$PKG/usr/bin/tephron"
install -m644 "$ROOT/assets/tephron.desktop" "$PKG/usr/share/applications/tephron.desktop"

for size in 16 32 48 64 128 256 512; do
    icon="$ROOT/assets/icon-${size}.png"
    [[ -f "$icon" ]] || continue
    install -d "$PKG/usr/share/icons/hicolor/${size}x${size}/apps"
    install -m644 "$icon" "$PKG/usr/share/icons/hicolor/${size}x${size}/apps/tephron.png"
done

{
    echo 'Format: https://www.debian.org/doc/packaging-manuals/copyright-format/1.0/'
    echo 'Upstream-Name: Tephron'
    echo 'Source: https://github.com/lemushyman/Tephron'
    echo 'Files: *'
    echo 'Copyright: 2026 Nikolai Lester'
    echo 'License: BSD-3-clause'
    sed 's/^/ /' "$ROOT/LICENSE"
} > "$PKG/usr/share/doc/tephron/copyright"

INSTALLED_SIZE="$(du -sk "$PKG/usr" | awk '{print $1}')"
cat > "$PKG/DEBIAN/control" <<EOF
Package: tephron
Version: $VERSION
Section: science
Priority: optional
Architecture: $ARCH
Maintainer: Nikolai Lester <lemushyman@users.noreply.github.com>
Installed-Size: $INSTALLED_SIZE
Depends: libc6 (>= 2.31), libgcc-s1, libstdc++6 (>= 10), libsdl2-2.0-0 (>= 2.0.10), libsdl2-ttf-2.0-0, libvulkan1
Recommends: mesa-vulkan-drivers | vulkan-icd
Homepage: https://github.com/lemushyman/Tephron
Description: real-time Vulkan N-body particle simulator
 Tephron simulates gravitational, electromagnetic, and nuclear forces using
 Vulkan compute shaders. It includes black-hole lensing and accretion,
 pulsars, gravitational-wave instruments, diagnostics, and GPU particle trails.
EOF

find "$PKG" -type d -exec chmod 0755 {} +
mkdir -p "$OUT_DIR"
OUTPUT="$OUT_DIR/tephron_${VERSION}_${ARCH}.deb"
dpkg-deb --root-owner-group --build "$PKG" "$OUTPUT"
echo "$OUTPUT"
