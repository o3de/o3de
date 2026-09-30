#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(astc-encoder
    VERSION "3.2"
    LICENSE "Apache-2.0"
    URL "https://github.com/ARM-software/astc-encoder/archive/refs/tags/3.2.tar.gz"
    URL_HASH "ed24d600b3b18178df3c6375e2ba8f256ccfc1f20e57863f2f21a1c9e39f820e"
    GIT "https://github.com/ARM-software/astc-encoder.git"
    GIT_HASH "f32d8d4dd9e06f8986cd5e10f12e199fc023f150"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(astc-encoder)

file(COPY
    "${astc-encoder_SOURCE_DIR}/Source/astcenc.h"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND CMAKE_CXX_COMPILER_ARCHITECTURE_ID STREQUAL "x64")
    set(astc_encoder_compile_definitions ASTCENC_SSE=41 ASTCENC_POPCNT=1)
endif()

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
            ${astc_encoder_compile_definitions}
            $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
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
