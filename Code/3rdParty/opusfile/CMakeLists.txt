#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(opusfile
    VERSION "0.12"
    LICENSE "BSD-3-Clause"
    URL "https://downloads.xiph.org/releases/opus/opusfile-0.12.tar.gz"
    URL_HASH "118d8601c12dd6a44f52423e68ca9083cc9f2bfe72da7a8c1acb22a80ae3550b"
    GIT "https://github.com/xiph/opusfile.git"
    GIT_HASH "a55c164e9891a9326188b7d4d216ec9a88373739"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(opusfile)

file(COPY
    "${opusfile_SOURCE_DIR}/include/opusfile.h"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include/opus"
)

include(CheckSymbolExists)
include(CMakePushCheckState)
cmake_push_check_state(RESET)
if(UNIX)
    set(CMAKE_REQUIRED_LIBRARIES m)
endif()
check_symbol_exists(lrintf "math.h" OPUSFILE_HAVE_LRINTF)
cmake_pop_check_state()

o3de_disable_warnings()

ly_add_target(
    NAME opusfile STATIC
    NAMESPACE 3rdParty
    FILES_CMAKE files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            "${CMAKE_CURRENT_BINARY_DIR}/Include"
            "${CMAKE_CURRENT_BINARY_DIR}/Include/opus"
    COMPILE_DEFINITIONS
        PRIVATE $<$<BOOL:${OPUSFILE_HAVE_LRINTF}>:OP_HAVE_LRINTF>
    BUILD_DEPENDENCIES
        PUBLIC
            3rdParty::ogg
            3rdParty::opus
)

set_target_properties(opusfile PROPERTIES
    C_STANDARD 99
    POSITION_INDEPENDENT_CODE ON
    SYSTEM TRUE
)

if(UNIX)
    ly_target_link_libraries(opusfile PRIVATE m)
endif()

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/opusfile
)
