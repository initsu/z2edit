cmake_minimum_required(VERSION 3.21)

project(z2edit VERSION 0.1.2 LANGUAGES C CXX)

# C++23: supported by MSVC (/std:c++23, VS 2022 17.6+), GCC 11+ and clang
# 16+, which covers every toolchain this project builds with.  All
# third-party deps are compiled from source under CMAKE_CXX_STANDARD, and
# abseil picks it up via ABSL_PROPAGATE_CXX_STD (cmake/dependencies.cmake).
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(Z2EDIT_BUILD_TOOLS "Build aux tools (music players etc.)" ON)

# ---------------------------------------------------------------------------
# Build modes
# ---------------------------------------------------------------------------
# Z2EDIT_WINDOWS_CROSS: target is Windows but we are not on Windows (e.g.
# llvm-mingw from Linux).  All third-party deps are fetched from source.
if(CMAKE_CROSSCOMPILING AND CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(Z2EDIT_WINDOWS_CROSS ON)
endif()

# On Windows the only supported flavour is mingw (gcc/clang), never MSVC:
# the source tree uses POSIX headers (unistd.h, sched.h) that MSVC lacks.
if(WIN32)
  set(Z2EDIT_MINGW OFF)
  if(NOT MSVC)
    set(Z2EDIT_MINGW ON)
  endif()
endif()

# ---------------------------------------------------------------------------
# generated headers (zelda2_config.h, version.h)
# ---------------------------------------------------------------------------
set(Z2EDIT_GENDIR "${CMAKE_BINARY_DIR}/generated")

# ---------------------------------------------------------------------------
# dependencies (all fetched and built from source via CPM)
# ---------------------------------------------------------------------------
include(FindPkgConfig)
find_package(Threads REQUIRED)
# OpenGL is the only dependency left that comes from the platform (mesa on
# Linux, the GL framework on macOS); everything else lives in CPM.cmake.
if(NOT WIN32)
  find_package(OpenGL REQUIRED)
endif()

# Share the CPM git clones between the build presets.
if(NOT DEFINED CPM_SOURCE_CACHE)
  if(CMAKE_HOST_WIN32)
    set(CPM_SOURCE_CACHE "$ENV{USERPROFILE}/.cpm")
  else()
    set(CPM_SOURCE_CACHE "$ENV{HOME}/.cpm")
  endif()
endif()
include(cmake/CPM.cmake)
include(cmake/dependencies.cmake)

# ---------------------------------------------------------------------------
# global include layout: sources include "util/x.h", "proto/x.pb.h",
# "imwidget/y.h" relative to the repo root, and generated proto headers appear
# under <gendir>/proto/...
# ---------------------------------------------------------------------------
include_directories(
  ${PROJECT_SOURCE_DIR}
  ${Z2EDIT_GENDIR}
)

# ---------------------------------------------------------------------------
# common target setup
# ---------------------------------------------------------------------------
function(z2edit_setup TARGET)
  target_link_libraries(${TARGET} PUBLIC Threads::Threads)
  target_include_directories(${TARGET} PUBLIC
    ${PROJECT_SOURCE_DIR}
    ${Z2EDIT_GENDIR})
endfunction()

# static library named like the bazel target it replaces
macro(z2edit_library NAME)
  add_library(z2edit_${NAME} STATIC ${ARGN})
  z2edit_setup(z2edit_${NAME})
  add_library(z2edit::${NAME} ALIAS z2edit_${NAME})
endmacro()

# ---------------------------------------------------------------------------
# subdirectories (topological order)
# ---------------------------------------------------------------------------
add_subdirectory(proto)
add_subdirectory(util)
add_subdirectory(alg)
add_subdirectory(nes)
add_subdirectory(ips)
add_subdirectory(imwidget)
add_subdirectory(tools)
if(Z2EDIT_BUILD_TOOLS)
  add_subdirectory(music)
endif()

# ---------------------------------------------------------------------------
# version.h  (build-time, like bazel's workspace status)
# ---------------------------------------------------------------------------
set(Z2EDIT_VERSION_H "${Z2EDIT_GENDIR}/version.h")
add_custom_command(
  OUTPUT ${Z2EDIT_VERSION_H}
  COMMAND ${CMAKE_COMMAND} -DSRC=${PROJECT_SOURCE_DIR} -DOUT=${Z2EDIT_VERSION_H} -P ${PROJECT_SOURCE_DIR}/cmake/MakeVersion.cmake
  COMMENT "Generating version.h"
)
add_custom_target(version_h DEPENDS ${Z2EDIT_VERSION_H})

# ---------------------------------------------------------------------------
# zelda2_config.h  (pack_config < zelda2.textpb + content/*.textpb)
#
# Native builds regenerate it at build time.  Cross builds (Windows target,
# Linux host) cannot execute a Windows pack_config, so they fall back to a
# committed generated/zelda2_config.h when present.
# ---------------------------------------------------------------------------
set(Z2EDIT_ZELDA2_H "${Z2EDIT_GENDIR}/zelda2_config.h")
if(Z2EDIT_WINDOWS_CROSS)
  if(EXISTS "${PROJECT_SOURCE_DIR}/generated/zelda2_config.h")
    configure_file("${PROJECT_SOURCE_DIR}/generated/zelda2_config.h"
                   "${Z2EDIT_ZELDA2_H}" COPYONLY)
    message(STATUS "z2edit: using committed generated/zelda2_config.h for cross build")
  else()
    message(WARNING
      "z2edit: generated/zelda2_config.h is missing; cross build will embed an "
      "empty kZelda2Cfg. Generate it on a native build and commit it.")
    file(WRITE "${Z2EDIT_ZELDA2_H}" "const unsigned char kZelda2Cfg[] = {0};\n")
  endif()
else()
  set(Z2EDIT_CONTENT_FILES)
  file(GLOB Z2EDIT_CONTENT_FILES ${PROJECT_SOURCE_DIR}/content/*.textpb)
  add_custom_command(
    OUTPUT ${Z2EDIT_ZELDA2_H}
    COMMAND ${CMAKE_COMMAND}
      -DPACK_CONFIG=$<TARGET_FILE:pack_config>
      -DCONFIG=${PROJECT_SOURCE_DIR}/zelda2.textpb
      -DSYMBOL=kZelda2Cfg
      -DOUT=${Z2EDIT_ZELDA2_H}
      -P ${PROJECT_SOURCE_DIR}/cmake/run_pack_config.cmake
    DEPENDS pack_config ${PROJECT_SOURCE_DIR}/zelda2.textpb ${Z2EDIT_CONTENT_FILES}
    COMMENT "Generating zelda2_config.h"
  )
  add_custom_target(zelda2_config_h DEPENDS ${Z2EDIT_ZELDA2_H})
endif()

# ---------------------------------------------------------------------------
# app + z2edit
# ---------------------------------------------------------------------------
add_library(z2edit_app STATIC app.cc app.h)
z2edit_setup(z2edit_app)
set_source_files_properties(app.cc APPEND PROPERTY OBJECT_DEPENDS ${Z2EDIT_VERSION_H})
target_link_libraries(z2edit_app PUBLIC
  z2edit::base
  z2edit::drops
  z2edit::editor
  z2edit::enemyattr
  z2edit::hwpalette
  z2edit::item_effects
  z2edit::neschrview
  z2edit::object_table
  z2edit::overworld_tiles
  z2edit::palace_gfx
  z2edit::palette
  z2edit::project
  z2edit::rom_memory
  z2edit::simplemap
  z2edit::start_values
  z2edit::text_table
  z2edit::tile_transform
  z2edit::xptable
  z2edit::cartridge
  z2edit::chr_util
  z2edit::cpu6502
  z2edit::mappers
  z2edit::text_encoding
  z2edit::proto_rominfo
  z2edit::browser
  z2edit::fpsmgr
  z2edit::imgui_sdl_opengl
  z2edit::os
  z2edit::logging
  gflags::gflags
  SDL3::SDL3
  nfd
)
# SDL3main only exists / is needed on Windows (console main elsewhere).
if(WIN32 AND TARGET SDL3::SDL3main)
  target_link_libraries(z2edit_app PUBLIC SDL3::SDL3main)
endif()
if(TARGET imgui)
  target_link_libraries(z2edit_app PUBLIC imgui)
endif()

add_executable(z2edit main.cc)
z2edit_setup(z2edit)
set_source_files_properties(main.cc APPEND PROPERTY OBJECT_DEPENDS ${Z2EDIT_ZELDA2_H})
target_link_libraries(z2edit PRIVATE
  z2edit_app
  z2edit::config
  gflags::gflags)

# platform link libraries
if(WIN32)
  if(Z2EDIT_MINGW)
    target_link_libraries(z2edit PRIVATE
      opengl32 dinput8 dxguid dxerr8 user32 gdi32 winmm imm32 ole32
      oleaut32 shell32 version uuid setupapi advapi32 hid ws2_32 m)
  else()
    target_link_libraries(z2edit PRIVATE
      opengl32 user32 gdi32 winmm imm32 ole32 oleaut32 shell32 version
      uuid setupapi advapi32 ws2_32)
  endif()
elseif(APPLE)
  target_link_libraries(z2edit PRIVATE OpenGL::GL "-framework Cocoa")
else()
  target_link_libraries(z2edit PRIVATE OpenGL::GL m)
endif()

# ---------------------------------------------------------------------------
# extra binaries (mirror of the bazel cc_binary targets)
# ---------------------------------------------------------------------------
add_executable(ipspatch ips/ipspatch.cc)
z2edit_setup(ipspatch)
target_link_libraries(ipspatch PRIVATE z2edit::ips z2edit::file z2edit::status gflags::gflags)
if(Z2EDIT_MINGW)
  target_link_libraries(ipspatch PRIVATE m)
endif()