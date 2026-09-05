#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(meshoptimizer
    VERSION "v1.2"
    LICENSE "MIT"
    URL "https://github.com/zeux/meshoptimizer/archive/refs/tags/v1.2.tar.gz"
    URL_HASH "e40f71b809cdf3361b9a4def85fd44534e8733ce29d4b943c145b76859e4c2b4"
    GIT "https://github.com/zeux/meshoptimizer.git"
    GIT_HASH "9d9890c73011d75920af614485296d1e03e95448"
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
