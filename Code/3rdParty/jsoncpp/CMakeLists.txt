#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(jsoncpp
    VERSION "1.9.8"
    LICENSE "Public Domain OR MIT"
    URL "https://github.com/open-source-parsers/jsoncpp/archive/refs/tags/1.9.8.tar.gz"
    URL_HASH "51828cf3574281d2b79ec2a1c56a9e4c20cc1103711321ea96384cffb8d2d904"
    GIT "https://github.com/open-source-parsers/jsoncpp.git"
    GIT_HASH "8519b8381f3c741ad1421f88237b1deda0b11412"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(jsoncpp)

file(COPY "${jsoncpp_SOURCE_DIR}/include/json" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include")

o3de_disable_warnings()

ly_add_target(
    NAME jsoncpp STATIC
    NAMESPACE 3rdParty
    FILES_CMAKE files.cmake
)

target_include_directories(jsoncpp SYSTEM PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/Include")
target_compile_options(jsoncpp PRIVATE ${O3DE_COMPILE_OPTION_ENABLE_EXCEPTIONS})

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/jsoncpp
)
