cmake_minimum_required(VERSION 3.9)

project(serum VERSION 0.1.0 DESCRIPTION
        "Cross-platform library for decoding Serum files, a colorization format for pinball ROMs.")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_VISIBILITY_PRESET hidden)

include(GNUInstallDirs)

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
