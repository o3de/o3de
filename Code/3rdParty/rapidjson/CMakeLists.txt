#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(rapidjson
    VERSION "1.1.0"
    LICENSE "MIT"
    URL "https://github.com/Tencent/rapidjson/archive/a895ce150fa2d73f4ee55f8657761a6b3e96236f.tar.gz" # 2020-02-13
    URL_HASH "91dfec64e5b3e63bec373d61af839430a403c74945a6dea7dce63def4434ffba"
    GIT "https://github.com/Tencent/rapidjson.git"
    GIT_HASH "a895ce150fa2d73f4ee55f8657761a6b3e96236f"
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
