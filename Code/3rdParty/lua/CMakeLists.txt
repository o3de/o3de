#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(lua
    VERSION "5.4.4"
    LICENSE "MIT"
    URL "https://github.com/lua/lua/archive/refs/tags/v5.4.4.tar.gz"
    URL_HASH "2ff89bbea22a7c8b910d6b006f5ba4559edcf468a25537cedf9988ed90f6b450"
    GIT "https://github.com/lua/lua.git"
    GIT_HASH "5d708c3f9cae12820e415d4f89c9eacbe2ab964b"
    PATCH_FILES "${CMAKE_CURRENT_LIST_DIR}/lua.patch"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(lua)

file(COPY
    "${lua_SOURCE_DIR}/lauxlib.h"
    "${lua_SOURCE_DIR}/lua.h"
    "${lua_SOURCE_DIR}/luaconf.h"
    "${lua_SOURCE_DIR}/lualib.h"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include/Lua"
)

o3de_disable_warnings()

ly_add_target(
    NAME lua STATIC
    NAMESPACE 3rdParty
    NO_UNITY
    FILES_CMAKE files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE "${lua_SOURCE_DIR}"
        PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/Include"
    COMPILE_DEFINITIONS
        PRIVATE
            MAKE_LIB
            $<$<PLATFORM_ID:Darwin>:LUA_USE_MACOSX>
            $<$<PLATFORM_ID:Linux>:LUA_USE_LINUX>
            $<$<PLATFORM_ID:Windows>:_CRT_SECURE_NO_WARNINGS>
)

set_target_properties(lua PROPERTIES SYSTEM TRUE)

if(CMAKE_DL_LIBS)
    ly_target_link_libraries(lua PUBLIC ${CMAKE_DL_LIBS})
endif()
if(UNIX)
    ly_target_link_libraries(lua PUBLIC m)
endif()

ly_create_alias(NAME Lua NAMESPACE 3rdParty TARGETS 3rdParty::lua)

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/lua
)
