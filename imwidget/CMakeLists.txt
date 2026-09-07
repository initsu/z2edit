# imwidget library set (mirrors imwidget/BUILD).

z2edit_library(base
  debug_console.cc debug_console.h
  imapp.cc imapp.h
  imutil.cc imutil.h
  imwidget.h)
target_link_libraries(z2edit_base PUBLIC
  gflags::gflags imgui SDL3::SDL3
  z2edit::fpsmgr z2edit::gamecontrollerdb z2edit::imgui_sdl_opengl
  z2edit::logging z2edit::os)

z2edit_library(error_dialog error_dialog.cc error_dialog.h)
target_link_libraries(z2edit_error_dialog PUBLIC z2edit::base absl::strings)

z2edit_library(glbitmap glbitmap.cc glbitmap.h)
target_link_libraries(z2edit_glbitmap PUBLIC imgui SDL3::SDL3)

z2edit_library(hwpalette hwpalette.cc hwpalette.h)
target_link_libraries(z2edit_hwpalette PUBLIC z2edit::base)

z2edit_library(neschrview neschrview.cc neschrview.h)
target_link_libraries(z2edit_neschrview PUBLIC
  z2edit::base z2edit::error_dialog z2edit::glbitmap z2edit::mappers nfd)

z2edit_library(editor editor.cc editor.h)
if(NOT MSVC)
  target_compile_options(z2edit_editor PRIVATE
    -fpermissive
    -Wno-unused-function
    -Wno-unused-variable
    -Wno-unused-but-set-variable
    -Wno-write-strings
    -Wno-comment
    -Wno-narrowing)
endif()
target_link_libraries(z2edit_editor PUBLIC
  z2edit::base z2edit::error_dialog z2edit::glbitmap z2edit::hwpalette
  z2edit::map_connect z2edit::overworld_encounters z2edit::randomize
  gflags::gflags z2edit::mappers z2edit::z2decompress z2edit::z2objcache
  z2edit::proto_rominfo z2edit::config z2edit::imgui_sdl_opengl
  z2edit::stb_tilemap_editor)

z2edit_library(palette palette.cc palette.h)
target_link_libraries(z2edit_palette PUBLIC
  z2edit::base z2edit::error_dialog z2edit::hwpalette
  gflags::gflags z2edit::mappers z2edit::proto_rominfo z2edit::config)

z2edit_library(simplemap map_command.cc map_command.h simplemap.cc simplemap.h)
target_link_libraries(z2edit_simplemap PUBLIC
  z2edit::base z2edit::error_dialog z2edit::glbitmap z2edit::hwpalette
  z2edit::overworld_encounters fontawesome
  z2edit::enemylist z2edit::mappers z2edit::text_list z2edit::z2decompress
  z2edit::z2objcache z2edit::proto_rominfo z2edit::config absl::strings)

z2edit_library(multimap multimap.cc multimap.h)
target_link_libraries(z2edit_multimap PUBLIC
  z2edit::base z2edit::glbitmap z2edit::simplemap
  z2edit::fdg z2edit::palace_gen
  z2edit::mappers z2edit::z2decompress z2edit::z2objcache
  z2edit::proto_generator z2edit::proto_rominfo z2edit::proto_session
  z2edit::config z2edit::macros)

z2edit_library(map_connect map_connect.cc map_connect.h)
target_link_libraries(z2edit_map_connect PUBLIC
  z2edit::base z2edit::multimap z2edit::mappers z2edit::proto_rominfo)

z2edit_library(overworld_encounters overworld_encounters.cc overworld_encounters.h)
target_link_libraries(z2edit_overworld_encounters PUBLIC
  z2edit::mappers z2edit::config)

z2edit_library(palace_gfx palace_gfx.cc palace_gfx.h)
target_link_libraries(z2edit_palace_gfx PUBLIC
  z2edit::base z2edit::mappers z2edit::proto_rominfo z2edit::config)

z2edit_library(randomize randomize.cc randomize.h)
target_link_libraries(z2edit_randomize PUBLIC
  z2edit::map_connect z2edit::terrain z2edit::proto_rominfo z2edit::config
  z2edit::stb_tilemap_editor)

z2edit_library(start_values start_values.cc start_values.h)
target_link_libraries(z2edit_start_values PUBLIC
  z2edit::base z2edit::mappers z2edit::config)

z2edit_library(object_table object_table.cc object_table.h)
target_link_libraries(z2edit_object_table PUBLIC
  z2edit::base z2edit::hwpalette z2edit::mappers z2edit::z2objcache
  z2edit::proto_rominfo z2edit::config gflags::gflags)

z2edit_library(enemyattr enemyattr.cc enemyattr.h)
target_link_libraries(z2edit_enemyattr PUBLIC
  z2edit::base z2edit::error_dialog gflags::gflags z2edit::mappers
  z2edit::z2objcache z2edit::proto_rominfo z2edit::config)

z2edit_library(xptable xptable.cc xptable.h)
target_link_libraries(z2edit_xptable PUBLIC
  z2edit::base z2edit::error_dialog gflags::gflags z2edit::mappers
  z2edit::text_encoding z2edit::z2objcache z2edit::proto_rominfo z2edit::config)

z2edit_library(text_table text_table.cc text_table.h)
target_link_libraries(z2edit_text_table PUBLIC
  z2edit::base z2edit::error_dialog gflags::gflags z2edit::mappers
  z2edit::text_list z2edit::proto_rominfo z2edit::config)

z2edit_library(project project.cc project.h)
target_link_libraries(z2edit_project PUBLIC
  z2edit::base z2edit::error_dialog z2edit::ips z2edit::cartridge
  z2edit::proto_project z2edit::compress z2edit::config z2edit::file
  z2edit::logging z2edit::os z2edit::status)

z2edit_library(drops drops.cc drops.h)
target_link_libraries(z2edit_drops PUBLIC
  z2edit::base z2edit::simplemap z2edit::mappers z2edit::proto_rominfo z2edit::config)

z2edit_library(tile_transform tile_transform.cc tile_transform.h)
target_link_libraries(z2edit_tile_transform PUBLIC
  z2edit::base z2edit::mappers z2edit::proto_rominfo z2edit::config)

z2edit_library(item_effects item_effects.cc item_effects.h)
target_link_libraries(z2edit_item_effects PUBLIC
  z2edit::base z2edit::mappers z2edit::proto_rominfo z2edit::config)

z2edit_library(rom_memory rom_memory.cc rom_memory.h)
target_link_libraries(z2edit_rom_memory PUBLIC
  z2edit::base z2edit::glbitmap gflags::gflags z2edit::mappers)