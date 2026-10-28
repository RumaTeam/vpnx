#!/usr/bin/env bash
# Builds libpinmame (static, devkitA64) and the test NRO. Requires devkitPro with switch-dev.
set -euo pipefail
cd "$(dirname "$0")"
export DEVKITPRO=${DEVKITPRO:-/opt/devkitpro}
TC=$DEVKITPRO/cmake/Switch.cmake

# upstream PinMAME at the commit the Switch patch was made against (patches/pinmame.sha)
if [ ! -d pinmame ]; then
  git init -q pinmame
  git -C pinmame remote add origin https://github.com/vpinball/pinmame.git
  git -C pinmame fetch -q --depth 1 origin "$(cat patches/pinmame.sha)"
  git -C pinmame checkout -q FETCH_HEAD
fi
if [ ! -f pinmame/CMakeLists.txt ]; then
  (cd pinmame && git apply ../patches/pinmame-switch.patch)
fi
# romsets also directly in <pinmame base> (zips next to the table), not only in <base>/roms; applied only once
(cd pinmame && git apply --check ../patches/pinmame-romspath.patch 2>/dev/null && git apply ../patches/pinmame-romspath.patch) || true

# PLATFORM=switch selects the non-Windows source set; asmjit (needs JIT memory) and
# Pinball 2000 (C++17/MediaGX, niche) are off for the first bring-up.
cmake -S pinmame -B build/pinmame-switch -DCMAKE_TOOLCHAIN_FILE=$TC -DCMAKE_BUILD_TYPE=Release \
  -DPLATFORM=switch -DARCH=arm64 -DBUILD_SHARED=OFF -DBUILD_STATIC=ON \
  -DPINMAME_JIT_ASMJIT=OFF -DPINMAME_P2K=OFF
cmake --build build/pinmame-switch -j"$(nproc)"

cmake -S pinmame-nro -B build/pinmame-nro -DCMAKE_TOOLCHAIN_FILE=$TC -DCMAKE_BUILD_TYPE=Release
cmake --build build/pinmame-nro
echo "NRO: build/pinmame-nro/pinmame-nro.nro"
