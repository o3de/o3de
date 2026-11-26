#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(astc-encoder
    VERSION "5.7.1"
    LICENSE "Apache-2.0"
    URL "https://github.com/ARM-software/astc-encoder/archive/refs/tags/5.7.1.tar.gz"
    URL_HASH "37ff3928c4c156fb3ac0d1adb5df144ff9c5cdbc8a8e1ffeb20229cc7f35c767"
    GIT "https://github.com/ARM-software/astc-encoder.git"
    GIT_HASH "0affb51aaf0e0de3a8b038fa32487d59754a1eed"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(astc-encoder)

file(COPY
    "${astc-encoder_SOURCE_DIR}/Source/astcenc.h"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

o3de_disable_warnings()

ly_add_target(
    NAME astc-encoder STATIC
    NAMESPACE 3rdParty
    NO_UNITY
    FILES_CMAKE files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/Include"
    COMPILE_DEFINITIONS
        PRIVATE
            $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
)

target_compile_options(astc-encoder PRIVATE
    ${O3DE_COMPILE_OPTION_DISABLE_FAST_MATH}
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<STREQUAL:${CMAKE_CXX_COMPILER_FRONTEND_VARIANT},MSVC>>:-Xclang>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-ffp-contract=off>
)

set_target_properties(astc-encoder PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    SYSTEM TRUE
)

find_package(Threads REQUIRED)
ly_target_link_libraries(astc-encoder PUBLIC Threads::Threads)

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/astc-encoder
)
