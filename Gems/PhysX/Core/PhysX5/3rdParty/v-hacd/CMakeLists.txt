#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(v-hacd
    VERSION "v4.1.0"
    LICENSE "BSD-3-Clause"
    URL "https://github.com/kmammou/v-hacd/archive/refs/tags/v4.1.0.tar.gz"
    URL_HASH "9fe895cd10ec995d2171b11bde97aaaa221b418a3aaed0f5d9a068ae057d626b"
    GIT "https://github.com/kmammou/v-hacd.git"
    GIT_HASH "22ec20a7f8ea221ab600df869369b9c6a258cb10"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(v-hacd)

file(
    COPY "${v-hacd_SOURCE_DIR}/include/VHACD.h"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

ly_add_target(
    NAME v-hacd HEADERONLY
    NAMESPACE 3rdParty
    FILES_CMAKE files.cmake
)

target_include_directories(v-hacd SYSTEM INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/Include")

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Gems/PhysX/Core/PhysX5/3rdParty/v-hacd
)
