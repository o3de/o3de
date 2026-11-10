#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(earcut
    VERSION "3.2.4"
    LICENSE "ISC"
    URL "https://github.com/mapbox/earcut.hpp/archive/92acda9e2ed96ff1f910cda35de444727e9da7b0.tar.gz"
    URL_HASH "799dc5d93478f74ca5331eaba935946efa25772f00dc7274005e07e296d469a8"
    GIT "https://github.com/mapbox/earcut.hpp.git"
    GIT_HASH "92acda9e2ed96ff1f910cda35de444727e9da7b0"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(earcut)

file(COPY
    "${earcut_SOURCE_DIR}/include/mapbox/earcut.hpp"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include/mapbox"
)

ly_add_target(
    NAME earcut HEADERONLY
    NAMESPACE 3rdParty
)

target_include_directories(earcut SYSTEM
    INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/earcut
)
