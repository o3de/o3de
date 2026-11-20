#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(jsoncpp
    VERSION "1.6.2"
    LICENSE "Public Domain OR MIT"
    URL "https://github.com/open-source-parsers/jsoncpp/archive/refs/tags/1.6.2.tar.gz"
    URL_HASH "c40e5a2583df633b904525305bda40b622d9971fee1ece7ce48ca723a4907d36"
    GIT "https://github.com/open-source-parsers/jsoncpp.git"
    GIT_HASH "24682e37bf1425455b45e7db69dbdbcd30aec7dd"
    SOURCE_SUBDIR "no-add-subdir"
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
