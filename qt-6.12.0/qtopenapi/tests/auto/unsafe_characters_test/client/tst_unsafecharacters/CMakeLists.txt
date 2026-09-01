# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(cpp-qt-unsafe-characters LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(cpp-qt-unsafe-characters
    SOURCES
        tst_safecharacters.cpp
    LIBRARIES
        Qt::Core
        Qt::Network
        Qt::Test
)

qt_add_library(GenTestClient)
qt_add_openapi_client(GenTestClient
    SPEC_FILE
        ${CMAKE_CURRENT_SOURCE_DIR}/unsafe-characters-test.yaml
    OUTPUT_DIRECTORY
        ${CMAKE_CURRENT_BINARY_DIR}
)

if(NOT WIN32)
    target_compile_options(GenTestClient PUBLIC "-Wno-comment")
endif()

qt_autogen_tools_initial_setup(GenTestClient)
target_link_libraries(cpp-qt-unsafe-characters PRIVATE GenTestClient)

if(WIN32)
    add_custom_command(TARGET cpp-qt-unsafe-characters POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:GenTestClient>
            ${CMAKE_CURRENT_BINARY_DIR}
        VERBATIM)
endif()
