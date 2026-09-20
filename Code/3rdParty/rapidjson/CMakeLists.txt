#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(rapidjson
    VERSION "1.1.0"
    LICENSE "MIT"
    URL "https://github.com/Tencent/rapidjson/archive/24b5e7a8b27f42fa16b96fc70aade9106cf7102f.tar.gz" # 2025-02-05
    URL_HASH "2d2601a82d2d3b7e143a3c8d43ef616671391034bc46891a9816b79cf2d3e7a8"
    GIT "https://github.com/Tencent/rapidjson.git"
    GIT_HASH "24b5e7a8b27f42fa16b96fc70aade9106cf7102f"
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
