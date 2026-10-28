cmake_minimum_required(VERSION 3.25)

# Build first:  cmake -S ../pinmame -B ../build/pinmame-switch ... (see ../README.md)
set(PINMAME_ROOT "${CMAKE_CURRENT_LIST_DIR}/../pinmame" CACHE PATH "pinmame source tree (switch branch)")
set(PINMAME_BUILD "${CMAKE_CURRENT_LIST_DIR}/../build/pinmame-switch" CACHE PATH "pinmame Switch build tree")

project(pinmame-nro VERSION 0.1.0 LANGUAGES C CXX)

add_executable(pinmame-nro source/main.cpp)
target_include_directories(pinmame-nro PRIVATE "${PINMAME_ROOT}/src/libpinmame")
target_link_libraries(pinmame-nro PRIVATE "${PINMAME_BUILD}/libpinmame.a" nx)
# libpinmame.a is built with LTO (-fno-fat-lto-objects), so link with it too
target_compile_options(pinmame-nro PRIVATE -flto=auto)
target_link_options(pinmame-nro PRIVATE -flto=auto -Wl,--wrap=exit,--wrap=abort,--wrap=__assert_func)

nx_generate_nacp(OUTPUT pinmame-nro.nacp NAME "PinMAME Test" AUTHOR "vpnx" VERSION "${PROJECT_VERSION}")
nx_create_nro(pinmame-nro NACP pinmame-nro.nacp)
