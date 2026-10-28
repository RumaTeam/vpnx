#!/usr/bin/env bash
# Configures and builds the Switch NRO of vpinball. Needs prefix/switch from the other build-*.sh scripts.
#
#   ./build-vpinball.sh          OpenGL ES 3.2 (Mesa/nouveau)  -> build/vpinball-gl.nro
#   VARIANT=vk ./build-vpinball.sh   Vulkan (nxvk, NVK)        -> build/vpinball-vk.nro
#
# The two drivers cannot share a binary (both embed Mesa's util code), so each variant is its own build directory. The Vulkan
# one needs libbgfx-vk.a (BGFX_VARIANT=vk ./build-bgfx.sh) and libnvk.a, libnvk_support.a, libexpat.a from the nxvk build.
set -euo pipefail
cd "$(dirname "$0")"
export DEVKITPRO=${DEVKITPRO:-/opt/devkitpro}
VARIANT=${VARIANT:-gl}
if [ "$VARIANT" = "vk" ]; then VK=ON; BDIR=build/vpinball-switch-vk; else VK=OFF; BDIR=build/vpinball-switch; fi
# upstream vpinball at the commit the port was made against, plus our Switch changes
if [ ! -d vpinball ]; then
  git clone https://github.com/vpinball/vpinball.git vpinball
  git -C vpinball checkout "$(cat patches/vpinball.sha)"
  git -C vpinball switch -c switch 2>/dev/null || true
  git -C vpinball apply ../patches/vpinball-switch.patch
fi
NX_FLAGS="-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -ftls-model=local-exec -ffunction-sections -fdata-sections -D__SWITCH__"
cmake -S vpinball -B $BDIR -DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/cmake/Switch.cmake -DCMAKE_BUILD_TYPE=Release \
  -DPLATFORM=switch -DARCH=arm64 -DRENDERER=BGFX -DBUILD_APP=ON -DVPX_SWITCH_VULKAN=$VK "-DCMAKE_C_FLAGS=$NX_FLAGS" "-DCMAKE_CXX_FLAGS=$NX_FLAGS"
# the LTO link alone takes ~10 minutes
cmake --build $BDIR -j"$(nproc)" -- -k 2>&1 | tee build/vpinball-build.log | grep -E "error|Error|Built target|\.nro" || true
[ -f $BDIR/vpinball.nro ] && cp $BDIR/vpinball.nro build/vpinball-$VARIANT.nro && echo "NRO: build/vpinball-$VARIANT.nro"
