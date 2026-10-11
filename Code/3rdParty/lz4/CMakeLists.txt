#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(lz4
    VERSION "1.9.4"
    LICENSE "BSD-2-Clause"
    URL "https://github.com/lz4/lz4/archive/refs/tags/v1.9.4.tar.gz"
    URL_HASH "0b0e3aa07c8c063ddf40b082bdf7e37a1562bda40a0ff5272957f3e987e0e54b"
    GIT "https://github.com/lz4/lz4.git"
    GIT_HASH "5ff839680134437dbf4678f3d0c7b371d84f4964"
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
