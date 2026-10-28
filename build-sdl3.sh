#!/usr/bin/env bash
# Builds devkitPro's SDL3 Switch port (switch-sdl-3.4) as a static library into ./prefix/switch,
# then the SDL3 smoke-test NRO. Does not touch /opt/devkitpro/portlibs.
set -euo pipefail
cd "$(dirname "$0")"
export DEVKITPRO=${DEVKITPRO:-/opt/devkitpro}
TC=$DEVKITPRO/cmake/Switch.cmake

[ -d external/SDL-switch ] || git clone --depth 1 --branch switch-sdl-3.4 https://github.com/devkitPro/SDL.git external/SDL-switch

cmake -S external/SDL-switch -B build/sdl3-switch -DCMAKE_TOOLCHAIN_FILE=$TC -DCMAKE_BUILD_TYPE=Release \
  -DSDL_STATIC=ON -DSDL_SHARED=OFF -DSDL_TESTS=OFF -DSDL_TEST_LIBRARY=OFF -DSDL_EXAMPLES=OFF -DSDL_INSTALL_TESTS=OFF \
  -DCMAKE_INSTALL_PREFIX="$PWD/prefix/switch"
cmake --build build/sdl3-switch -j"$(nproc)"
cmake --install build/sdl3-switch

cmake -S sdl3-test -B build/sdl3-test -DCMAKE_TOOLCHAIN_FILE=$TC -DCMAKE_BUILD_TYPE=Release
cmake --build build/sdl3-test
echo "NRO: build/sdl3-test/sdl3-test.nro"
