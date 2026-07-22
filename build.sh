#!/bin/bash
#
# Tephron - Linux Build Script
#
# Requirements:
#   - Vulkan SDK (vulkan-devel or libvulkan-dev)
#   - SDL2 (libsdl2-dev)
#   - SDL2_ttf (libsdl2-ttf-dev)
#   - GLSL compiler (glslc from Vulkan SDK, or glslangValidator)
#   - g++ with C++17 support
#
# Install on Debian/Ubuntu:
#   sudo apt install libvulkan-dev libsdl2-dev libsdl2-ttf-dev glslang-tools
#
# Install on Arch:
#   sudo pacman -S vulkan-devel sdl2 sdl2_ttf glslang
#
# Install on Fedora:
#   sudo dnf install vulkan-devel SDL2-devel SDL2_ttf-devel glslang
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "========================================"
echo "  Tephron - Vulkan N-body Simulator"
echo "========================================"
echo ""

# Check dependencies
echo "Checking dependencies..."

check_dep() {
    if ! command -v "$1" &> /dev/null; then
        echo "  [MISSING] $1"
        return 1
    else
        echo "  [OK] $1"
        return 0
    fi
}

MISSING=0
check_dep g++ || MISSING=1
check_dep pkg-config || MISSING=1

# Find shader compiler
GLSLC=""
if command -v glslc &> /dev/null; then
    GLSLC="glslc"
    echo "  [OK] glslc (shader compiler)"
elif command -v glslangValidator &> /dev/null; then
    GLSLC="glslangValidator"
    echo "  [OK] glslangValidator (shader compiler)"
else
    echo "  [MISSING] Shader compiler (glslc or glslangValidator)"
    MISSING=1
fi

# Check libraries
if ! pkg-config --exists sdl2 2>/dev/null; then
    echo "  [MISSING] SDL2 library"
    MISSING=1
else
    echo "  [OK] SDL2"
fi

if ! pkg-config --exists vulkan 2>/dev/null; then
    # Try without pkg-config
    if [ ! -f /usr/include/vulkan/vulkan.h ]; then
        echo "  [MISSING] Vulkan SDK"
        MISSING=1
    else
        echo "  [OK] Vulkan (found headers)"
    fi
else
    echo "  [OK] Vulkan"
fi

if [ $MISSING -eq 1 ]; then
    echo ""
    echo "ERROR: Missing dependencies. Please install them first."
    echo ""
    echo "Debian/Ubuntu:"
    echo "  sudo apt install build-essential libvulkan-dev libsdl2-dev libsdl2-ttf-dev glslang-tools"
    echo ""
    echo "Arch Linux:"
    echo "  sudo pacman -S base-devel vulkan-devel sdl2 sdl2_ttf glslang"
    echo ""
    echo "Fedora:"
    echo "  sudo dnf install gcc-c++ vulkan-devel SDL2-devel SDL2_ttf-devel glslang"
    exit 1
fi

echo ""

# Compile shaders
echo "Compiling shaders..."
mkdir -p shaders

compile_shader() {
    local src="$1"
    local dst="$2"
    if [ "$GLSLC" = "glslc" ]; then
        glslc -O "$src" -o "$dst"
    else
        glslangValidator -V "$src" -o "$dst"
    fi
    echo "  $src -> $dst"
}

compile_shader shaders/physics.comp shaders/physics.comp.spv
compile_shader shaders/tilecom.comp shaders/tilecom.comp.spv
compile_shader shaders/trailupdate.comp shaders/trailupdate.comp.spv
compile_shader shaders/permute.comp shaders/permute.comp.spv
compile_shader shaders/particle.vert shaders/particle.vert.spv
compile_shader shaders/particle.frag shaders/particle.frag.spv
compile_shader shaders/text.vert shaders/text.vert.spv
compile_shader shaders/text.frag shaders/text.frag.spv
compile_shader shaders/trail.vert shaders/trail.vert.spv
compile_shader shaders/trail.frag shaders/trail.frag.spv
compile_shader shaders/composite.vert shaders/composite.vert.spv
compile_shader shaders/composite.frag shaders/composite.frag.spv

echo ""

# Get compiler flags
SDL2_CFLAGS=$(pkg-config --cflags sdl2 2>/dev/null || echo "-I/usr/include/SDL2")
SDL2_LIBS=$(pkg-config --libs sdl2 2>/dev/null || echo "-lSDL2")
VULKAN_CFLAGS=$(pkg-config --cflags vulkan 2>/dev/null || echo "")
VULKAN_LIBS=$(pkg-config --libs vulkan 2>/dev/null || echo "-lvulkan")

# Compile application
echo "Compiling application..."
echo "  g++ -std=c++17 -O3 -mtune=generic src/*.cpp ..."

g++ -std=c++17 -O3 -mtune=generic \
    -Wall -Wextra -Wno-missing-field-initializers \
    src/*.cpp \
    -o tephron \
    $SDL2_CFLAGS $SDL2_LIBS -lSDL2_ttf \
    $VULKAN_CFLAGS $VULKAN_LIBS \
    -lm -lpthread

echo ""
echo "========================================"
echo "  Build successful!"
echo "========================================"
echo ""
echo "Run with: ./tephron"
echo ""
echo "Controls:"
echo "  WASD        - Fly camera (forward/back/strafe)"
echo "  Q/E         - Fly up/down"
echo "  Arrows      - Rotate camera view"
echo "  Mouse drag  - Rotate camera"
echo "  Scroll      - Zoom in/out"
echo "  HOME        - Center camera on particles"
echo "  Space       - Pause/Resume"
echo "  R           - Reset simulation"
echo "  H           - Toggle menu"
echo "  1-0         - Load presets"
echo "  Esc         - Quit"
echo ""
