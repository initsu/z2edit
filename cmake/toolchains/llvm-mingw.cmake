# Windows cross build (llvm-mingw from Linux, or any mingw-hosting compiler).
# Everything is fetched from source because there is no package manager for a
# foreign target here.  Set CMAKE_TOOLCHAIN_FILE to this file and add
# -DZ2EDIT_WINDOWS_CROSS=ON from the command line (see CMakePresets.json).

set(CMAKE_CROSSCOMPILING ON)
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# llvm-mingw ships driver wrappers named x86_64-w64-mingw32-gcc/g++ that add
# -target x86_64-w64-mingw32 --driver-mode=g++ and link the bundled libc++
# automatically.  Point CMake at them; it may also be given via
# CMAKE_C_COMPILER/CMAKE_CXX_COMPILER on the command line.
set(CMAKE_SYSTEM_PREFIX_PATH "${CMAKE_CURRENT_LIST_DIR}/../../toolchain_mingw"
    "${CMAKE_SYSTEM_PREFIX_PATH}")

if(NOT CMAKE_C_COMPILER)
  find_program(_Z2EDIT_MINGW_CC x86_64-w64-mingw32-gcc)
  if(NOT _Z2EDIT_MINGW_CC)
    message(FATAL_ERROR "z2edit: x86_64-w64-mingw32-gcc not found. Point CMAKE_C_COMPILER "
                        "at the llvm-mingw driver or put the toolchain bin/ on PATH.")
  endif()
  set(CMAKE_C_COMPILER "${_Z2EDIT_MINGW_CC}")
endif()
if(NOT CMAKE_CXX_COMPILER)
  find_program(_Z2EDIT_MINGW_CXX x86_64-w64-mingw32-g++)
  if(NOT _Z2EDIT_MINGW_CXX)
    message(FATAL_ERROR "z2edit: x86_64-w64-mingw32-g++ not found. Point CMAKE_CXX_COMPILER "
                        "at the llvm-mingw driver or put the toolchain bin/ on PATH.")
  endif()
  set(CMAKE_CXX_COMPILER "${_Z2EDIT_MINGW_CXX}")
endif()

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# SDK-style suffix: executables end in .exe and imports are mingw-built.
set(CMAKE_EXECUTABLE_SUFFIX .exe)
set(CMAKE_STATIC_LIBRARY_PREFIX lib)