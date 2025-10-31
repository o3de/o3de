/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_LUA
#define AZCORE_LUA

extern "C" {
#   include <Lua/lua.h>
#   include <Lua/lualib.h>
#   include <Lua/lauxlib.h>
}

// Currently we support Lua 5.4.4 and later
// note that Lua 5.x defines LUA_RID_LAST + 1 to be an index of a free-list.
#define AZ_LUA_SCRIPT_CONTEXT_REF       LUA_RIDX_LAST + 2
#define AZ_LUA_GLOBALS_TABLE_REF        LUA_RIDX_LAST + 3
#define AZ_LUA_CLASS_TABLE_REF          LUA_RIDX_LAST + 4
#define AZ_LUA_WEAK_CACHE_TABLE_REF     LUA_RIDX_LAST + 5
#define AZ_LUA_ERROR_HANDLER_FUN_REF    LUA_RIDX_LAST + 6

#define AZ_LUA_CLASS_METATABLE_NAME_INDEX 1 // can we always read the name from the behavior class???
#define AZ_LUA_CLASS_METATABLE_BEHAVIOR_CLASS 2
#define AZ_LUA_CLASS_METATABLE_INSTANCES 3
#define AZ_LUA_CLASS_METATABLE_STORAGE_TYPE_INDEX 4
#define AZ_LUA_CLASS_METATABLE_TYPE_INDEX 5
#define AZ_LUA_CLASS_METATABLE_STORAGE_CREATOR_INDEX 6
#define AZ_LUA_CLASS_METATABLE_WRAPPED_TYPEID 7
#define AZ_LUA_CLASS_METATABLE_VALUE_CLONE 8

#endif // AZCORE_LUA
