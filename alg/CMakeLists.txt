# alg library set (mirrors alg/BUILD).

add_library(z2edit_vecmath INTERFACE)
target_link_libraries(z2edit_vecmath INTERFACE imgui)
target_include_directories(z2edit_vecmath INTERFACE ${PROJECT_SOURCE_DIR})
add_library(z2edit::vecmath ALIAS z2edit_vecmath)

z2edit_library(fdg fdg.cc fdg.h)
target_link_libraries(z2edit_fdg PUBLIC z2edit::vecmath)

z2edit_library(terrain terrain.cc terrain.h)
target_link_libraries(z2edit_terrain PUBLIC z2edit::stb_perlin)

z2edit_library(palace_gen palace_gen.cc palace_gen.h)
target_link_libraries(z2edit_palace_gen PUBLIC
  z2edit::simplemap z2edit::mappers z2edit::proto_generator z2edit::proto_rominfo
  z2edit::config)