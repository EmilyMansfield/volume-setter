cmake_minimum_required(VERSION 3.23)
project(volume_setter)

include(GNUInstallDirs)

find_package(argparse CONFIG REQUIRED)
find_package(cppwinrt CONFIG REQUIRED)
find_package(toml11 CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

add_executable(volume_setter)
target_sources(volume_setter PRIVATE
        main.cpp
        app.manifest
        )

target_compile_options(volume_setter PRIVATE
        /await
        /utf-8
        /Zc:__cplusplus
        )
target_compile_options(volume_setter PRIVATE
        /W4
        )
target_link_libraries(volume_setter PRIVATE
        argparse::argparse
        Microsoft::CppWinRT
        toml11::toml11
        )
set_target_properties(volume_setter PROPERTIES
        OUTPUT_NAME "volume-setter"
        )

install(TARGETS volume_setter
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
