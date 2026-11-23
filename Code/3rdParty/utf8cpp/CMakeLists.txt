#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(utf8cpp
    VERSION "4.2.1"
    LICENSE "BSL-1.0"
    URL "https://github.com/nemtrif/utfcpp/archive/3d1ff6dbf54e471c846dff55dda6c8ee10c0ee5c.tar.gz"
    URL_HASH "b118efb8f08732906e0063c2f95610ad9c1179bb8819e5aa5d46efa56368bfc3"
    GIT "https://github.com/nemtrif/utfcpp.git"
    GIT_HASH "3d1ff6dbf54e471c846dff55dda6c8ee10c0ee5c"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(utf8cpp)

file(COPY
    "${utf8cpp_SOURCE_DIR}/source/utf8.h"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include"
)
file(COPY
    "${utf8cpp_SOURCE_DIR}/source/utf8/checked.h"
    "${utf8cpp_SOURCE_DIR}/source/utf8/core.h"
    "${utf8cpp_SOURCE_DIR}/source/utf8/cpp11.h"
    "${utf8cpp_SOURCE_DIR}/source/utf8/cpp17.h"
    "${utf8cpp_SOURCE_DIR}/source/utf8/cpp20.h"
    "${utf8cpp_SOURCE_DIR}/source/utf8/unchecked.h"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include/utf8"
)

ly_add_target(
    NAME utf8cpp HEADERONLY
    NAMESPACE 3rdParty
)

target_include_directories(utf8cpp SYSTEM
    INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/utf8cpp
)
