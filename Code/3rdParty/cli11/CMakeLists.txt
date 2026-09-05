#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(cli11
    VERSION "2.6.2"
    LICENSE "BSD-3-Clause"
    URL "https://github.com/CLIUtils/CLI11/archive/refs/tags/v2.6.2.tar.gz"
    URL_HASH "c6ea6b2e5608b3ea8617999bd5f47420c71b2ebdb8dc4767c1034d1da5785711"
    GIT "https://github.com/CLIUtils/CLI11.git"
    GIT_HASH "37bb6edc5317e99af72ef48405e65d9ca5218861"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(cli11)

file(COPY "${cli11_SOURCE_DIR}/include/CLI" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include")

ly_add_target(
    NAME cli11 HEADERONLY
    NAMESPACE 3rdParty
)

target_include_directories(cli11 SYSTEM INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/Include")

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/cli11
)
