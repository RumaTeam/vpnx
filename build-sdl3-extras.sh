#!/usr/bin/env bash
# Builds SDL3_image (stb backend: JPEG and PNG) and SDL3_ttf (devkitPro freetype portlib) as static libs into ./prefix/switch.
# Requires build-sdl3.sh to have been run first.
set -euo pipefail
cd "$(dirname "$0")"
BUILD_TYPE=${BUILD_TYPE:-Release}
source vpinball/platforms/config.sh >/dev/null
export DEVKITPRO=${DEVKITPRO:-/opt/devkitpro}
TC=$DEVKITPRO/cmake/Switch.cmake
PREFIX=$PWD/prefix/switch
NX_FLAGS="-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -ftls-model=local-exec -ffunction-sections -fdata-sections -D__SWITCH__"
COMMON=(-DCMAKE_TOOLCHAIN_FILE=$TC -DCMAKE_BUILD_TYPE=Release -DCMAKE_POSITION_INDEPENDENT_CODE=ON
  -DCMAKE_INSTALL_PREFIX="$PREFIX" -DCMAKE_PREFIX_PATH="$PREFIX;$DEVKITPRO/portlibs/switch" -DBUILD_SHARED_LIBS=OFF)

fetch() { # name url
  mkdir -p "external/$1" && (cd "external/$1" && [ -n "$(ls -d $1-* 2>/dev/null)" ] || curl -sL "$2" | tar xz)
}

fetch SDL_image "https://github.com/libsdl-org/SDL_image/archive/${SDL_IMAGE_SHA}.tar.gz"
I=$(ls -d external/SDL_image/SDL_image-*/ | head -1)
cmake -S "$I" -B build/sdl3-image-switch "${COMMON[@]}" -DSDL3_DIR="$PREFIX/lib/cmake/SDL3" \
  -DSDLIMAGE_SAMPLES=OFF -DSDLIMAGE_TESTS=OFF -DSDLIMAGE_VENDORED=OFF -DSDLIMAGE_DEPS_SHARED=OFF \
  -DSDLIMAGE_AVIF=OFF -DSDLIMAGE_WEBP=OFF -DSDLIMAGE_JXL=OFF -DSDLIMAGE_TIF=OFF -DSDLIMAGE_PNG=ON -DSDLIMAGE_PNG_LIBPNG=OFF -DSDLIMAGE_PNG_SAVE=OFF -DSDLIMAGE_JPG=ON -DSDLIMAGE_JPG_SAVE=OFF \
  "-DCMAKE_C_FLAGS=$NX_FLAGS"
cmake --build build/sdl3-image-switch -j"$(nproc)"; cmake --install build/sdl3-image-switch

fetch SDL_ttf "https://github.com/libsdl-org/SDL_ttf/archive/${SDL_TTF_SHA}.tar.gz"
T=$(ls -d external/SDL_ttf/SDL_ttf-*/ | head -1)
cmake -S "$T" -B build/sdl3-ttf-switch "${COMMON[@]}" -DSDL3_DIR="$PREFIX/lib/cmake/SDL3" \
  -DSDLTTF_SAMPLES=OFF -DSDLTTF_VENDORED=OFF -DSDLTTF_HARFBUZZ=OFF -DSDLTTF_PLUTOSVG=OFF \
  "-DCMAKE_C_FLAGS=$NX_FLAGS" "-DCMAKE_CXX_FLAGS=$NX_FLAGS"
cmake --build build/sdl3-ttf-switch -j"$(nproc)"; cmake --install build/sdl3-ttf-switch
