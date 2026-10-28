#!/usr/bin/env bash
# Builds vpinball's non-graphics dependencies for the Switch as static libs into ./prefix/switch:
#   libwinevbs (VBScript engine), FreeImage (image loading), libzip.
# Same sources vpinball pins in platforms/config.sh. (PinMAME: build.sh, SDL3: build-sdl3.sh, bgfx: build-bgfx.sh)
set -euo pipefail
cd "$(dirname "$0")"
BUILD_TYPE=${BUILD_TYPE:-Release}
source vpinball/platforms/config.sh >/dev/null
export DEVKITPRO=${DEVKITPRO:-/opt/devkitpro}
TC=$DEVKITPRO/cmake/Switch.cmake
PREFIX=$PWD/prefix/switch
# NEVER replace the toolchain flags: passing -DCMAKE_C(XX)_FLAGS on the command line drops -mtp=soft etc.
# Where extra flags are needed, repeat these too.
NX_FLAGS="-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -ftls-model=local-exec -ffunction-sections -fdata-sections -D__SWITCH__"
COMMON=(-DCMAKE_TOOLCHAIN_FILE=$TC -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_INSTALL_PREFIX="$PREFIX")

fetch() { # name url-template-sha dir
  mkdir -p "external/$1" && (cd "external/$1" && [ -n "$(ls -d $2-* 2>/dev/null)" ] || curl -sL "$3" | tar xz)
}

# --- libwinevbs: needs patches/libwinevbs-switch.patch (newlib ferror/feof macros need FILE*)
fetch winevbs libwinevbs "https://github.com/vpinball/libwinevbs/archive/${LIBWINEVBS_SHA}.tar.gz"
W=$(ls -d external/winevbs/libwinevbs-*/ | head -1)
grep -q "(FILE\*)hFile" "$W/src/wine.c" || (cd "$W" && patch -p1 < ../../../patches/libwinevbs-switch.patch)
cmake -S "$W" -B build/winevbs-switch "${COMMON[@]}" -DPLATFORM=switch -DARCH=arm64 -DBUILD_SHARED=OFF -DBUILD_STATIC=ON
cmake --build build/winevbs-switch -j"$(nproc)"; cmake --install build/winevbs-switch

# --- FreeImage: LibRaw needs htons/ntohs (arpa/inet.h) and swab (_GNU_SOURCE)
fetch freeimage freeimage "https://github.com/toxieainc/freeimage/archive/${FREEIMAGE_SHA}.tar.gz"
FI=$(ls -d external/freeimage/freeimage-*/ | head -1)
cmake -S "$FI" -B build/freeimage-switch "${COMMON[@]}" -DPLATFORM=switch -DARCH=arm64 -DBUILD_STATIC=ON \
  "-DCMAKE_C_FLAGS=$NX_FLAGS -D_GNU_SOURCE" "-DCMAKE_CXX_FLAGS=$NX_FLAGS -D_GNU_SOURCE -include arpa/inet.h"
cmake --build build/freeimage-switch -j"$(nproc)"
# the install rule names "source/FreeImage.h" (wrong case) and fails after copying the library;
# vpinball ships its own standalone/FreeImage.h, so the header is not needed
cmake --install build/freeimage-switch || echo "(FreeImage header install skipped: library installed)"

# --- libzip: options as vpinball, static; zlib comes from devkitPro portlibs
fetch libzip libzip "https://github.com/nih-at/libzip/archive/${LIBZIP_SHA}.tar.gz"
LZ=$(ls -d external/libzip/libzip-*/ | head -1)
cmake -S "$LZ" -B build/libzip-switch "${COMMON[@]}" -DBUILD_SHARED_LIBS=OFF -DENABLE_ZSTD=OFF -DENABLE_BZIP2=OFF \
  -DENABLE_LZMA=OFF -DENABLE_OPENSSL=OFF -DENABLE_GNUTLS=OFF -DENABLE_MBEDTLS=OFF -DENABLE_COMMONCRYPTO=OFF \
  -DENABLE_WINDOWS_CRYPTO=OFF -DBUILD_TOOLS=OFF -DBUILD_REGRESS=OFF -DBUILD_OSSFUZZ=OFF -DBUILD_EXAMPLES=OFF -DBUILD_DOC=OFF
cmake --build build/libzip-switch -j"$(nproc)"; cmake --install build/libzip-switch
echo "deps installed in $PREFIX"
