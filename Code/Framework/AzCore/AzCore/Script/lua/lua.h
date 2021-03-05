/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#ifndef AZCORE_LUA
#define AZCORE_LUA

extern "C" {
#   include <Lua/lua.h>
#   include <Lua/lualib.h>
#   include <Lua/lauxlib.h>
}

// Currently we support Lua 5.1 and later (we have tested with 5.2)

#if LUA_VERSION_NUM <= 502

inline void lua_pushunsigned(lua_State* l, unsigned int v)
{
    lua_pushnumber(l, static_cast<lua_Number>(v));
}

inline unsigned int lua_tounsigned(lua_State* l, int idx)
{
    return static_cast<unsigned int>(lua_tonumber(l, idx));
}

#define lua_pushglobaltable(L) lua_pushvalue(L, LUA_GLOBALSINDEX)

inline LUA_API int lua_load(lua_State* L, lua_Reader reader, void* data, const char* chunkname, const char* mode)
{
    (void)mode;
    return lua_load(L, reader, data, chunkname);
}

#define LUA_RIDX_LAST 0
#define LUA_NUMTAGS 9

#endif

#define AZ_LUA_SCRIPT_CONTEXT_REF       LUA_RIDX_LAST + 1
#define AZ_LUA_GLOBALS_TABLE_REF        LUA_RIDX_LAST + 2
#define AZ_LUA_CLASS_TABLE_REF          LUA_RIDX_LAST + 3
#define AZ_LUA_WEAK_CACHE_TABLE_REF     LUA_RIDX_LAST + 4
#define AZ_LUA_ERROR_HANDLER_FUN_REF    LUA_RIDX_LAST + 5

#define AZ_LUA_CLASS_METATABLE_NAME_INDEX 1 // can we always read the name from the behavior class???
#define AZ_LUA_CLASS_METATABLE_BEHAVIOR_CLASS 2
#define AZ_LUA_CLASS_METATABLE_INSTANCES 3
#define AZ_LUA_CLASS_METATABLE_STORAGE_TYPE_INDEX 4
#define AZ_LUA_CLASS_METATABLE_TYPE_INDEX 5
#define AZ_LUA_CLASS_METATABLE_STORAGE_CREATOR_INDEX 6
#define AZ_LUA_CLASS_METATABLE_WRAPPED_TYPEID 7
#define AZ_LUA_CLASS_METATABLE_VALUE_CLONE 8

#endif // AZCORE_LUA
