#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(zip
    VERSION "0.3.15"
    LICENSE "Unlicense AND MIT"
    URL "https://github.com/kuba--/zip/archive/f57d98fbaa774c73de650fde16a9b5f8a92743ea.tar.gz"
    URL_HASH "196c62409ab47fdf42d398dea389f1df03a43e38fb63052131c790ac29b4affe"
    GIT "https://github.com/kuba--/zip.git"
    GIT_HASH "f57d98fbaa774c73de650fde16a9b5f8a92743ea"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(zip)

file(COPY
    "${zip_SOURCE_DIR}/src/zip.h"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include/zip"
)

o3de_disable_warnings()

ly_add_target(
    NAME zip STATIC
    NAMESPACE 3rdParty
    NO_UNITY
    FILES_CMAKE files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/Include"
    COMPILE_DEFINITIONS
        PRIVATE
            MINIZ_USE_UNALIGNED_LOADS_AND_STORES=0
            $<$<BOOL:${UNIX}>:ZIP_HAVE_SYMLINK=1>
)

set_target_properties(zip PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    SYSTEM TRUE
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
)

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/zip
)
