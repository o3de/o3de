#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(lz4
    VERSION "1.11.1"
    LICENSE "BSD-2-Clause"
    URL "https://github.com/lz4/lz4/archive/refs/tags/v1.11.1.tar.gz"
    URL_HASH "683fed08a844ad012196381e40badf80c0bbe4962959761416d41c9e67d77ec7"
    GIT "https://github.com/lz4/lz4.git"
    GIT_HASH "aa4647fad575d01a874aaaadcdbe8e6a40e072f5"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(lz4)

file(COPY
    "${lz4_SOURCE_DIR}/lib/lz4.c"
    "${lz4_SOURCE_DIR}/lib/lz4.h"
    "${lz4_SOURCE_DIR}/lib/lz4frame.c"
    "${lz4_SOURCE_DIR}/lib/lz4frame.h"
    "${lz4_SOURCE_DIR}/lib/lz4hc.c"
    "${lz4_SOURCE_DIR}/lib/lz4hc.h"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Source"
)

file(COPY
    "${lz4_SOURCE_DIR}/lib/lz4.h"
    "${lz4_SOURCE_DIR}/lib/lz4frame.h"
    "${lz4_SOURCE_DIR}/lib/lz4hc.h"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

o3de_disable_warnings()

ly_add_target(
    NAME lz4 STATIC
    NAMESPACE 3rdParty
    NO_UNITY
    FILES_CMAKE files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

set_target_properties(lz4 PROPERTIES SYSTEM TRUE)
ly_target_link_libraries(lz4 PUBLIC 3rdParty::xxhash)

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/lz4
)
