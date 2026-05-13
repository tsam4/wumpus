cmake_minimum_required(VERSION 3.13)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(example example.cc)
target_link_libraries(example emdw)
