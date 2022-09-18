cmake_minimum_required(VERSION 3.23)
project(volume_setter)

set(CMAKE_CXX_STANDARD 23)

add_executable(volume_setter main.cpp)

target_compile_options(volume_setter PRIVATE /await)
target_link_libraries(volume_setter PRIVATE windowsapp)
