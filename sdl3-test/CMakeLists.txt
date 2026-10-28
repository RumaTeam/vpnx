cmake_minimum_required(VERSION 3.25)

# SDL3 built from external/SDL-switch into ../prefix/switch (see ../build-sdl3.sh)
set(SDL3_PREFIX "${CMAKE_CURRENT_LIST_DIR}/../prefix/switch" CACHE PATH "SDL3 (Switch) install prefix")

project(sdl3-test VERSION 0.1.0 LANGUAGES C CXX)

add_executable(sdl3-test source/main.cpp)
target_include_directories(sdl3-test PRIVATE "${SDL3_PREFIX}/include" "$ENV{DEVKITPRO}/portlibs/switch/include")
# static libs: dependency order matters (SDL3 first, its GL/EGL deps after, libnx last)
target_link_libraries(sdl3-test PRIVATE
    "${SDL3_PREFIX}/lib/libSDL3.a" EGL GLESv2 glapi drm_nouveau nx stdc++)

nx_generate_nacp(OUTPUT sdl3-test.nacp NAME "SDL3 Test" AUTHOR "vpnx" VERSION "${PROJECT_VERSION}")
nx_create_nro(sdl3-test NACP sdl3-test.nacp)
