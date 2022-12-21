cmake_minimum_required(VERSION 3.9)

project(serum VERSION 0.1.0 DESCRIPTION
        "Cross-platform library for decoding Serum files, a colorization format for pinball ROMs.")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 99)

option(USE_DLLEXPORTS "dll exports" OFF)
if (USE_DLLEXPORTS)
    add_compile_definitions(DLL_EXPORTS)
endif()

option(USE_WIN32 "Win32" OFF)
if (USE_WIN32)
    add_compile_definitions(WIN32)
endif()

option(USE_OSXARM "macOS arm64" OFF)
if (USE_OSXARM)
    set(CMAKE_OSX_ARCHITECTURES arm64)
endif()

option(USE_OSXINTEL "macOS x86_64" OFF)
if (USE_OSXINTEL)
    set(CMAKE_OSX_ARCHITECTURES x86_64)
endif()

add_library(serum SHARED
        src/miniz/zip_file.hpp
        src/serum-decode.h
        src/serum-decode.cpp
        )

set_target_properties(serum PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
        )

target_include_directories(serum PUBLIC
        src
        )

install(TARGETS serum
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
