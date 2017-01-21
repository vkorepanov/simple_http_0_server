# Distributed under terms of the MIT license.
# Copyright (C) 2017 Korepanov Vyacheslav <real93@live.ru>

cmake_minimum_required (VERSION 2.8)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 11)

project(simple_http_0_server)

find_package(Threads REQUIRED)

add_subdirectory(boost_parser)

set(SRCS
    common.h
    server.cpp
    server.h
    )
add_library(SourcesLib OBJECT ${SRCS})

add_executable(final $<TARGET_OBJECTS:SourcesLib> main.cpp)
target_link_libraries(final PUBLIC ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(final PRIVATE BoostParserLib)

target_compile_options(final PRIVATE -Wall -Wextra -Wpedantic -Werror)

if (UNIX AND NOT "${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
    add_custom_target(${PROJECT_NAME}-symlink
        ALL ln --force -s
            ${CMAKE_CURRENT_BINARY_DIR}/final
            ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}
        DEPENDS ${PROJECT_NAME})

    set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES
        ${CMAKE_SOURCE_DIR}/${PROJECT_NAME})
endif()
