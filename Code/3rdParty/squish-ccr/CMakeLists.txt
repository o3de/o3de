#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(squish-ccr
    VERSION "2.00-alpha2"
    LICENSE "MIT"
    URL "https://github.com/Ethatron/squish-ccr/archive/deb557d2fa647b191b37a2d8682df54ec8a7cfba.tar.gz"
    URL_HASH "5b578a95b339a99af3fc31116b159118c2fad8568388f824bbdfacba00afa7f3"
    GIT "https://github.com/Ethatron/squish-ccr.git"
    GIT_HASH "deb557d2fa647b191b37a2d8682df54ec8a7cfba"
    PATCH_FILES "${CMAKE_CURRENT_LIST_DIR}/squish-ccr.patch"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(squish-ccr)

file(COPY
    "${squish-ccr_SOURCE_DIR}/coloursinglelookup_ccr_vector.inl"
    "${squish-ccr_SOURCE_DIR}/coloursinglelookup_ccr.inl"
    "${squish-ccr_SOURCE_DIR}/config.h"
    "${squish-ccr_SOURCE_DIR}/degeneracy_ccr.inl"
    "${squish-ccr_SOURCE_DIR}/squish.h"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include/squish-ccr"
)

o3de_disable_warnings()

ly_add_target(
    NAME squish-ccr STATIC
    NAMESPACE 3rdParty
    NO_UNITY
    FILES_CMAKE files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE "${squish-ccr_SOURCE_DIR}"
        PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/Include"
    COMPILE_DEFINITIONS
        PUBLIC
            SQUISH_USE_CCR
            SQUISH_USE_CPP
            SQUISH_USE_SSE=2
            USING_SQUISH_SDK
)

ly_target_link_libraries(squish-ccr PRIVATE 3rdParty::sse2neon)

set_target_properties(squish-ccr PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    SYSTEM TRUE
)

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/squish-ccr
)
