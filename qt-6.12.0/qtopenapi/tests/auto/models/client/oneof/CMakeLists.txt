# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(cpp-qt-oneof LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(cpp-qt-oneof
    SOURCES
        tst_oneof.cpp
    LIBRARIES
        Qt::Core
        Qt::Network
        Qt::Test
)

qt_add_library(basicSchemaAlternatives)
qt_add_openapi_client(basicSchemaAlternatives
    SPEC_FILE
        ${CMAKE_CURRENT_SOURCE_DIR}/basic-schema-alternatives.yaml
    ADDITIONAL_PROPERTIES
        cppNamespace=SchemasModelsOneOf
    OUTPUT_DIRECTORY
        ${CMAKE_CURRENT_BINARY_DIR}/basicSchemaAlternatives
)

qt_autogen_tools_initial_setup(basicSchemaAlternatives)
target_link_libraries(cpp-qt-oneof PRIVATE basicSchemaAlternatives)

if(WIN32)
    add_custom_command(TARGET cpp-qt-oneof POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:basicSchemaAlternatives>
            ${CMAKE_CURRENT_BINARY_DIR}
        VERBATIM)
endif()
