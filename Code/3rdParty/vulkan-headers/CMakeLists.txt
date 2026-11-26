#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(vulkan-headers
    VERSION "1.4.368"
    LICENSE "Apache-2.0 OR MIT"
    URL "https://github.com/KhronosGroup/Vulkan-Headers/archive/refs/tags/v1.4.368.tar.gz"
    URL_HASH "5c5c6f64fcce28a24cc934143a048707af036a858e4ed11d7f71fe016006ecda"
    GIT "https://github.com/KhronosGroup/Vulkan-Headers.git"
    GIT_HASH "88487f687f3100ebe1d3559a8a34d971ee85b977"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(vulkan-headers)

file(
    COPY "${vulkan-headers_SOURCE_DIR}/include/"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

ly_add_target(
    NAME vulkan-headers HEADERONLY
    NAMESPACE 3rdParty
    INCLUDE_DIRECTORIES
        INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

set_target_properties(vulkan-headers PROPERTIES SYSTEM TRUE)

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/vulkan-headers
)
