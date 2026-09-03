#!/bin/bash
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

$HOME/bauhaus-suite/setup.sh --non-interactive
export PATH=/home/qt/bauhaus-suite/bin:$PATH
export BAUHAUS_CONFIG=$(cd $(dirname $(readlink -f $0)) && pwd)
export AXIVION_VERSION_NAME=$(git rev-parse HEAD)
export CAFECC_BASEPATH="/home/qt/work/qt/$TESTED_MODULE_COIN"
export AXIVION_NUM_JOBS_COMPILE="4"
export AXIVION_NUM_JOBS_LINK="1"
export MODULE=$TESTED_MODULE_COIN
gccsetup --cc gcc --cxx g++ --config "$BAUHAUS_CONFIG"
cd "$CAFECC_BASEPATH"
BAUHAUS_IR_COMPRESSION=none COMPILE_ONLY=1 cmake -G Ninja -DAXIVION_ANALYSIS_TOOLCHAIN_FILE=/home/qt/bauhaus-suite/profiles/cmake/axivion-launcher-toolchain.cmake -DCMAKE_PREFIX_PATH=/home/qt/work/qt/qtopenapi/build -DCMAKE_PROJECT_INCLUDE_BEFORE=/home/qt/bauhaus-suite/profiles/cmake/axivion-before-project-hook.cmake -DQT_BUILD_TESTS=ON -B build -S . --fresh
cmake --build build --verbose -j4

ROOT_DIR=src
MAGIC="Qt-Security score:critical"

INCLUDE_FILES=""

EXTRA_DIRS=(
    "build/tests/auto/qtopenapigen/client/openapigen/cmake_generated/clienttest/client"
    "build/tests/auto/qtopenapigen/client/openapigen/cmake_generated/commonlibtest/common"
)

while IFS= read -r file; do
    if [ -z "$INCLUDE_FILES" ]; then
        INCLUDE_FILES="$file"
    else
        INCLUDE_FILES="$INCLUDE_FILES:$file"
    fi
done < <(
    grep -rl "$MAGIC" "$ROOT_DIR"

    for dir in "${EXTRA_DIRS[@]}"; do
        [ -d "$dir" ] && find "$dir" -maxdepth 1 -type f
    done
)

export INCLUDE_FILES

for MODULE in qtopenapi qtopenapi_generator-common qtopenapi_generator-client; do
    export MODULE
    export EXCLUDE_FILES=""
    export PLUGINS=""
    export IRNAME=build/$MODULE.ir
    if [ "$MODULE" == "qtopenapi" ]
    then
        export TARGET_NAME="build/lib/libQt6OpenApiCommon.so.*.ir"
        export PACKAGE="Add-ons"
    elif [ "$MODULE" == "qtopenapi_generator-common" ]
    then
        export TARGET_NAME="build/tests/auto/qtopenapigen/client/openapigen/libcmakeCommonLibGeneratedClient.so*.ir"
        export PACKAGE="Add-ons"
    elif [ "$MODULE" == "qtopenapi_generator-client" ]
    then
        export EXCLUDE_FILES="build/tests/auto/qtopenapigen/client/openapigen/cmake_generated/commonlibtest/common/*"
        export TARGET_NAME="build/tests/auto/qtopenapigen/client/openapigen/libcmakeGeneratedClient.so*.ir"
        export PACKAGE="Add-ons"
    fi
    axivion_ci "$@"
done

