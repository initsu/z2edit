# Protobuf code generation.  protoc is invoked with -I <src> so the virtual
# path of each .proto is "proto/x.proto"; the generated headers therefore
# appear as <gendir>/proto/x.pb.h and the source tree's #include
# "proto/x.pb.h" resolves against <gendir> (added globally in the root).

if(TARGET protobuf::protoc)
  set(Z2EDIT_PROTOC protobuf::protoc)
else()
  set(Z2EDIT_PROTOC protoc)
endif()

function(z2edit_proto_library NAME)
  set(PROTO_FILES ${ARGN})
  set(GEN_CC)
  set(PROTO_PATHS)
  foreach(pf IN LISTS PROTO_FILES)
    get_filename_component(base "${pf}" NAME_WE)
    list(APPEND GEN_CC
      "${Z2EDIT_GENDIR}/proto/${base}.pb.cc"
      "${Z2EDIT_GENDIR}/proto/${base}.pb.h")
    list(APPEND PROTO_PATHS "${PROJECT_SOURCE_DIR}/proto/${pf}")
  endforeach()

  add_custom_command(
    OUTPUT ${GEN_CC}
    COMMAND ${Z2EDIT_PROTOC}
      ARGS
        -I ${PROJECT_SOURCE_DIR}
        --cpp_out ${Z2EDIT_GENDIR}
        ${PROTO_PATHS}
    DEPENDS ${PROTO_PATHS}
    COMMENT "Generating protobuf C++ for ${NAME}")

  add_library(z2edit_proto_${NAME} STATIC ${GEN_CC})
  target_link_libraries(z2edit_proto_${NAME} PUBLIC protobuf::libprotobuf)
  target_include_directories(z2edit_proto_${NAME} PUBLIC ${Z2EDIT_GENDIR})
  z2edit_setup(z2edit_proto_${NAME})
  add_library(z2edit::proto_${NAME} ALIAS z2edit_proto_${NAME})
endfunction()

set(Z2EDIT_PROTO_SRC ${CMAKE_CURRENT_SOURCE_DIR})

z2edit_proto_library(rominfo keybinds.proto rominfo.proto)
z2edit_proto_library(session session.proto)
z2edit_proto_library(generator generator.proto)
z2edit_proto_library(project project.proto)
target_link_libraries(z2edit_proto_project PUBLIC
  z2edit::proto_rominfo z2edit::proto_session)