include_directories(libraries)

add_executable(program1 program1.c)
add_executable(program2 program2.c)

add_subdirectory(libraries)

target_link_libraries(program1 math1)
target_link_libraries(program1 math2)