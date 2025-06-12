# pack_config: converts a textpb config into a C header (mirrors tools/BUILD).

add_executable(pack_config pack_config.cc)
z2edit_setup(pack_config)
target_link_libraries(pack_config PRIVATE
  z2edit::proto_rominfo z2edit::config gflags::gflags)