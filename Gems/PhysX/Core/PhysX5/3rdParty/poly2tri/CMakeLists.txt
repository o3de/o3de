#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(poly2tri
    VERSION "4732e46"
    LICENSE "BSD-3-Clause"
    URL "https://github.com/jhasse/poly2tri/archive/4732e46da88086c6b7b3935a5b4f6f87360062c3.tar.gz"
    URL_HASH "65df56280d51aec49db0234e26e8b96d7c349324c667a156355d30c9bc0969f8"
    GIT "https://github.com/jhasse/poly2tri.git"
    GIT_HASH "4732e46da88086c6b7b3935a5b4f6f87360062c3"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(poly2tri)

file(COPY "${poly2tri_SOURCE_DIR}/poly2tri/"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include"
    FILES_MATCHING PATTERN "*.h"
)

o3de_disable_warnings()

ly_add_target(
    NAME poly2tri STATIC
    NAMESPACE 3rdParty
    NO_UNITY
    FILES_CMAKE files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/Include"
    COMPILE_DEFINITIONS
        PUBLIC P2T_STATIC_EXPORTS
)

target_compile_options(poly2tri ${O3DE_COMPILE_OPTION_ENABLE_EXCEPTIONS})

set_target_properties(poly2tri PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    SYSTEM TRUE
)

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Gems/PhysX/Core/PhysX5/3rdParty/poly2tri
)
