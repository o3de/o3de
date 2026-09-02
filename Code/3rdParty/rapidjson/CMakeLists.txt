#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(rapidjson
    VERSION "1.1.0"
    LICENSE "MIT"
    URL "https://github.com/Tencent/rapidjson/archive/ebcbd04484fcdaddbb9fd7798e76bbfb4ae8f840.tar.gz" # 2020-03-30
    URL_HASH "9af16b73313b4b635c13c2fd51cb432ac5573012f933ae53e5aa9125e15ac59b"
    GIT "https://github.com/Tencent/rapidjson.git"
    GIT_HASH "ebcbd04484fcdaddbb9fd7798e76bbfb4ae8f840"
    SOURCE_SUBDIR "no-add-subdir"
)
FetchContent_MakeAvailable(rapidjson)

file(
    COPY "${rapidjson_SOURCE_DIR}/include/rapidjson"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

ly_add_target(
    NAME rapidjson HEADERONLY
    NAMESPACE 3rdParty
)

target_include_directories(rapidjson SYSTEM
    INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

ly_create_alias(NAME RapidJSON NAMESPACE 3rdParty TARGETS 3rdParty::rapidjson)

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/rapidjson
)
