#!/usr/bin/env bash
# Builds the bgfx stack (bx, bimg, bimg_decode, bgfx) for the Switch as static libs into ./prefix/switch,
# using the same sources vpinball pins (bgfx.cmake release + vbousquet's bgfx fork), GLES 3.2 renderer.
# bx already has a BX_PLATFORM_NX path (POSIX, no dlopen, EGL/GLES + Vulkan VI surface); devkitA64 just
# doesn't define __NX__, so we add it, plus two small bx patches (patches/bgfx-switch.patch).
set -euo pipefail
cd "$(dirname "$0")"
BUILD_TYPE=${BUILD_TYPE:-Release}
source vpinball/platforms/config.sh >/dev/null
export DEVKITPRO=${DEVKITPRO:-/opt/devkitpro}
TC=$DEVKITPRO/cmake/Switch.cmake
PREFIX=$PWD/prefix/switch
# BGFX_VARIANT=gles (default): OpenGL ES 3.2 through Mesa/nouveau, installs libbgfx.a
# BGFX_VARIANT=vk: Vulkan through the statically linked nxvk driver, installs libbgfx-vk.a. The two drivers cannot be linked
# into the same binary (both embed Mesa's util code), so each variant is its own bgfx build and its own NRO.
VARIANT=${BGFX_VARIANT:-gles}
# BGFX_PROFILER=1 turns on the BGFX_PROFILER_SCOPE regions (reported by the app on its [scopes] line); a little overhead
PROFILER_FLAGS=""; [ "${BGFX_PROFILER:-0}" = "1" ] && PROFILER_FLAGS="-DBGFX_CONFIG_PROFILER=1"

if [ ! -d external/bgfx/bgfx.cmake ]; then
  mkdir -p external/bgfx && cd external/bgfx
  curl -sL https://github.com/bkaradzic/bgfx.cmake/releases/download/v${BGFX_CMAKE_VERSION}/bgfx.cmake.v${BGFX_CMAKE_VERSION}.tar.gz | tar xz
  curl -sL https://github.com/vbousquet/bgfx/archive/${BGFX_PATCH_SHA}.tar.gz | tar xz
  rm -rf bgfx.cmake/bgfx && mv bgfx-${BGFX_PATCH_SHA} bgfx.cmake/bgfx
  (cd bgfx.cmake && patch -p1 < ../../../patches/bgfx-switch.patch && patch -p1 < ../../../patches/bgfx-vulkan-nx.patch)
  cd ../..
fi

# IMPORTANT: passing -DCMAKE_C(XX)_FLAGS on the command line REPLACES the toolchain's flags. The Switch
# needs -mtp=soft (libnx software thread pointer) and -ftls-model=local-exec: without them any
# thread_local access (e.g. bgfx's s_threadIndex) reads the wrong register and faults. So repeat them.
NX_FLAGS="-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -ftls-model=local-exec -ffunction-sections -fdata-sections -D__SWITCH__"
# BGFX_DEBUG=1 enables bgfx's own traces/asserts (forwarded through the app's CallbackI); off by default.
DEBUG_DEFS=""; [ "${BGFX_DEBUG:-0}" = "1" ] && DEBUG_DEFS="-DBX_CONFIG_DEBUG=1"
# RENDERERS: bgfx only auto-detects backends when NONE of BGFX_CONFIG_RENDERER_* is defined; defining any
# (e.g. "=0" to switch one off) disables auto-detect and everything else silently becomes 0, leaving only
# the Noop renderer. So name the ones we want: GLES 3.2 and Vulkan (the statically linked nxvk driver, see
# patches/bgfx-vulkan-nx.patch).
# BGFX_PLATFORM_SUPPORTS_ESSL=1: for NX bgfx only embeds NVN/SPIR-V shaders, so without this the GLES
# renderer finds no embedded (debug font etc.) shaders. Anything else that embeds bgfx shaders (vpinball)
# must define it too.
# -D_GNU_SOURCE exposes fseeko/ftello in newlib.
if [ "$VARIANT" = "vk" ]; then
  RENDERER_FLAGS="-DBGFX_CONFIG_RENDERER_VULKAN=1"; BDIR=build/bgfx-switch-vk
else
  RENDERER_FLAGS="-DBGFX_CONFIG_RENDERER_OPENGLES=32 -DBGFX_PLATFORM_SUPPORTS_ESSL=1"; BDIR=build/bgfx-switch
fi
cmake -S external/bgfx/bgfx.cmake -B $BDIR -DCMAKE_TOOLCHAIN_FILE=$TC -DCMAKE_BUILD_TYPE=Release \
  -DBGFX_BUILD_EXAMPLES=OFF -DBGFX_BUILD_TOOLS=OFF -DBGFX_CONFIG_MULTITHREADED=ON \
  -DBGFX_CONFIG_MAX_FRAME_BUFFERS=256 -DBGFX_OPENGLES_VERSION=32 -DBGFX_LIBRARY_TYPE=STATIC \
  "-DCMAKE_C_FLAGS=$NX_FLAGS -D__NX__ -D_GNU_SOURCE $DEBUG_DEFS" \
  "-DCMAKE_CXX_FLAGS=$NX_FLAGS -D__NX__ -D_GNU_SOURCE $DEBUG_DEFS $PROFILER_FLAGS $RENDERER_FLAGS"
# bimg_encode (NVTT) does not support this platform and vpinball does not link it
cmake --build $BDIR -j"$(nproc)" --target bx bimg bimg_decode bgfx

mkdir -p "$PREFIX/lib" "$PREFIX/include"
if [ "$VARIANT" = "vk" ]; then
  cp $BDIR/cmake/bgfx/libbgfx.a "$PREFIX/lib/libbgfx-vk.a"
  # bx and bimg do not depend on the renderer: install them too, so a Vulkan-only setup needs no GL run of this script
  for l in bx/libbx.a bimg/libbimg.a bimg/libbimg_decode.a; do [ -f "$PREFIX/lib/$(basename $l)" ] || cp $BDIR/cmake/$l "$PREFIX/lib/"; done
else
  cp $BDIR/cmake/bx/libbx.a $BDIR/cmake/bimg/libbimg.a $BDIR/cmake/bimg/libbimg_decode.a $BDIR/cmake/bgfx/libbgfx.a "$PREFIX/lib/"
fi
B=external/bgfx/bgfx.cmake
cp -r $B/bgfx/include/bgfx $B/bimg/include/bimg $B/bx/include/bx "$PREFIX/include/"
[ -d $B/bx/include/compat ] && cp -r $B/bx/include/compat "$PREFIX/include/" || true
echo "bgfx ($VARIANT) libs in $PREFIX/lib"
