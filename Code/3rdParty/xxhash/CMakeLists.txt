#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(xxhash
    VERSION "0.8.4"
    LICENSE "BSD-2-Clause"
    URL "https://github.com/Cyan4973/xxHash/archive/refs/tags/v0.8.4.tar.gz"
    URL_HASH "5738270935e7c3d38a79b3adf7c9692566ce7895a25f67de43ad52ab504acd32"
    GIT "https://github.com/Cyan4973/xxHash.git"
    GIT_HASH "c87183a77d67f7d37e3d2d1b7eaac5e7c695e4f0"
    NO_ADD_SUBDIR
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

target_include_directories(xxhash SYSTEM
    PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/xxhash
)
