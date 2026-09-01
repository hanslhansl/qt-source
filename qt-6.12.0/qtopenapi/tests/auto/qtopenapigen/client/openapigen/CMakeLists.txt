# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(qt-openapi-generator LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

set(cmake_generated_dir "cmake_generated")
qt_internal_add_test(qt-openapi-generator-client
    SOURCES
        qtopenapigen_common.h
        tst_openapigen.cpp
    LIBRARIES
        Qt::Core
        Qt::Network
        Qt::Test
    DEFINES
        CMAKE_GENERATED_DIR="${cmake_generated_dir}"
        BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)

#################################
# Generate Qt Client Library
#################################
set(cmake_generator_tests_client "")
qt_add_library(cmakeGeneratedClient)
qt_add_openapi_client(cmakeGeneratedClient
    SPEC_FILE
        ${CMAKE_CURRENT_SOURCE_DIR}/data/petstore.yaml
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${cmake_generated_dir}/clienttest"
)
qt_autogen_tools_initial_setup(cmakeGeneratedClient)
target_link_libraries(qt-openapi-generator-client PRIVATE cmakeGeneratedClient)
list(APPEND cmake_generator_tests_client clienttest)

#################################
# Generate Qt Common Library
#################################
qt_internal_add_test(qt-openapi-generator-common
    SOURCES
        qtopenapigen_common.h
        tst_commonopenapigen.cpp
    LIBRARIES
        Qt::Core
        Qt::Network
        Qt::Test
    DEFINES
        CMAKE_GENERATED_DIR="${cmake_generated_dir}"
        BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)

set(cmake_generator_tests_common "")
qt_add_library(cmakeCommonLibGeneratedClient)
qt6_add_openapi_client(cmakeCommonLibGeneratedClient
    SPEC_FILE
        # want to reuse existing one YAML file for commonlib
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../src/openapicommon/qtopenapi-common.yaml
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${cmake_generated_dir}/commonlibtest"
    __QT_INTERNAL_GENERATE_COMMON_LIBRARY_TARGET
)
qt_autogen_tools_initial_setup(cmakeCommonLibGeneratedClient)
target_compile_definitions(cmakeCommonLibGeneratedClient
    PUBLIC QT_OPENAPI_GENERATOR_VERSION=\"${QT_OPENAPI_GENERATOR_VERSION}\")
target_link_libraries(cmakeCommonLibGeneratedClient PRIVATE Qt::Core Qt::Network)
target_link_libraries(qt-openapi-generator-common PRIVATE cmakeCommonLibGeneratedClient)
list(APPEND cmake_generator_tests_common commonlibtest)

# Handle the conditional finding of either system zlib or qt zlib.
if(NOT QT_FEATURE_system_zlib)
    find_package(Qt6 COMPONENTS ZlibPrivate)
    qt_internal_extend_target(cmakeCommonLibGeneratedClient LIBRARIES Qt::ZlibPrivate)
elseif(NOT TARGET WrapZLIB::WrapZLIB)
    qt_find_package(WrapZLIB PROVIDED_TARGETS WrapZLIB::WrapZLIB)
endif()

if(TARGET WrapZLIB::WrapZLIB)
    qt_internal_extend_target(cmakeCommonLibGeneratedClient LIBRARIES WrapZLIB::WrapZLIB)
endif()

if(WIN32)
    add_custom_command(TARGET qt-openapi-generator-client POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:cmakeGeneratedClient>
            ${CMAKE_CURRENT_BINARY_DIR}
        VERBATIM)
    add_custom_command(TARGET qt-openapi-generator-common POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:cmakeCommonLibGeneratedClient>
            ${CMAKE_CURRENT_BINARY_DIR}
        VERBATIM)
endif()

list(JOIN cmake_generator_tests_client "," cmake_generator_tests_client_string)
qt_internal_extend_target(qt-openapi-generator-client
    DEFINES
        CMAKE_GENERATOR_TESTS_CLIENT="${cmake_generator_tests_client_string}"
)

list(JOIN cmake_generator_tests_common "," cmake_generator_tests_common_string)
qt_internal_extend_target(qt-openapi-generator-common
    DEFINES
        CMAKE_GENERATOR_TESTS_COMMON="${cmake_generator_tests_common_string}"
)
