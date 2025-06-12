# All third-party dependencies, fetched and built from source through
# CPM.cmake.  Every version here is pinned to the exact release the previous
# vcpkg baseline (builtin-baseline 04a9d8e5) installed, so behaviour stays
# identical while dropping vcpkg entirely.
#
# Nothing in this file should ever require a system package; the only system
# pieces left are compiler/platform libraries (Threads, OpenGL) and what the
# platform backends SDL/GTK talk to (X11/Wayland on Linux, GTK3 for
# nativefiledialog).

# Global: never build a third-party dep as a shared library.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# MSVC: force the static CRT (/MT) for every dependency, like the old vcpkg
# x64-windows-static triplet did.  CMP0091 (CMAKE_MSVC_RUNTIME_LIBRARY) is
# ignored inside deps whose cmake_minimum_required predates 3.15 (SDL,
# gflags) and they would otherwise default to /MD, which can't link against
# the /MT objects we compile and would re-introduce the LNK2038 mess.
if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "" FORCE)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    string(APPEND CMAKE_C_FLAGS " /MTd")
    string(APPEND CMAKE_CXX_FLAGS " /MTd")
  else()
    string(APPEND CMAKE_C_FLAGS " /MT")
    string(APPEND CMAKE_CXX_FLAGS " /MT")
  endif()
endif()

# All the small fetch-and-assemble repos (imgui, IconFontCppHeaders,
# nativefiledialog) have no CMakeLists.txt of their own; CPM populates their
# source dirs and we build the targets ourselves below.
# (Absolutely never discover a dep that later grows a CMakeLists.)

# --- abseil (used directly by z2edit and required by protobuf) ---------------
set(ABSL_PROPAGATE_CXX_STD ON CACHE BOOL "" FORCE)
# abseil overrides CMAKE_MSVC_RUNTIME_LIBRARY to the DLL variant unless this
# option is on; keep it aligned with our global static-CRT choice.
set(ABSL_MSVC_STATIC_RUNTIME ON CACHE BOOL "" FORCE)
CPMAddPackage(
  NAME absl
  GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
  GIT_TAG 20260107.1
)

# --- gflags ------------------------------------------------------------------
set(gflags_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(gflags_BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)
set(gflags_BUILD_gflags_LIB ON CACHE BOOL "" FORCE)
set(gflags_INSTALL OFF CACHE BOOL "" FORCE)
CPMAddPackage(
  NAME gflags
  GIT_REPOSITORY https://github.com/gflags/gflags.git
  GIT_TAG v2.3.0
)
if(TARGET gflags_static AND NOT TARGET gflags::gflags)
  add_library(gflags::gflags ALIAS gflags_static)
elseif(TARGET gflags AND NOT TARGET gflags::gflags)
  add_library(gflags::gflags ALIAS gflags)
endif()

# --- zlib --------------------------------------------------------------------
# zlib uses its own ZLIB_BUILD_SHARED/ZLIB_BUILD_STATIC options (defaults: both
# ON) and ignores BUILD_SHARED_LIBS; the shared variant (output name "z") would
# make z2edit.exe require z.dll at runtime. Use the static one only.
set(ZLIB_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(ZLIB_BUILD_STATIC ON CACHE BOOL "" FORCE)
CPMAddPackage(
  NAME zlib
  GIT_REPOSITORY https://github.com/madler/zlib.git
  GIT_TAG v1.3.2
)
if(TARGET ZLIB::ZLIB)
  # provided by zlib itself
elseif(TARGET zlib)
  add_library(ZLIB::ZLIB ALIAS zlib)
elseif(TARGET zlibstatic)
  add_library(ZLIB::ZLIB ALIAS zlibstatic)
endif()

# --- SDL2 (static, in-tree; no prebuilt packages anywhere) --------------------
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
CPMAddPackage(
  NAME sdl2
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG release-2.32.10
)
if(NOT TARGET SDL2::SDL2)
  if(TARGET SDL2-static)
    add_library(SDL2::SDL2 ALIAS SDL2-static)
  elseif(TARGET SDL2)
    add_library(SDL2::SDL2 ALIAS SDL2)
  endif()
endif()
if(NOT TARGET SDL2::SDL2main AND TARGET SDL2main)
  add_library(SDL2::SDL2main ALIAS SDL2main)
endif()

# --- protobuf ----------------------------------------------------------------
set(protobuf_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_CONFORMANCE OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_PROTOBUF_BINARIES ON CACHE BOOL "" FORCE)
set(protobuf_BUILD_PROTOC_BINARIES ON CACHE BOOL "" FORCE)
set(protobuf_BUILD_LIBUPB ON CACHE BOOL "" FORCE)
set(protobuf_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(protobuf_WITH_ZLIB ON CACHE BOOL "" FORCE)
set(protobuf_INSTALL OFF CACHE BOOL "" FORCE)
# protobuf locates abseil through find_package(absl); abseil was added above
# as a subdirectory, so the config to find is the one in its build tree.
set(absl_DIR "${absl_BINARY_DIR}")
CPMAddPackage(
  NAME protobuf
  GIT_REPOSITORY https://github.com/protocolbuffers/protobuf.git
  GIT_TAG v33.4
)
if(NOT TARGET protobuf::libprotobuf)
  add_library(protobuf::libprotobuf ALIAS libprotobuf)
endif()
if(NOT TARGET protobuf::protoc)
  add_library(protobuf::protoc ALIAS protoc)
endif()

# --- imgui core --------------------------------------------------------------
CPMAddPackage(
  NAME imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.92.9b
  DOWNLOAD_ONLY YES
)
add_library(imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})
target_link_libraries(imgui PUBLIC Threads::Threads)

# The imgui repo ships a pile of example projects (with pre-built .vcxproj
# files), plus docs and its own backends; none are used here.  Drop them so
# IDE folder views (e.g. Visual Studio) don't show a hundred example projects.
# Runs on every configure; if the source was already populated this prunes the
# stale copy in place.
file(REMOVE_RECURSE
  "${imgui_SOURCE_DIR}/examples"
  "${imgui_SOURCE_DIR}/misc"
  "${imgui_SOURCE_DIR}/docs"
  "${imgui_SOURCE_DIR}/tools"
  "${imgui_SOURCE_DIR}/backends")

# --- IconFontCppHeaders ------------------------------------------------------
CPMAddPackage(
  NAME iconfonts
  GIT_REPOSITORY https://github.com/juliettef/IconFontCppHeaders.git
  GIT_TAG fda5f470b767f7b413e4a3995fa8cfe47f78b586
  DOWNLOAD_ONLY YES
)
add_library(fontawesome INTERFACE)
target_include_directories(fontawesome INTERFACE ${iconfonts_SOURCE_DIR})

# --- nativefiledialog --------------------------------------------------------
CPMAddPackage(
  NAME nfd
  GIT_REPOSITORY https://github.com/mlabbe/nativefiledialog.git
  GIT_TAG 5cfe5002eb0fac1e49777a17dec70134147931e2
  DOWNLOAD_ONLY YES
)

if(WIN32)
  # The upstream file includes <ShObjIdl.h>, which is fine on Windows' own
  # case-insensitive filesystem but breaks on case-sensitive drives ("linux"
  # style).  Materialise a fixed copy, like the Bazel genrule did.
  set(Z2EDIT_NFD_WIN "${CMAKE_BINARY_DIR}/nfd_win.cpp")
  file(READ ${nfd_SOURCE_DIR}/src/nfd_win.cpp Z2EDIT_NFD_WIN_SRC)
  string(REPLACE "ShObjIdl.h" "shobjidl.h" Z2EDIT_NFD_WIN_SRC "${Z2EDIT_NFD_WIN_SRC}")
  file(WRITE ${Z2EDIT_NFD_WIN} "${Z2EDIT_NFD_WIN_SRC}")

  add_library(nfd_impl STATIC
    ${nfd_SOURCE_DIR}/src/nfd_common.c
    ${Z2EDIT_NFD_WIN})
  target_compile_definitions(nfd_impl PRIVATE MINGW_HAS_SECURE_API=1)
  target_link_libraries(nfd_impl PUBLIC
    shell32 ole32 oleaut32 uuid comdlg32 shlwapi)
elseif(APPLE)
  add_library(nfd_impl STATIC
    ${nfd_SOURCE_DIR}/src/nfd_common.c
    ${nfd_SOURCE_DIR}/src/nfd_cocoa.m)
  target_link_libraries(nfd_impl PUBLIC "-framework Cocoa")
else()
  pkg_check_modules(GTK3 REQUIRED IMPORTED_TARGET gtk+-3.0)
  add_library(nfd_impl STATIC
    ${nfd_SOURCE_DIR}/src/nfd_common.c
    ${nfd_SOURCE_DIR}/src/nfd_gtk.c)
  target_link_libraries(nfd_impl PUBLIC PkgConfig::GTK3)
endif()
target_include_directories(nfd_impl PUBLIC
  ${nfd_SOURCE_DIR}/src ${nfd_SOURCE_DIR}/src/include)
set_target_properties(nfd_impl PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(nfd INTERFACE)
target_link_libraries(nfd INTERFACE nfd_impl)
target_include_directories(nfd INTERFACE
  ${nfd_SOURCE_DIR}/src ${nfd_SOURCE_DIR}/src/include)

add_compile_definitions(HAVE_NFD)