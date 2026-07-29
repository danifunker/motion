# Motion - An SGI Emulator!
# Copyright (C) 2026 starfrost

cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# $<CONFIG> rather than ${CMAKE_BUILD_TYPE}: the variable is empty unless someone
# passes -DCMAKE_BUILD_TYPE, and it is ALWAYS empty on multi-config generators
# (Xcode, Visual Studio), which then append their own config dir on top.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/$<CONFIG>)

# mingw-w64 doesn't support static linking against the UCRT

# Install path
set(ROM_SRC_DIR "${CMAKE_SOURCE_DIR}/roms")
set(ROM_DST_DIR "${CMAKE_BINARY_DIR}/output/$<CONFIG>/roms")

project(motion)

# Build SDL static, we like static libraries (esp. on Linux, where the system SDL3 might not be the same version!)

set(SDL_SHARED OFF CACHE BOOL "Build SDL shared library" FORCE)
set(SDL_STATIC ON CACHE BOOL "Build SDL static library" FORCE)

# Add our submodule directories

add_subdirectory(external/sdl3 EXCLUDE_FROM_ALL)

# Also allow us to use our own headers. At some point, we need to have a public/private header distinction...
add_subdirectory(src)


if(WIN32 AND MINGW)
    target_link_options(motion PRIVATE 
        -static -static-libgcc -static-libstdc++ 
        -Wl,-Bstatic -lwinpthread -Wl,-Bdynamic
    )
elseif(APPLE)
    # Apple Clang hard-errors on -static-libgcc and silently ignores
    # -static-libstdc++. libc++ is part of the macOS ABI and always comes from
    # /usr/lib, so there is nothing to static link here anyway.
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # if you statically link everything basically pipewrie will KILL SDL lol
    target_link_options(motion PRIVATE -static-libstdc++ -static-libgcc)             # static stdc++
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Clang on Linux/BSD: -static-libstdc++ is fine, -static-libgcc is not reliable
    target_link_options(motion PRIVATE -static-libstdc++)
endif()

target_link_libraries(motion PRIVATE 
SDL3::SDL3-static                   # static sdl3
moira)                              # 68k


# install
add_custom_target(InstallAssets 
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${ROM_SRC_DIR}" "${ROM_DST_DIR}"
    COMMENT "Updating ROMs..."
)

add_dependencies(motion InstallAssets)

# Packaging. Without these, `cmake --install` produces nothing and the release
# workflow has no artifacts to attach.
#
# Flat layout on purpose: the PROM is loaded from a path relative to the working
# directory (see prom.cpp), so roms/ has to sit next to the executable.
install(TARGETS motion RUNTIME DESTINATION .)

# OFF by default. The PROM images are SGI firmware, and attaching them to a
# public release is redistribution of third-party copyrighted material - which
# is a different thing legally from keeping them in a repo you control.
option(MOTION_INSTALL_ROMS "Bundle roms/ into the install tree" OFF)

if(MOTION_INSTALL_ROMS)
    install(DIRECTORY "${ROM_SRC_DIR}/" DESTINATION roms)
endif()