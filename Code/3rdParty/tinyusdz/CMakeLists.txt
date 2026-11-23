#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(tinyusdz
    VERSION "6050eef"
    LICENSE "Apache-2.0"
    URL "https://github.com/lighttransport/tinyusdz/archive/6050eef932f7d2788656d63297aa488fb0961ed1.tar.gz"
    URL_HASH "5a9aa6702a1d922d9ef12f52ea7a7a4f11425fb03e210dd0fdd2c3a000d56e4b"
    GIT "https://github.com/lighttransport/tinyusdz.git"
    GIT_HASH "6050eef932f7d2788656d63297aa488fb0961ed1"
    PATCH_FILES "${CMAKE_CURRENT_LIST_DIR}/tinyusdz.patch"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(tinyusdz)

file(COPY
    "${tinyusdz_SOURCE_DIR}/src/"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include"
    FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hh"
        PATTERN "*.hpp"
        PATTERN "*.inc"
        PATTERN "attic" EXCLUDE
)

o3de_disable_warnings()

ly_add_target(
    NAME tinyusdz STATIC
    NAMESPACE 3rdParty
    NO_UNITY
    FILES_CMAKE files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE "${tinyusdz_SOURCE_DIR}/src"
        PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/Include"
    COMPILE_DEFINITIONS
        PRIVATE
            FPNG_NO_SSE=1
            TINYUSDZ_USE_USDMTLX
            TINYUSDZ_USE_USDOBJ
            TINYUSDZ_USE_USDVOX
            TINYUSDZ_WITH_AUDIO
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::lz4
            3rdParty::pugixml
)

set_target_properties(tinyusdz PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    SYSTEM TRUE
)

target_compile_options(tinyusdz PRIVATE ${O3DE_COMPILE_OPTION_ENABLE_EXCEPTIONS})

ly_install(
    DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Include/"
    DESTINATION Code/3rdParty/tinyusdz/Include
    COMPONENT ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}
    FILES_MATCHING
        PATTERN "*.hh"
        PATTERN "*.inc"
)

ly_install(
    DIRECTORY "${tinyusdz_SOURCE_DIR}/src/"
    DESTINATION Code/3rdParty/tinyusdz/Notices
    COMPONENT ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}
    FILES_MATCHING
        PATTERN "*LICENSE*"
        PATTERN "*COPYING*"
)

ly_install_files(
    FILES
        LICENSE.txt
        NOTICES.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/tinyusdz
)
