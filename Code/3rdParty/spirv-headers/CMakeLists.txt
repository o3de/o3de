#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(spirv-headers
    VERSION "1.4.363.0"
    LICENSE "MIT"
    URL "https://github.com/KhronosGroup/SPIRV-Headers/archive/refs/tags/vulkan-sdk-1.4.363.0.tar.gz"
    URL_HASH "5b3a0d099f5c7649e29c0938476c462e71d66ad89a0bde42fa1ba3eb476956c9"
    GIT "https://github.com/KhronosGroup/SPIRV-Headers.git"
    GIT_HASH "496543121ce6419f23d6fa5d7194ba66c36212d2"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(spirv-headers)

file(
    COPY "${spirv-headers_SOURCE_DIR}/include/"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

ly_add_target(
    NAME spirv-headers HEADERONLY
    NAMESPACE 3rdParty
    INCLUDE_DIRECTORIES
        INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

set_target_properties(spirv-headers PROPERTIES SYSTEM TRUE)

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/spirv-headers
)
