# music tools (mirrors music/BUILD, targets renamed to avoid clashing with
# CTest's reserved "test" name).

z2edit_library(music music.cc music.h)

add_executable(music_test main.cc)
target_link_libraries(music_test PRIVATE z2edit::music)

add_executable(solstice solstice.cc)
target_link_libraries(solstice PRIVATE z2edit::music)

add_executable(rog rog.cc)
target_link_libraries(rog PRIVATE z2edit::music)

add_executable(undersea undersea.cc)
target_link_libraries(undersea PRIVATE z2edit::music)

add_executable(amida amida.cc)
target_link_libraries(amida PRIVATE z2edit::music)

add_executable(credits credits.cc)
target_link_libraries(credits PRIVATE z2edit::music)