# nes library set (mirrors nes/BUILD).

z2edit_library(cartridge cartridge.cc cartridge.h)
target_link_libraries(z2edit_cartridge PUBLIC gflags::gflags z2edit::base z2edit::file)

z2edit_library(chr_util chr_util.cc chr_util.h)
target_link_libraries(z2edit_chr_util PUBLIC z2edit::mappers)

z2edit_library(cpu6502 cpu6502.cc cpu6502.h)
target_link_libraries(z2edit_cpu6502 PUBLIC z2edit::mappers absl::strings)

z2edit_library(enemylist enemylist.cc enemylist.h)
target_link_libraries(z2edit_enemylist PUBLIC
  z2edit::mappers z2edit::proto_rominfo z2edit::config z2edit::logging)

# mappers rely on a static registry referenced through function pointers, so
# they must be linked whole (bazel set alwayslink = 1).
z2edit_library(mappers mapper.cc mapper1.cc mapper1.h memory.cc memory.h mapper.h)
target_link_libraries(z2edit_mappers PUBLIC
  z2edit::cartridge z2edit::base z2edit::proto_rominfo z2edit::config z2edit::logging)
if(MSVC)
  target_link_options(z2edit_mappers INTERFACE
    "/WHOLEARCHIVE:$<TARGET_FILE:z2edit_mappers>")
else()
  target_link_options(z2edit_mappers INTERFACE
    "-Wl,--whole-archive,$<TARGET_FILE:z2edit_mappers>"
    "-Wl,--no-whole-archive")
endif()

z2edit_library(text_encoding text_encoding.cc text_encoding.h)

z2edit_library(text_list text_list.cc text_list.h)
target_link_libraries(z2edit_text_list PUBLIC
  z2edit::mappers z2edit::text_encoding z2edit::proto_rominfo z2edit::config z2edit::logging)

z2edit_library(z2decompress z2decomp_funcs.cc z2decompress.cc z2decompress.h)
target_link_libraries(z2edit_z2decompress PUBLIC
  z2edit::mappers gflags::gflags z2edit::proto_rominfo z2edit::config z2edit::logging)

z2edit_library(z2objcache z2objcache.cc z2objcache.h)
target_link_libraries(z2edit_z2objcache PUBLIC
  z2edit::mappers z2edit::glbitmap z2edit::hwpalette z2edit::proto_rominfo
  z2edit::config z2edit::logging)