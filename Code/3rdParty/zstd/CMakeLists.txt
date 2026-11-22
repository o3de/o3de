#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(zstd
    VERSION "1.5.7"
    LICENSE "BSD-3-Clause"
    URL "https://github.com/facebook/zstd/archive/refs/tags/v1.5.7.tar.gz"
    URL_HASH "37d7284556b20954e56e1ca85b80226768902e2edabd3b649e9e72c0c9012ee3"
    GIT "https://github.com/facebook/zstd.git"
    GIT_HASH "f8745da6ff1ad1e7bab384bd1f9d742439278e99"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(zstd)

file(COPY
    "${zstd_SOURCE_DIR}/lib/zstd.h"
    "${zstd_SOURCE_DIR}/lib/zstd_errors.h"
    "${zstd_SOURCE_DIR}/lib/zdict.h"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

set(zstd_asm OFF)
if(NOT MSVC AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang" AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)$")
    enable_language(ASM)
    set(zstd_asm ON)
endif()

o3de_disable_warnings()

ly_add_target(
    NAME zstd STATIC
    NAMESPACE 3rdParty
    NO_UNITY
    FILES_CMAKE files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            "${zstd_SOURCE_DIR}/lib"
            "${zstd_SOURCE_DIR}/lib/common"
            "${zstd_SOURCE_DIR}/lib/legacy"
        PUBLIC
            "${CMAKE_CURRENT_BINARY_DIR}/Include"
    COMPILE_DEFINITIONS
        PRIVATE
            ZSTD_LEGACY_SUPPORT=4
            $<$<NOT:$<BOOL:${zstd_asm}>>:ZSTD_DISABLE_ASM>
            $<$<BOOL:${MSVC}>:ZSTD_HEAPMODE=0>
            $<$<BOOL:${MSVC}>:_CRT_SECURE_NO_WARNINGS>
)

if(NOT ANDROID AND NOT EMSCRIPTEN)
    target_compile_definitions(zstd PRIVATE ZSTD_MULTITHREAD)
    if(UNIX)
        ly_target_link_libraries(zstd PRIVATE pthread)
    endif()
endif()

set_target_properties(zstd PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
    SYSTEM TRUE
)

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/zstd
)
