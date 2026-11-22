#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(zlib
    VERSION "1.4"
    LICENSE "Zlib"
    URL "https://github.com/madler/zlib/archive/refs/tags/v1.4.tar.gz"
    URL_HASH "4331355048e286908260fdbeaa0b8cf75ceaf2907c75855a093a6487c7032aaa"
    GIT "https://github.com/madler/zlib.git"
    GIT_HASH "99623ef7097352da7993ba614161fd6a311e046e"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(zlib)

file(COPY
    "${zlib_SOURCE_DIR}/zlib.h"
    "${zlib_SOURCE_DIR}/zconf.h"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

include(CheckIncludeFile)
check_include_file(unistd.h ZLIB_HAVE_UNISTD_H)

o3de_disable_warnings()

ly_add_target(
    NAME zlib STATIC
    NAMESPACE 3rdParty
    NO_UNITY
    FILES_CMAKE files.cmake
    OUTPUT_NAME z
    INCLUDE_DIRECTORIES
        PRIVATE "${zlib_SOURCE_DIR}"
        PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/Include"
    COMPILE_DEFINITIONS
        PRIVATE ZLIB_BUILD
        PUBLIC $<$<BOOL:${ZLIB_HAVE_UNISTD_H}>:Z_HAVE_UNISTD_H>
)

add_library(ZLIB::ZLIB ALIAS zlib)

set_target_properties(zlib PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    SYSTEM TRUE
)

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/zlib
)
