cmake_minimum_required(VERSION 3.23)
project(volume_setter)

find_package(argparse CONFIG REQUIRED)
find_package(cppwinrt CONFIG REQUIRED)
find_package(toml11 CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 23)

add_executable(volume_setter main.cpp)
target_sources(volume_setter PRIVATE
        main.cpp
        app.manifest
        )

target_compile_options(volume_setter PRIVATE /await /utf-8 /W4)
target_link_libraries(volume_setter PRIVATE
        argparse::argparse
        Microsoft::CppWinRT
        toml11::toml11
        )
