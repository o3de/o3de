#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(xxhash
    VERSION "0.8.3"
    LICENSE "BSD-2-Clause"
    URL "https://github.com/Cyan4973/xxHash/archive/refs/tags/v0.8.3.tar.gz"
    URL_HASH "aae608dfe8213dfd05d909a57718ef82f30722c392344583d3f39050c7f29a80"
    GIT "https://github.com/Cyan4973/xxHash.git"
    GIT_HASH "e626a72bc2321cd320e953a0ccf1584cad60f363"
    SOURCE_SUBDIR "no-add-subdir"
)
FetchContent_MakeAvailable(xxhash)

file(
    COPY "${xxhash_SOURCE_DIR}/xxhash.h"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

o3de_disable_warnings()

ly_add_target(
    NAME xxhash STATIC
    NAMESPACE 3rdParty
    NO_UNITY
    FILES_CMAKE files.cmake
)

ly_target_include_system_directories(
    TARGET xxhash
    PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/xxhash
)
