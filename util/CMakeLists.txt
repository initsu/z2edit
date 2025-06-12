# util library set (mirrors util/BUILD).

z2edit_library(status status.cc status.h statusor.h)
target_link_libraries(z2edit_status PUBLIC absl::status absl::statusor)

z2edit_library(file file.cc file.h)
target_link_libraries(z2edit_file PUBLIC z2edit::status absl::strings)

z2edit_library(logging logging.cc logging.h)
target_link_libraries(z2edit_logging PUBLIC gflags::gflags absl::strings)

z2edit_library(os os.cc os.h)
target_link_libraries(z2edit_os PUBLIC z2edit::logging absl::strings)

z2edit_library(compress compress.cc compress.h)
target_link_libraries(z2edit_compress PUBLIC ZLIB::ZLIB z2edit::logging z2edit::status)

z2edit_library(crc crc.cc crc.h)
target_link_libraries(z2edit_crc PUBLIC ZLIB::ZLIB)

# config.h is header-only but pulls in file/logging/os.
add_library(z2edit_config INTERFACE)
target_include_directories(z2edit_config INTERFACE ${PROJECT_SOURCE_DIR})
target_link_libraries(z2edit_config INTERFACE
  z2edit::file z2edit::logging z2edit::os)
add_library(z2edit::config ALIAS z2edit_config)

z2edit_library(gamecontrollerdb gamecontrollerdb.cc gamecontrollerdb.h)
z2edit_library(browser browser.cc browser.h)

add_library(z2edit_fpsmgr INTERFACE)
target_include_directories(z2edit_fpsmgr INTERFACE ${PROJECT_SOURCE_DIR})
add_library(z2edit::fpsmgr ALIAS z2edit_fpsmgr)

add_library(z2edit_macros INTERFACE)
target_include_directories(z2edit_macros INTERFACE ${PROJECT_SOURCE_DIR})
add_library(z2edit::macros ALIAS z2edit_macros)

# stb single-header libs
add_library(z2edit_stb_tilemap_editor INTERFACE)
target_include_directories(z2edit_stb_tilemap_editor INTERFACE ${PROJECT_SOURCE_DIR})
add_library(z2edit::stb_tilemap_editor ALIAS z2edit_stb_tilemap_editor)

add_library(z2edit_stb_perlin INTERFACE)
target_include_directories(z2edit_stb_perlin INTERFACE ${PROJECT_SOURCE_DIR})
add_library(z2edit::stb_perlin ALIAS z2edit_stb_perlin)

# imgui backends carried in-tree
z2edit_library(imgui_sdl_opengl
  imgui_impl_opengl3.cpp imgui_impl_opengl3.h
  imgui_impl_opengl3_loader.h
  imgui_impl_sdl2.cpp imgui_impl_sdl2.h
  imgui_hidpi.cc imgui_hidpi.h)
target_link_libraries(z2edit_imgui_sdl_opengl PUBLIC imgui SDL2::SDL2)