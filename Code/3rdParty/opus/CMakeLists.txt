#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

set(opus_version "1.6.1")
o3de_fetch_content(opus
    VERSION "${opus_version}"
    LICENSE "BSD-3-Clause"
    URL "https://downloads.xiph.org/releases/opus/opus-${opus_version}.tar.gz"
    URL_HASH "6ffcb593207be92584df15b32466ed64bbec99109f007c82205f0194572411a1"
    GIT "https://github.com/xiph/opus.git"
    GIT_HASH "22244de5a79bd1d6d623c32e72bf1954b56235be"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(opus)

file(COPY
    "${opus_SOURCE_DIR}/include/"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include/opus"
    FILES_MATCHING
        PATTERN "*.h"
        PATTERN "opus_custom.h" EXCLUDE
)

o3de_disable_warnings()

set(opus_architecture "${CMAKE_SYSTEM_PROCESSOR}")
if(CMAKE_OSX_ARCHITECTURES)
    set(opus_architecture "${CMAKE_OSX_ARCHITECTURES}")
elseif(CMAKE_C_COMPILER_ARCHITECTURE_ID)
    set(opus_architecture "${CMAKE_C_COMPILER_ARCHITECTURE_ID}")
endif()
string(TOLOWER "${opus_architecture}" opus_architecture)

set(opus_simd "")
set(opus_platform_definitions)
set(opus_platform_includes)

if(MSVC)
    list(APPEND opus_platform_definitions USE_ALLOCA _CRT_SECURE_NO_WARNINGS)
    set(opus_c_standard 11)
else()
    list(APPEND opus_platform_definitions VAR_ARRAYS)
    set(opus_c_standard 99)
endif()

if(opus_architecture MATCHES "^(amd64|x64|x86_64)$")
    set(opus_simd x86_64)
    list(APPEND opus_platform_definitions
        OPUS_HAVE_RTCD
        OPUS_X86_MAY_HAVE_SSE
        OPUS_X86_MAY_HAVE_SSE2
        OPUS_X86_MAY_HAVE_SSE4_1
        OPUS_X86_MAY_HAVE_AVX2
        OPUS_X86_PRESUME_SSE
        OPUS_X86_PRESUME_SSE2
        $<$<NOT:$<C_COMPILER_ID:MSVC>>:CPU_INFO_BY_ASM>
    )
elseif(opus_architecture MATCHES "^(aarch64|arm64)$")
    set(opus_simd arm_neon)
    list(APPEND opus_platform_definitions
        OPUS_ARM_MAY_HAVE_NEON
        OPUS_ARM_MAY_HAVE_NEON_INTR
        OPUS_ARM_PRESUME_NEON
        OPUS_ARM_PRESUME_NEON_INTR
    )
    list(APPEND opus_platform_includes "${opus_SOURCE_DIR}/silk/fixed")
endif()

include(CheckSymbolExists)
include(CMakePushCheckState)
cmake_push_check_state(RESET)
if(UNIX)
    set(CMAKE_REQUIRED_LIBRARIES m)
endif()
check_symbol_exists(lrint "math.h" OPUS_HAVE_LRINT)
check_symbol_exists(lrintf "math.h" OPUS_HAVE_LRINTF)
cmake_pop_check_state()

ly_add_target(
    NAME opus STATIC
    NAMESPACE 3rdParty
    FILES_CMAKE files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            "${CMAKE_CURRENT_BINARY_DIR}/Include"
            "${CMAKE_CURRENT_BINARY_DIR}/Include/opus"
        PRIVATE
            "${opus_SOURCE_DIR}"
            "${opus_SOURCE_DIR}/celt"
            "${opus_SOURCE_DIR}/dnn"
            "${opus_SOURCE_DIR}/include"
            "${opus_SOURCE_DIR}/silk"
            "${opus_SOURCE_DIR}/silk/float"
            ${opus_platform_includes}
    COMPILE_DEFINITIONS
        PRIVATE
            OPUS_BUILD
            PACKAGE_VERSION="${opus_version}"
            ENABLE_HARDENING
            DISABLE_DEBUG_FLOAT
            $<$<BOOL:${OPUS_HAVE_LRINT}>:HAVE_LRINT>
            $<$<BOOL:${OPUS_HAVE_LRINTF}>:HAVE_LRINTF>
            ${opus_platform_definitions}
)

set_target_properties(opus PROPERTIES
    C_STANDARD ${opus_c_standard}
    POSITION_INDEPENDENT_CODE ON
    SYSTEM TRUE
    UNITY_BUILD_BATCH_SIZE 8
)
add_library(Opus::opus ALIAS opus)

if(UNIX)
    ly_target_link_libraries(opus PRIVATE m)
endif()

ly_install(
    DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Include/opus"
    DESTINATION include
    COMPONENT CORE
)

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/opus
)
