#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(sse2neon
    VERSION "v1.9.1"
    LICENSE "MIT"
    URL "https://github.com/DLTcollab/sse2neon/archive/refs/tags/v1.9.1.tar.gz"
    URL_HASH "6b70e7cb8c5ce4641002b85deaafe97efdf9ade9b49884edeaf678b35f0e132f"
    GIT "https://github.com/DLTcollab/sse2neon.git"
    GIT_HASH "92f6de174717aef09033ad21568d5bb9e5470404"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(sse2neon)

file(COPY
    "${sse2neon_SOURCE_DIR}/sse2neon.h"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

ly_add_target(
    NAME sse2neon HEADERONLY
    NAMESPACE 3rdParty
    INCLUDE_DIRECTORIES
        INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

set_target_properties(sse2neon PROPERTIES SYSTEM TRUE)

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/sse2neon
)
