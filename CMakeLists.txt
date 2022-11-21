cmake_minimum_required(VERSION 3.23)
project(volume_setter VERSION 0.1.1)

include(GNUInstallDirs)

find_package(argparse CONFIG REQUIRED)
find_package(cppwinrt CONFIG REQUIRED)
find_package(toml11 CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

set(EM_EXECUTABLE_NAME "volume-setter")
set(EM_EXECUTABLE_VERSION ${PROJECT_VERSION})

add_executable(volume_setter)
target_sources(volume_setter PRIVATE
        app.manifest
        src/declvol/main.cpp
        )

target_compile_options(volume_setter PRIVATE
        /await
        /EHsc
        /GR
        /utf-8
        /Zc:__cplusplus
        )
target_compile_options(volume_setter PRIVATE
        /W4
        )
target_compile_definitions(volume_setter PRIVATE
        EM_EXECUTABLE_NAME="${EM_EXECUTABLE_NAME}"
        EM_EXECUTABLE_VERSION="${EM_EXECUTABLE_VERSION}"
        )
target_link_libraries(volume_setter PRIVATE
        argparse::argparse
        Microsoft::CppWinRT
        toml11::toml11
        )
set_target_properties(volume_setter PROPERTIES
        OUTPUT_NAME ${EM_EXECUTABLE_NAME}
        )

install(TARGETS volume_setter
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES "LICENSE"
        DESTINATION ${CMAKE_INSTALL_BINDIR})
