#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(meshoptimizer
    VERSION "v1.3"
    LICENSE "MIT"
    URL "https://github.com/zeux/meshoptimizer/archive/refs/tags/v1.3.tar.gz"
    URL_HASH "ca4f47d451201593904b9e3067fa54b65db94bc34ab5173864950862b748a145"
    GIT "https://github.com/zeux/meshoptimizer.git"
    GIT_HASH "9e1f07b159d3cb777f1c67ed31fc11fd117986f4"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(meshoptimizer)

file(
    COPY "${meshoptimizer_SOURCE_DIR}/src/meshoptimizer.h"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

o3de_disable_warnings()

ly_add_target(
    NAME meshoptimizer STATIC
    NAMESPACE 3rdParty
    NO_UNITY
    FILES_CMAKE files.cmake
)

target_include_directories(meshoptimizer SYSTEM PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/Include")

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/meshoptimizer
)
