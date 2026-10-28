cmake_minimum_required(VERSION 3.25)

# Libraries from ../prefix/switch (see ../build-sdl3.sh and ../build-bgfx.sh)
set(PREFIX "${CMAKE_CURRENT_LIST_DIR}/../prefix/switch" CACHE PATH "Switch library prefix")

project(bgfx-test VERSION 0.1.0 LANGUAGES C CXX)

add_executable(bgfx-test source/main.cpp ../compat/switch_compat.c)
# bx/bgfx select their Switch code path with __NX__, which devkitA64 does not define
target_compile_options(bgfx-test PRIVATE -fPIE)  # NROs must be position independent: vtables would otherwise put relocations in read-only data
target_compile_definitions(bgfx-test PRIVATE __NX__ _GNU_SOURCE BGFX_PLATFORM_SUPPORTS_ESSL=1)
target_include_directories(bgfx-test PRIVATE "${PREFIX}/include" "${PREFIX}/include/compat/linux" "$ENV{DEVKITPRO}/portlibs/switch/include")
# static libs: dependency order matters (bgfx -> bimg -> bx, SDL3 -> GL/EGL -> libnx)
target_link_libraries(bgfx-test PRIVATE
    "${PREFIX}/lib/libbgfx.a" "${PREFIX}/lib/libbimg_decode.a" "${PREFIX}/lib/libbimg.a" "${PREFIX}/lib/libbx.a"
    "${PREFIX}/lib/libSDL3.a" EGL GLESv2 glapi drm_nouveau nx stdc++)

nx_generate_nacp(OUTPUT bgfx-test.nacp NAME "bgfx Test" AUTHOR "RumaTeam" VERSION "${PROJECT_VERSION}")
nx_create_nro(bgfx-test NACP bgfx-test.nacp)
