cmake_minimum_required(VERSION 3.23)
project(volume_setter VERSION 0.1.1)

include(GNUInstallDirs)

find_package(argparse CONFIG REQUIRED)
find_package(cppwinrt CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(toml11 CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

set(EM_EXECUTABLE_NAME "volume-setter")
set(EM_EXECUTABLE_VERSION ${PROJECT_VERSION})

# Set common compilation and link options for the given target.
#
# Many options, such as warnings, should not be made PUBLIC for libraries
# because they should be up to the user. This function allows them to be PRIVATE
# without duplicating them explicitly for each target.
function(em_set_common target)
    target_compile_options(${target} PRIVATE
            # Needed for cppwinrt to compile properly.
            /await
            # Explicitly set the exception handling mode because CMake does not
            # currently pass these to clang-cl like it does with MSVC.
            /EHsc
            /GR
            # Set the source and execution encoding to UTF-8. The manifest file
            # ensures that the UTF-8 codepage is used at runtime.
            /utf-8
            # Set the value of __cplusplus correctly, which MSVC does not do by
            # default.
            /Zc:__cplusplus
            # Warnings are nice.
            /W4
            )
endfunction()

################################################################################
# Library
################################################################################
add_library(declvol_lib)
add_library(em::declvol_lib ALIAS declvol_lib)
em_set_common(declvol_lib)

target_sources(declvol_lib PRIVATE
        src/declvol/config.cpp
        src/declvol/exception.cpp
        src/declvol/process.cpp
        src/declvol/profile.cpp
        src/declvol/volume.cpp
        src/declvol/windows.cpp
        )
target_include_directories(declvol_lib PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        )
target_link_libraries(declvol_lib
        PUBLIC
        Microsoft::CppWinRT

        PRIVATE
        toml11::toml11
        )

################################################################################
# Executable
################################################################################
add_executable(declvol)
em_set_common(declvol)

target_sources(declvol PRIVATE
        app.manifest

        src/declvol/executable.cpp
        src/declvol/v1/declvol.grpc.pb.cc
        src/declvol/v1/declvol.pb.cc
        )
target_include_directories(declvol PRIVATE src)
target_link_libraries(declvol PRIVATE
        argparse::argparse
        em::declvol_lib
        gRPC::grpc++
        )

target_compile_definitions(declvol PRIVATE
        EM_EXECUTABLE_NAME="${EM_EXECUTABLE_NAME}"
        EM_EXECUTABLE_VERSION="${EM_EXECUTABLE_VERSION}"
        )
set_target_properties(declvol PROPERTIES
        OUTPUT_NAME ${EM_EXECUTABLE_NAME}
        )

install(TARGETS declvol
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES "LICENSE"
        DESTINATION ${CMAKE_INSTALL_BINDIR})
