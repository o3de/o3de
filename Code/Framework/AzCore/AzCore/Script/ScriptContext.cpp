/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Memory/ChildAllocatorSchema.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/BehaviorContextUtilities.h>
#include <AzCore/Script/ScriptContextDebug.h>
#include <AzCore/Script/ScriptProperty.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/Script/lua/lua.h>
#include <AzCore/Serialization/Locale.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/RTTI/AttributeReader.h>

#include <AzCore/Script/ScriptPropertyTable.h>
#include <AzCore/StringFunc/StringFunc.h>

extern "C" {
#   include <Lua/lualib.h>
#   include <Lua/lauxlib.h>
#   include <Lua/lobject.h>

    // versions of LUA before 5.3.x used to define a union that contained a double, a pointer, and a long
    // as L_Umaxalign.  Newer versions define those inner types in the macro LUAI_MAXALIGN instead but
    // no longer actually declare a union around it.  For backward compatibility we define the same one here
    union L_Umaxalign { LUAI_MAXALIGN; };
}

#include <limits>
#include <cmath>

bool AZ::Internal::IsAvailableInLua(const AttributeArray& attributes)
{
    // Check for "ignore" attribute
    if (FindAttribute(Script::Attributes::Ignore, attributes))
    {
        return false;
    }

    // Only bind if marked with the Scope type of Launcher
    if (!AZ::Internal::IsInScope(attributes, Script::Attributes::ScopeFlags::Launcher))
    {
        return false;
    }

    return true;
}

#if defined(AZ_LUA_VALIDATE_STACK)

#if !defined(AZ_TESTS_ENABLED)
#pragma message("WARNING: Lua Stack Validation enabled. This is an expensive process, please ensure you want AZ_LUA_VALIDATE_STACK defined.")
#endif // AZ_TESTS_ENABLED

namespace AzLsvInternal
{
    // Struct used for keeping track of required return values
    LuaStackValidator::LuaStackValidator(lua_State* lua)
        : m_lua(lua)
    {
        m_startStackSize = lua_gettop(m_lua);
    }

    // Constructor for when return count is known ahead of time
    LuaStackValidator::LuaStackValidator(lua_State* lua, int expectedStackChange)
        : LuaStackValidator(lua)
    {
        SetExpectedStackChange(expectedStackChange);
    }

    // Used for setting return count later
    void LuaStackValidator::SetExpectedStackChange(int expectedReturnCount)
    {
        AZ_Assert(!m_returnCountSet, "Can only set ExpectedStackChange once!");

        m_expectedStackChange = expectedReturnCount;
        m_returnCountSet = true;
    }

    // Used on function exit to validate stack size
    LuaStackValidator::~LuaStackValidator()
    {
        AZ_Assert(m_returnCountSet, "Unchecked control flow detected!");

        int endStackSize = lua_gettop(m_lua);
        int stackSizeChange = endStackSize - m_startStackSize;

        AZ_Assert(stackSizeChange == m_expectedStackChange,
            "[LUA] STACK SIZE CHANGE INCONSISTENT. Beginning size: %d End size: %d change: %d expected change: %d",
            m_startStackSize, endStackSize, stackSizeChange, m_expectedStackChange);
    }
}
#endif // AZ_LUA_VALIDATE_STACK

namespace ScriptContextCpp
{
    // remove Lua packgage.loadlib immediately after loading libraries to prevent use of unsafe function
    void OpenLuaLibraries(lua_State* m_lua)
    {
        // LUA Must execute in the "C" Locale.
        AZ::Locale::ScopedSerializationLocale scopedLocale;

        // Lua:
        luaL_openlibs(m_lua);
        // Lua:
        lua_getglobal(m_lua, "package");
        // Lua: package
        AZ_Assert(lua_type(m_lua, -1) == LUA_TTABLE, "package was not a table");
        // Lua: package, package.loadlib
        lua_getfield(m_lua, -1, "loadlib");
        // Lua: package, package.loadlib
        AZ_Assert(lua_type(m_lua, -1) == LUA_TFUNCTION, "package.loadlib was not a function");
        lua_pop(m_lua, 1);
        // Lua: package
        lua_pushnil(m_lua);
        // Lua: package, nil
        lua_setfield(m_lua, -2, "loadlib"); // package.loadlib = nil
        // Lua: package
        lua_getfield(m_lua, -1, "loadlib");
        // Lua: package, package.loadlib
        AZ_Assert(lua_type(m_lua, -1) == LUA_TNIL, "package.loadlib was not nil");
        lua_pop(m_lua, -2);
        // Lua;
    }
}

namespace AZ
{
    using namespace ScriptContextCpp;

    struct ExposedLambda
    {
        AZ_TYPE_INFO(ExposedLambda, "{B702DB0B-516B-4807-8007-DC50A5CE180A}");
        AZ_CLASS_ALLOCATOR(ExposedLambda, AZ::SystemAllocator);

        // assumes a lambda is at the top of the stack and will pop it
        ExposedLambda(lua_State* lua)
            : m_lambdaRegistryIndex(luaL_ref_Checked(lua))
            , m_lua(lua)
        {
            lua_pushinteger(lua, 1);
            m_refCountRegistryIndex = luaL_ref_Checked(lua);
        }

        ExposedLambda(ExposedLambda&& source) noexcept
        {
            *this = AZStd::move(source);
        }

        ExposedLambda(const ExposedLambda& source)
        {
            *this = source;
        }

        ~ExposedLambda()
        {
            if (m_lua && DecrementRefCount() == 0)
            {
                luaL_unref(m_lua, LUA_REGISTRYINDEX, m_lambdaRegistryIndex);
                luaL_unref(m_lua, LUA_REGISTRYINDEX, m_refCountRegistryIndex);
            }
        }

        ExposedLambda& operator=(const ExposedLambda& source)
        {
            if (this != &source)
            {
                m_lambdaRegistryIndex = source.m_lambdaRegistryIndex;
                m_refCountRegistryIndex = source.m_refCountRegistryIndex;
                m_lua = source.m_lua;
                IncrementRefCount();
            }

            return *this;
        }

        ExposedLambda& operator=(ExposedLambda&& source) noexcept
        {
            if (this != &source)
            {
                m_lambdaRegistryIndex = source.m_lambdaRegistryIndex;
                m_refCountRegistryIndex = source.m_refCountRegistryIndex;
                source.m_lambdaRegistryIndex = source.m_refCountRegistryIndex = LUA_NOREF;
                m_lua = source.m_lua;
                source.m_lua = nullptr;
            }

            return *this;
        }

        void operator()([[maybe_unused]] AZ::BehaviorArgument*  resultBVP, AZ::BehaviorArgument* argsBVPs, int numArguments)
        {
            auto behaviorContext = AZ::ScriptContext::FromNativeContext(m_lua)->GetBoundContext();
            // Lua:
            lua_rawgeti(m_lua, LUA_REGISTRYINDEX, m_lambdaRegistryIndex);
            // Lua: lambda

            for (int i = 0; i < numArguments; ++i)
            {
                ExposedLambda::StackPush(m_lua, behaviorContext, argsBVPs[i]);
            }

            // Lua: lambda, args...
            Internal::LuaSafeCall(m_lua, numArguments, 0);
        }

        // \note Do not use, these are here for compiler compatibility only
        ExposedLambda()
            : m_lua(nullptr)
            , m_lambdaRegistryIndex(LUA_NOREF)
            , m_refCountRegistryIndex(LUA_NOREF)
        {}

    private:
        static int luaL_ref_Checked(lua_State* lua)
        {
            AZ_Assert(lua, "null lua_State");
            int ref = luaL_ref(lua, LUA_REGISTRYINDEX);
            AZ_Assert(ref != LUA_NOREF && ref != LUA_REFNIL, "invalid lambdaRegistryIndex");
            return ref;
        }

        template<typename T>
        static T* GetAs(AZ::BehaviorArgument& argument)
        {
            return argument.m_typeId == azrtti_typeid<T>()
                ? reinterpret_cast<T*>(argument.GetValueAddress())
                : nullptr;
        }

        static void StackPush(lua_State* lua, AZ::BehaviorContext* context, AZ::BehaviorArgument& argument)
        {
            if (auto cStringPtr = GetAs<const char*>(argument))
            {
                auto realValue = reinterpret_cast<const char*>(cStringPtr);
                lua_pushstring(lua, realValue);
            }
            else if (auto stringPtr = GetAs<AZStd::string>(argument))
            {
                lua_pushlstring(lua, stringPtr->data(), stringPtr->size());
            }
            else if (auto viewPtr = GetAs<AZStd::string_view>(argument))
            {
                lua_pushlstring(lua, viewPtr->data(), viewPtr->size());
            }
            else
            {
                AZ::StackPush(lua, context, argument);
            }
        }

        int m_lambdaRegistryIndex;
        int m_refCountRegistryIndex;
        lua_State* m_lua;

        int AddRefCount(int value)
        {
            AZ_Assert(value == 1 || value == -1, "ModRefCount is only for incrementing or decrementing on copy or destruction of ExposedLambda");
            lua_rawgeti(m_lua, LUA_REGISTRYINDEX, m_refCountRegistryIndex);
            // Lua: refCount-old
            const int refCount = Internal::azlua_tointeger(m_lua, -1) + value;
            lua_pushinteger(m_lua, m_refCountRegistryIndex);
            // Lua: refCount-old, registry index
            lua_pushinteger(m_lua, refCount);
            // Lua: refCount-old, registry index, refCount-new
            lua_rawset(m_lua, LUA_REGISTRYINDEX);
            // Lua: refCount-old
            lua_pop(m_lua, 1);
            // Lua:
            return refCount;
        }

        int DecrementRefCount()
        {
            return AddRefCount(-1);
        }

        void IncrementRefCount()
        {
            AddRefCount(1);
        }
    };

    constexpr const char* StorageTypeToString(Script::Attributes::StorageType storageType)
    {
        switch (storageType)
        {
        case AZ::Script::Attributes::StorageType::ScriptOwn:
            return "ScriptOwn ";
        case AZ::Script::Attributes::StorageType::RuntimeOwn:
            return "RuntimeOwn";
        case AZ::Script::Attributes::StorageType::Value:
            return "Value     ";
        default:
            return "<invalid> ";
        }
    }

    AZStd::string ToString(LuaUserData* userdata)
    {
        return AZStd::string::format("LuaUserData(%p)->(%p):%s", userdata, userdata->value, StorageTypeToString((Script::Attributes::StorageType)userdata->storageType));
    }

#if defined(LUA_USERDATA_TRACKING)
    void ScriptContext::AddAllocation(LuaUserData* userdata)
    {
        AZ_TracePrintf("Script", "++: %s", ToString(userdata).c_str());
        {
            auto valueIter = m_valueByUserData.find(userdata);
            if (valueIter != m_valueByUserData.end())
            {
                AZ_Warning("Script", false, "LuaUserData(%p)->(%p):%s is getting assigned another value: (%p). A memory leak will almost certainly follow.", userdata, userdata->value, StorageTypeToString(userdata->storageType));
            }

            m_valueByUserData[userdata] = userdata->value;
        }

        {
            auto userIter = m_userDataByValue.find(userdata->value);
            if (userIter != m_userDataByValue.end())
            {
                if (userIter->second->storageType == userdata->storageType)
                {
                    AZ_Warning("Script", false, "LuaUserData(%p)->(%p):%s. is stranglye SHARING WITH New: LuaUserData(%p)->(%p):%s."
                        , userIter->second
                        , userIter->second->value
                        , StorageTypeToString(userIter->second->storageType)
                        , userdata
                        , userdata->value
                        , StorageTypeToString(userdata->storageType));
                }
                else
                {
                    AZ_Warning("Script", false, "LuaUserData(%p)->(%p):%s. is BADLY SHARING WITH New: LuaUserData(%p)->(%p):%s."
                        , userIter->second
                        , userIter->second->value
                        , StorageTypeToString(userIter->second->storageType)
                        , userdata
                        , userdata->value
                        , StorageTypeToString(userdata->storageType));
                }
            }

            m_userDataByValue[userdata->value] = userdata;
        }
    }

    void ScriptContext::RemoveAllocation(LuaUserData* userdata)
    {
        {
            auto valueIter = m_valueByUserData.find(userdata);
            if (valueIter == m_valueByUserData.end())
            {
                AZ_Warning("Script", false, "LuaUserData(%p)->(%p):%s is removed but wasn't tracked by userdata", userdata, userdata->value, StorageTypeToString(userdata->storageType));
            }

            m_valueByUserData.erase(userdata);
        }

        {
            auto userIter = m_userDataByValue.find(userdata->value);
            if (userIter == m_userDataByValue.end())
            {
                AZ_Warning("Script", false, ", LuaUserData(%p)->(%p):%s is removed but wasn't tracked by value", userdata, userdata->value, StorageTypeToString(userdata->storageType));
            }

            m_userDataByValue.erase(userdata->value);
        }
    }
#endif//defined(LUA_USERDATA_TRACKING)

    class LuaCaller
    {
    public:

        virtual ~LuaCaller() {}

        virtual int ManualCall(lua_State* lua) = 0;

        virtual void PushClosure(lua_State* lua, const char* debugDescription) = 0;

        BehaviorMethod* m_method;
    };

    namespace Internal
    {
        // Flag on Lua user data to validate that it is AZ user data
        static const Crc32 AZLuaUserData = AZ_CRC_CE("AZLuaUserData");

        // Special indices in the table at AZ_LUA_WEAK_CACHE_TABLE_REF
        enum {
            WeakCacheMaxUsedIndex = 1,  // Index for the highest index of in-use data
            WeakCacheFreeListIndex = 2, // Index for the first element in the free list
            WeakCacheUser = 3,          // First index returnable to a user
        };

        ///////////////////////////////////////////////////////////////////////////////////////////////
        AZ_CHILD_ALLOCATOR_WITH_NAME(LuaSystemAllocator, "LuaSystemAllocator", "{7BEFB496-76EC-43DB-AB82-5ABA524FEF7F}", AZ::SystemAllocator);

        //=========================================================================
        // azlua_setglobal - raw setglobal function (no metamethods called)
        // [4/1/2012]
        //=========================================================================
        void azlua_setglobal(lua_State* _lua, const char* _name)
        {
            lua_pushglobaltable(_lua);          // push global table on the stack
            lua_pushstring(_lua, _name);         // push the name of the variable
            lua_pushvalue(_lua, -3);             // push the value (on the stack -1) before the call (the table we pushed)
            lua_rawset(_lua, -3);                // set the value in the table without calling any meta-methods
            lua_pop(_lua, 2);                    // pop the the table and the value from the stack
        }

        //=========================================================================
        // azlua_getglobal - raw getglobal function (no metamethods called)
        // [4/1/2012]
        //=========================================================================
        void azlua_getglobal(lua_State* _lua, const char* _name)
        {
            lua_pushglobaltable(_lua);  // push global table on the stack
            lua_pushstring(_lua, _name); // push the name of the variable on the stack
            lua_rawget(_lua, -2);        // get _G[name] and replace the name of the stack
            lua_replace(_lua, -2);       // replace the table we pushed with the result of get so this is only value that's left on the stack.
        }

        //////////////////////////////////////////////////////////////////////////
        void azlua_pushinteger(lua_State* lua, int value)
        {
            lua_pushinteger(lua, value);
        }

        //////////////////////////////////////////////////////////////////////////
        int azlua_tointeger(lua_State* lua, int stackIndex)
        {
            return static_cast<int>(lua_tointeger(lua, stackIndex));
        }

        //////////////////////////////////////////////////////////////////////////
        void azlua_pushtypeid(lua_State* lua, const AZ::Uuid& typeId)
        {
            lua_pushlightuserdata(lua, (void*)typeId.GetHash());
        }

        bool azlua_istable(lua_State* lua, int stackIndex)
        {
            return lua_istable(lua, stackIndex);
        }

        bool azlua_isfunction(lua_State* lua, int stackIndex)
        {
            return lua_isfunction(lua, stackIndex);
        }

        //////////////////////////////////////////////////////////////////////////
        bool azlua_isnil(lua_State* lua, int stackIndex)
        {
            return lua_isnil(lua, stackIndex);
        }

        //////////////////////////////////////////////////////////////////////////
        void azlua_pop(lua_State* lua, int numElements)
        {
            lua_pop(lua, numElements);
        }

        //////////////////////////////////////////////////////////////////////////
        void azlua_getfield(lua_State* lua, int stackIndex, const char* name)
        {
            lua_getfield(lua, stackIndex, name);
        }

        int azlua_gettop(lua_State* lua)
        {
            return lua_gettop(lua);
        }

        //////////////////////////////////////////////////////////////////////////
        void azlua_pushstring(lua_State* lua, const char* string)
        {
            lua_pushstring(lua, string);
        }

        void azlua_pushvalue(lua_State* lua, int stackIndex)
        {
            lua_pushvalue(lua, stackIndex);
        }

        //////////////////////////////////////////////////////////////////////////
        const char* azlua_tostring(lua_State* lua, int stackIndex)
        {
            return lua_tostring(lua, stackIndex);
        }

        //////////////////////////////////////////////////////////////////////////
        void azlua_rawset(lua_State* lua, int stackIndex)
        {
            lua_rawset(lua, stackIndex);
        }

        //////////////////////////////////////////////////////////////////////////
        void LuaRegistrySet(lua_State* lua, int valueIndex)
        {
            lua_rawseti(lua, LUA_REGISTRYINDEX, valueIndex);
        }

        //////////////////////////////////////////////////////////////////////////
        void LuaRegistryGet(lua_State* lua, int valueIndex)
        {
            lua_rawgeti(lua, LUA_REGISTRYINDEX, valueIndex);
        }

        //////////////////////////////////////////////////////////////////////////
        void LuaScriptValueStackPush(lua_State* lua, void* dataPointer, const AZ::Uuid typeId, AZ::ObjectToLua toLua)
        {
            if (!typeId.IsNull())
            {
                Internal::LuaClassToStack(lua, dataPointer, typeId, toLua);
            }
            else
            {
                lua_pushlightuserdata(lua, dataPointer);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void* LuaScriptValueStackRead(lua_State* lua, int stackIndex, const AZ::Uuid typeId)
        {
            if (typeId.IsNull() || lua_islightuserdata(lua, stackIndex))
            {
                return lua_touserdata(lua, stackIndex);
            }
            else
            {
                return Internal::LuaClassFromStack(lua, stackIndex, typeId);
            }
        }

        //=========================================================================
        // LuaPropertyTagHelper
        // [3/22/2012]
        //=========================================================================
        int LuaPropertyTagHelper(lua_State* l)
        {
            (void)l;
            AZ_Assert(false, "This function should never be called, it's used as a tag to store user data");
            return 0;
        }

        //=========================================================================
        // LuaMethodTagHelper
        // [9/25/2012]
        //=========================================================================
        int LuaMethodTagHelper(lua_State* l)
        {
            (void)l;
            AZ_Assert(false, "This function should never be called, it's used as a tag to store user data");
            return 0;
        }

        //=========================================================================
        // LuaSafeCallErrorHandler
        //=========================================================================
        static int LuaSafeCallErrorHandler(lua_State* lua)
        {
            LSV_BEGIN(lua, -1);

            if (lua_isstring(lua, -1))
            {
                ScriptContext::FromNativeContext(lua)->Error(ScriptContext::ErrorType::Error, true, "%s", lua_tostring(lua, -1));
            }
            else
            {
                AZ_Warning("Script", false, "First argument to LuaSafeCallErrorHandler must be a string, not %s!", luaL_typename(lua, -1));
            }

            // Pop the error message, return that there are no results
            lua_pop(lua, 1);
            return 0;
        }

        //=========================================================================
        // LuaSafeCall
        //=========================================================================
        bool LuaSafeCall(lua_State* lua, int numParams, int numResults)
        {
            // Lua must execute in the "C" Locale.  this does mean that the scoped locale will
            // bleed into any C++ calls that the pcall actually calls back into, but it is also the assumption
            // that any C++ calls that lua makes are also in the "C" locale.
            AZ::Locale::ScopedSerializationLocale scopedLocale;

            // Can't do LSV here because of multiple return values

            // Index the error handler should end up in
            // Should be the absolute index of the function to be called (that will be moved up)
            int errorIdx = lua_gettop(lua) - numParams;

            lua_rawgeti(lua, LUA_REGISTRYINDEX, AZ_LUA_ERROR_HANDLER_FUN_REF); // Get the error handler
            lua_insert(lua, errorIdx); // Push the function below the arguments and function

            bool success = 0 == lua_pcall(lua, numParams, numResults, errorIdx); // Call the function
            lua_remove(lua, errorIdx); // Remove the error function

            // If the function errors, the function is still on the stack and should be popped.
            if (!success)
            {
                lua_pop(lua, 1);
            }

            return success;
        }

        //=========================================================================
        // LuaEnumValueRead
        // [3/30/2012]
        //=========================================================================
        int LuaEnumValueRead(lua_State* l)
        {
            lua_pushvalue(l, lua_upvalueindex(1));
            return 1;
        }

        //=========================================================================
        // Class__Index
        // [3/22/2012]
        //=========================================================================
        int Class__Index(lua_State* l)
        {
            AZ::Locale::ScopedSerializationLocale scopedLocale;
            LSV_BEGIN(l, 1);

            // calling format __index(table,key)
            lua_getmetatable(l, -2); // load the userdata metatable
            int metaTableIndex = lua_gettop(l);

            // Check if the key is string, if so we expect it to be a function or property name
            // otherwise we allow users to provide custom index handlers
            // Technically we can allow strings too, but it will clash with function/property names and it be hard to figure
            // out what is going on from with in the system.
            if (lua_type(l, -2) == LUA_TSTRING)
            {
                lua_pushvalue(l, -2); // duplicate the key
                lua_rawget(l, -2); // load the value at this index
            }
            else
            {
                lua_pushliteral(l, "__AZ_Index");
                lua_rawget(l, -2); // check if the user provided custom Index method in the class metatable
                if (lua_isnil(l, -1)) // if not report an error
                {
                    lua_rawgeti(l, -2, AZ_LUA_CLASS_METATABLE_NAME_INDEX); // load the class name for a better error
                    if (!lua_isstring(l, -1))   // if we failed it means we are the base metatable
                    {
                        lua_pop(l, 1);
                        lua_rawgeti(l, 1, AZ_LUA_CLASS_METATABLE_NAME_INDEX);
                    }
                    ScriptContext::FromNativeContext(l)->Error(ScriptContext::ErrorType::Warning, true, "Invalid index type [], should be string! '%s:%s'!", lua_tostring(l, -1), lua_tostring(l, -4));
                }
                else
                {
                    // if we have custom index handler
                    lua_pushvalue(l, -4); // duplicate the table (class pointer)
                    lua_pushvalue(l, -4); // duplicate the index value for the call
                    lua_call(l, 2, 1); // call the function
                }

                lua_remove(l, metaTableIndex); // remove the metatable
                return 1;
            }

            if (lua_isnil(l, -1))
            {
                lua_rawgeti(l, -2, AZ_LUA_CLASS_METATABLE_NAME_INDEX); // load the class name for a better error
                if (!lua_isstring(l, -1))   // if we failed it means we are the base metatable
                {
                    lua_pop(l, 1);
                    lua_rawgeti(l, 1, AZ_LUA_CLASS_METATABLE_NAME_INDEX);
                }

                // we did not found the element in the table trigger an error
                ScriptContext::FromNativeContext(l)->Error(ScriptContext::ErrorType::Warning, true, "Access to undeclared '%s:%s'!", lua_tostring(l, -1), lua_tostring(l, -4));
                lua_pop(l, 1); // pop class name
            }
            else
            {
                if (lua_tocfunction(l, -1) == &Internal::LuaPropertyTagHelper) // if it's a property
                {
                    lua_getupvalue(l, -1, 1);   // push on the stack the getter function
                    lua_remove(l, -2); // remove property object

                    if (lua_isnil(l, -1))
                    {
                        lua_rawgeti(l, -2, AZ_LUA_CLASS_METATABLE_NAME_INDEX); // load the class name for a better error
                        if (!lua_isstring(l, -1))   // if we failed it means we are the base metatable
                        {
                            lua_pop(l, 1);
                            lua_rawgeti(l, 1, AZ_LUA_CLASS_METATABLE_NAME_INDEX);
                        }

                        ScriptContext::FromNativeContext(l)->Error(ScriptContext::ErrorType::Warning, true, "Property '%s:%s' is write only", lua_tostring(l, -1), lua_tostring(l, -4));
                        lua_pop(l, 1); // pop class name
                    }
                    else
                    {
                        lua_pushvalue(l, -4); // copy the user data to be passed as a this pointer.
                        lua_call(l, 1, 1); // call a function with one argument (this pointer) and 1 result
                    }
                }
            }

            lua_remove(l, metaTableIndex); // remove the metatable
            return 1;
        }

        int Class__IndexAllowNil(lua_State* l)
        {
            AZ::Locale::ScopedSerializationLocale scopedLocale;
            LSV_BEGIN(l, 1);

            // calling format __index(table,key)
            lua_getmetatable(l, -2); // load the userdata metatable
            int metaTableIndex = lua_gettop(l);

            // Check if the key is string, if so we expect it to be a function or property name
            // otherwise we allow users to provide custom index handlers
            // Technically we can allow strings too, but it will clash with function/property names and it be hard to figure
            // out what is going on from with in the system.
            if (lua_type(l, -2) == LUA_TSTRING)
            {
                lua_pushvalue(l, -2); // duplicate the key
                lua_rawget(l, -2); // load the value at this index
            }
            else
            {
                lua_pushliteral(l, "__AZ_Index");
                lua_rawget(l, -2); // check if the user provided custom Index method in the class metatable
                if (lua_isnil(l, -1)) // if not report an error
                {
                    lua_rawgeti(l, -2, AZ_LUA_CLASS_METATABLE_NAME_INDEX); // load the class name for a better error
                    if (!lua_isstring(l, -1))   // if we failed it means we are the base metatable
                    {
                        lua_pop(l, 1);
                        lua_rawgeti(l, 1, AZ_LUA_CLASS_METATABLE_NAME_INDEX);
                    }
                    ScriptContext::FromNativeContext(l)->Error(ScriptContext::ErrorType::Warning, true, "Invalid index type [], should be string! '%s:%s'!", lua_tostring(l, -1), lua_tostring(l, -4));
                }
                else
                {
                    // if we have custom index handler
                    lua_pushvalue(l, -4); // duplicate the table (class pointer)
                    lua_pushvalue(l, -4); // duplicate the index value for the call
                    lua_call(l, 2, 1); // call the function
                }

                lua_remove(l, metaTableIndex); // remove the metatable
                return 1;
            }

            if (!lua_isnil(l, -1))
            {
                if (lua_tocfunction(l, -1) == &Internal::LuaPropertyTagHelper) // if it's a property
                {
                    lua_getupvalue(l, -1, 1);   // push on the stack the getter function
                    lua_remove(l, -2); // remove property object

                    if (lua_isnil(l, -1))
                    {
                        lua_rawgeti(l, -2, AZ_LUA_CLASS_METATABLE_NAME_INDEX); // load the class name for a better error
                        if (!lua_isstring(l, -1))   // if we failed it means we are the base metatable
                        {
                            lua_pop(l, 1);
                            lua_rawgeti(l, 1, AZ_LUA_CLASS_METATABLE_NAME_INDEX);
                        }

                        ScriptContext::FromNativeContext(l)->Error(ScriptContext::ErrorType::Warning, true, "Property '%s:%s' is write only", lua_tostring(l, -1), lua_tostring(l, -4));
                        lua_pop(l, 1); // pop class name
                    }
                    else
                    {
                        lua_pushvalue(l, -4); // copy the user data to be passed as a this pointer.
                        lua_call(l, 1, 1); // call a function with one argument (this pointer) and 1 result
                    }
                }
            }

            lua_remove(l, metaTableIndex); // remove the metatable
            return 1;
        }


        //=========================================================================
        // Class__NewIndex
        // [3/22/2012]
        //=========================================================================
        int Class__NewIndex(lua_State* l)
        {
            AZ::Locale::ScopedSerializationLocale scopedLocale;
            LSV_BEGIN(l, 0);

            // calling format __index(table,key,value)
            lua_getmetatable(l, -3); // load the userdata metatable
            int metaTableIndex = lua_gettop(l);

            // Check if the key is string, if so we expect it to be a function or property name
            // otherwise we allow users to provide custom index handlers
            // Technically we can allow strings too, but it will clash with function/property names and it be hard to figure
            // out what is going on from with in the system.
            if (lua_type(l, -3) == LUA_TSTRING)
            {
                lua_pushvalue(l, -3); // duplicate the key
                lua_rawget(l, -2); // load the value at this index
            }
            else
            {
                lua_pushliteral(l, "__AZ_NewIndex");
                lua_rawget(l, -2); // check if the user provided custom NewIndex
                if (lua_isnil(l, -1))
                {
                    lua_rawgeti(l, -2, AZ_LUA_CLASS_METATABLE_NAME_INDEX); // load the class name for a better error
                    if (!lua_isstring(l, -1))   // if we failed it means we are the base metatable
                    {
                        lua_pop(l, 1);
                        lua_rawgeti(l, 1, AZ_LUA_CLASS_METATABLE_NAME_INDEX);
                    }
                    ScriptContext::FromNativeContext(l)->Error(ScriptContext::ErrorType::Warning, true, "Invalid index type [], should be string! '%s:%s'!", lua_tostring(l, -1), lua_tostring(l, -5));
                    lua_pop(l, 1); // remove class name
                }
                else
                {
                    // we have custom index handler
                    lua_pushvalue(l, -5); // duplicate the table (class pointer)
                    lua_pushvalue(l, -5); // duplicate the index
                    lua_pushvalue(l, -5); // duplicate the value
                    lua_call(l, 3, 0);
                }

                lua_remove(l, metaTableIndex); // remove metatable
                return 0;
            }

            if (lua_isnil(l, -1))
            {
                lua_rawgeti(l, -2, AZ_LUA_CLASS_METATABLE_NAME_INDEX); // load the class name for a better error
                if (!lua_isstring(l, -1))   // if we failed it means we are the base metatable
                {
                    lua_pop(l, 1);
                    lua_rawgeti(l, 1, AZ_LUA_CLASS_METATABLE_NAME_INDEX);
                }

                ScriptContext::FromNativeContext(l)->Error(ScriptContext::ErrorType::Error, true, "Assign to undeclared member '%s:%s'!", lua_tostring(l, -1), lua_tostring(l, -5));
                lua_pop(l, 1); // remove class name
            }
            else
            {
                if (lua_tocfunction(l, -1) != &Internal::LuaPropertyTagHelper)
                {
                    lua_rawgeti(l, -2, AZ_LUA_CLASS_METATABLE_NAME_INDEX); // load the class name for a better error
                    if (!lua_isstring(l, -1))   // if we failed it means we are the base metatable
                    {
                        lua_pop(l, 1);
                        lua_rawgeti(l, 1, AZ_LUA_CLASS_METATABLE_NAME_INDEX);
                    }

                    ScriptContext::FromNativeContext(l)->Error(ScriptContext::ErrorType::Error, true, "Invalid assign to undeclared property or method '%s:%s'!", lua_tostring(l, -1), lua_tostring(l, -5));
                    lua_pop(l, 1); // remove class name
                }

                lua_getupvalue(l, -1, 2);   // push on the stack the setter function
                lua_remove(l, -2); // remove the prop object

                // <LuaCaller*>
                //lua_getupvalue(l, -1, 1);
                //LuaCaller* caller = reinterpret_cast<LuaCaller*>(lua_touserdata(l, -1));
                //BehaviorMethod* method = caller->m_method;
                //lua_pop(l, 1);
                // </LuaCaller*>

                if (lua_isnil(l, -1))
                {
                    lua_rawgeti(l, -2, AZ_LUA_CLASS_METATABLE_NAME_INDEX); // load the class name for a better error
                    if (!lua_isstring(l, -1))   // if we failed it means we are the base metatable
                    {
                        lua_pop(l, 1);
                        lua_rawgeti(l, 1, AZ_LUA_CLASS_METATABLE_NAME_INDEX);
                    }
                    ScriptContext::FromNativeContext(l)->Error(ScriptContext::ErrorType::Error, true, "Property '%s:%s' is read only", lua_tostring(l, -1), lua_tostring(l, -5));
                    lua_pop(l, 1); // remove class name
                }
                else
                {
                    //void* thisPtr = lua_touserdata(l, -6);

                    lua_pushvalue(l, -5); // copy the user data for the this pointer
                    lua_pushvalue(l, -4); // copy the value for set
                    lua_call(l, 2, 0);

                    //BehaviorObjectSignals::Event(thisPtr, &BehaviorObjectSignals::Events::OnMemberMethodCalled, method);
                }
            }

            lua_remove(l, metaTableIndex); // remove metatable
            return 0;
        }

        //=========================================================================
        // Class__ToString
        // [3/24/2012]
        //=========================================================================
        int Class__ToString(lua_State* l)
        {
            LSV_BEGIN(l, 1);
            // Lua: object
            AZ::LuaUserData* userData = reinterpret_cast<AZ::LuaUserData*>(lua_touserdata(l, -1));
            AZ_Assert(userData && userData->magicData == AZ_CRC_CE("AZLuaUserData"), "this isn't user data");
            lua_getmetatable(l, -1); // load the class metatable
            // Lua: object, mt
            lua_rawgeti(l, -1, AZ_LUA_CLASS_METATABLE_NAME_INDEX);
            // Lua: object, mt, class name
            lua_remove(l, -2);
            // Lua: object, class name
            const char* className = lua_tostring(l, -1);
            lua_pop(l, 1);
            // Lua: object
            lua_pushstring(l, AZStd::string::format("LuaUserData (%p)->(%p):%s %s", userData, userData->value, StorageTypeToString((Script::Attributes::StorageType)userData->storageType), className).c_str());
            // Lua: object, string
            return 1;
        }

        inline size_t BufferStringCopy(const char* source, char* destination, size_t destinationSize)
        {
            size_t srcLen = strlen(source);
            size_t toCopy = AZStd::GetMin(srcLen, destinationSize - 1);
            if (toCopy)
            {
                memcpy(destination, source, toCopy);
            }
            destination[toCopy] = '\0';
            return toCopy;
        }

        //=========================================================================
        // LuaStackTrace
        // [3/26/2012]
        //=========================================================================
        void LuaStackTrace(lua_State* l, char* stackOutput, size_t stackOutputSize)
        {
            lua_Debug entry;
            int depth = 0;
            if (stackOutput)
            {
                size_t bytesAdded = BufferStringCopy("\n ---------- START STACK TRACE ---------- \n", stackOutput, stackOutputSize);
                stackOutput += bytesAdded;
                stackOutputSize -= bytesAdded;
            }
            else
            {
                AZ_TracePrintf("Script", "\n ---------- START STACK TRACE ---------- \n");
            }

            //luaL_traceback(l,)
            char lineBuffer[ScriptContext::m_maxDbgName * 2];
            AZ::u32 stackDepthLimit = 1000;
            while (stackDepthLimit && lua_getstack(l, depth, &entry))
            {
                --stackDepthLimit;
                int status = lua_getinfo(l, "Sln", &entry);
                if (status == 0)
                {
                    break;
                }
                if (stackOutput)
                {
                    azsnprintf(lineBuffer, AZ_ARRAY_SIZE(lineBuffer), "%s(%d): %s\n", entry.source, entry.currentline, entry.name ? entry.name : "?");
                    size_t bytesAdded = BufferStringCopy(lineBuffer, stackOutput, stackOutputSize);
                    stackOutput += bytesAdded;
                    stackOutputSize -= bytesAdded;
                }
                else
                {
                    AZ_TracePrintf("Script", "%s(%d): %s\n", entry.short_src, entry.currentline, entry.name ? entry.name : "?");
                }
                depth++;
            }

            if (!stackDepthLimit)
            {
                AZ_TracePrintf("Script", "stack depth was over 1000, and full error info could not be retrieved. This is likely due to an infinite loop in Lua from a tail-ended recursive function.");
            }

            if (stackOutput)
            {
                BufferStringCopy("\n ---------- END STACK TRACE ---------- \n\n", stackOutput, stackOutputSize);
            }
            else
            {
                AZ_TracePrintf("Script", "\n ---------- END STACK TRACE ---------- \n\n");
            }
        }



        //////////////////////////////////////////////////////////////////////////
        int ClassAcquireOwnership(lua_State* l)
        {
            if (LuaIsClass(l,-1))
            {
                Internal::LuaAcquireClassOwnership(l, -1);
            }

            // copy the source as a result
            lua_pushvalue(l, -1);
            return 1;
        }

        //////////////////////////////////////////////////////////////////////////
        int ClassReleaseOwnership(lua_State* l)
        {
            if (LuaIsClass(l,-1))
            {
                Internal::LuaReleaseClassOwnership(l, -1, false);
            }
            // copy the source as a result
            lua_pushvalue(l, -1);
            return 1;
        }
    }

// #define AZ_ENABLE_LUA_ERRORS_ON_NUMBERS

    void RestrictLuaNumbersToCpp(lua_State* lua, AZStd::string_view msg)
    {
        AZ_UNUSED(msg);
#if defined(AZ_LUA_RESTRICT_NUMBER_CONVERSIONS_TO_CPP)
        luaL_error(lua, "%s", msg.data());
#else
        AZ_UNUSED(lua);
        AZ_Warning("Script", false, msg.data());
#endif
    }

    template <typename T>
    AZ_FORCE_INLINE T azlua_checknumber(lua_State* l, int index)
    {
        // Turn this feature on whenever there is TRACING enabled, which is where things like luaL_error will have any effect.
        // when TRACING is not enabled, chances are luaL_error will not be seen as there will be no listener
#if defined(AZ_ENABLE_TRACING)
        using limits = std::numeric_limits<T>;
        double number = luaL_checknumber(l, index);

        // Convert math.huge (infinity) to the type-appropriate value.
        if (limits::has_infinity)
        {
            if (number == std::numeric_limits<double>::infinity())
            {
                return limits::infinity();
            }
            AZ_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option") // conditional expression is constant
            else if (limits::is_signed && number == -std::numeric_limits<double>::infinity())
            AZ_POP_DISABLE_WARNING
            {
                AZ_PUSH_DISABLE_WARNING(4146, "-Wunknown-warning-option") // unary minus operator applied to unsigned type, result still unsigned
                return -limits::infinity();
                AZ_POP_DISABLE_WARNING
            }
        }

        // Check for decimal to integer conversion
        AZ_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option") // conditional expression is constant
        if (!std::is_floating_point<T>::value && std::fmod(number, 1.0f) > std::numeric_limits<double>::epsilon())
        AZ_POP_DISABLE_WARNING
        {
            RestrictLuaNumbersToCpp(l, "integer expected, got floating point");
        }

        // Check for proper signed-ness
        AZ_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option") // conditional expression is constant
        if (!limits::is_signed && number < 0.0f)
        AZ_POP_DISABLE_WARNING
        {
            RestrictLuaNumbersToCpp(l, "unsigned expected, got negative");
        }

        // Ensure value is within limits
        if (number > limits::max())
        {
            RestrictLuaNumbersToCpp(l, "number was too large for numerical limits");
        }

        if (number < limits::lowest())
        {
            RestrictLuaNumbersToCpp(l, "number was too small for numerical limits");
        }

        return static_cast<T>(number);
#else
        // If not doing a debug build, just convert it
        return static_cast<T>(lua_tonumber(l, index));
#endif // AZ_ENABLE_TRACING
    }

    //////////////////////////////////////////////////////////////////////////
    void ScriptValue<char>::StackPush(lua_State* l, int value)
    {
        lua_pushinteger(l, value);
    }

    //////////////////////////////////////////////////////////////////////////
    char ScriptValue<char>::StackRead(lua_State* l, int stackIndex)
    {
        return azlua_checknumber<char>(l, stackIndex);
    }
    //////////////////////////////////////////////////////////////////////////
    void ScriptValue<short>::StackPush(lua_State* l, int value)
    {
        lua_pushinteger(l, value);
    }

    //////////////////////////////////////////////////////////////////////////
    short ScriptValue<short>::StackRead(lua_State* l, int stackIndex)
    {
        return azlua_checknumber<short>(l, stackIndex);
    }
    //////////////////////////////////////////////////////////////////////////
    void ScriptValue<int>::StackPush(lua_State* l, int value)
    {
        lua_pushinteger(l, value);
    }
    //////////////////////////////////////////////////////////////////////////
    int ScriptValue<int>::StackRead(lua_State* l, int stackIndex)
    {
        return azlua_checknumber<int>(l, stackIndex);
    }
    //////////////////////////////////////////////////////////////////////////
    void ScriptValue<AZ::s64>::StackPush(lua_State* l, AZ::s64 value)
    {
        lua_pushnumber(l, aznumeric_caster(value));
    }
    //////////////////////////////////////////////////////////////////////////
    AZ::s64 ScriptValue<AZ::s64>::StackRead(lua_State* l, int stackIndex)
    {
        return aznumeric_caster(lua_tonumber(l, stackIndex));
    }
    //////////////////////////////////////////////////////////////////////////
    void ScriptValue<long>::StackPush(lua_State* l, long value)
    {
        lua_pushnumber(l, aznumeric_caster(value));
    }
    //////////////////////////////////////////////////////////////////////////
    long ScriptValue<long>::StackRead(lua_State* l, int stackIndex)
    {
        return aznumeric_caster(lua_tonumber(l, stackIndex));
    }

    //////////////////////////////////////////////////////////////////////////
    void ScriptValue<unsigned char>::StackPush(lua_State* l, unsigned int value)
    {
        lua_pushinteger(l, aznumeric_caster(value));
    }

    //////////////////////////////////////////////////////////////////////////
    unsigned char ScriptValue<unsigned char>::StackRead(lua_State* l, int stackIndex)
    {
        return azlua_checknumber<unsigned char>(l, stackIndex);
    }

    //////////////////////////////////////////////////////////////////////////
    void ScriptValue<unsigned short>::StackPush(lua_State* l, unsigned int value)
    {
        lua_pushinteger(l, value);
    }

    //////////////////////////////////////////////////////////////////////////
    unsigned short ScriptValue<unsigned short>::StackRead(lua_State* l, int stackIndex)
    {
        return azlua_checknumber<unsigned short>(l, stackIndex);
    }

    //////////////////////////////////////////////////////////////////////////
    void ScriptValue<unsigned int>::StackPush(lua_State* l, unsigned int value)
    {
        lua_pushinteger(l, aznumeric_caster(value));
    }

    //////////////////////////////////////////////////////////////////////////
    unsigned int ScriptValue<unsigned int>::StackRead(lua_State* l, int stackIndex)
    {
        return azlua_checknumber<unsigned int>(l, stackIndex);
    }

    //////////////////////////////////////////////////////////////////////////
    void ScriptValue<AZ::u64>::StackPush(lua_State* l, AZ::u64 value)
    {
        lua_pushnumber(l, aznumeric_caster(value));
    }

    //////////////////////////////////////////////////////////////////////////
    AZ::u64 ScriptValue<AZ::u64>::StackRead(lua_State* l, int stackIndex)
    {
        return aznumeric_caster(lua_tonumber(l, stackIndex));
    }

    //////////////////////////////////////////////////////////////////////////
    void ScriptValue<unsigned long>::StackPush(lua_State* l, unsigned long value)
    {
        lua_pushnumber(l, aznumeric_caster(value));
    }

    //////////////////////////////////////////////////////////////////////////
    unsigned long ScriptValue<unsigned long>::StackRead(lua_State* l, int stackIndex)
    {
        return aznumeric_caster(lua_tonumber(l, stackIndex));
    }


    //////////////////////////////////////////////////////////////////////////
    void ScriptValue<float>::StackPush(lua_State* l, float value)
    {
        AZ::Locale::ScopedSerializationLocale scopedLocale;
        lua_pushnumber(l, value);
    }

    //////////////////////////////////////////////////////////////////////////
    float ScriptValue<float>::StackRead(lua_State* l, int stackIndex)
    {
        AZ::Locale::ScopedSerializationLocale scopedLocale;
        return azlua_checknumber<float>(l, stackIndex);
    }

    //////////////////////////////////////////////////////////////////////////
    void ScriptValue<double>::StackPush(lua_State* l, double value)
    {
        AZ::Locale::ScopedSerializationLocale scopedLocale;
        lua_pushnumber(l, static_cast<lua_Number>(value));
    }

    //////////////////////////////////////////////////////////////////////////
    double ScriptValue<double>::StackRead(lua_State* l, int stackIndex)
    {
        AZ::Locale::ScopedSerializationLocale scopedLocale;
        return lua_tonumber(l, stackIndex);
    }

    //////////////////////////////////////////////////////////////////////////
    void ScriptValue<bool>::StackPush(lua_State* l, bool value)
    {
        lua_pushboolean(l, value);
    }

    //////////////////////////////////////////////////////////////////////////
    bool ScriptValue<bool>::StackRead(lua_State* l, int stackIndex)
    {
        return lua_toboolean(l, stackIndex) ? true : false;
    }

    //////////////////////////////////////////////////////////////////////////
    void ScriptValue<const char*>::StackPush(lua_State* l, const char* value)
    {
        lua_pushstring(l, value);
    }

    //////////////////////////////////////////////////////////////////////////
    const char* ScriptValue<const char*>::StackRead(lua_State* l, int stackIndex)
    {
        return lua_tostring(l, stackIndex);
    }

    //////////////////////////////////////////////////////////////////////////
    void ScriptValue<void*>::StackPush(lua_State* l, void* value)
    {
        lua_pushlightuserdata(l, value);
    }
    //////////////////////////////////////////////////////////////////////////
    void* ScriptValue<void*>::StackRead(lua_State* l, int stackIndex)
    {
        return lua_touserdata(l, stackIndex);
    }
    //////////////////////////////////////////////////////////////////////////
    void ScriptValue<AZStd::any>::StackPush(lua_State* l, const AZStd::any& value)
    {
#define CHECK_BUILTIN(T)                                                                        \
        if (value.is<T>()) {                                                                    \
            ScriptValue<T>::StackPush(l, AZStd::any_cast<T>(const_cast<AZStd::any&>(value)));   \
            return;                                                                             \
        }

        if (value.empty())
        {
            lua_pushnil(l);
        }
        else CHECK_BUILTIN(bool)
        else CHECK_BUILTIN(char)
        else CHECK_BUILTIN(u8)
        else CHECK_BUILTIN(s16)
        else CHECK_BUILTIN(u16)
        else CHECK_BUILTIN(s32)
        else CHECK_BUILTIN(u32)
        else CHECK_BUILTIN(s64)
        else CHECK_BUILTIN(u64)
        else CHECK_BUILTIN(float)
        else CHECK_BUILTIN(double)
        else CHECK_BUILTIN(char*)
        else CHECK_BUILTIN(AZStd::string)
        else
        {
            Internal::LuaClassToStack(l, AZStd::any_cast<void>(&const_cast<AZStd::any&>(value)), value.type());
        }

#undef CHECK_BUILTIN
    }
    //////////////////////////////////////////////////////////////////////////
    AZStd::any ScriptValue<AZStd::any>::StackRead(lua_State* lua, int stackIndex)
    {
        AZStd::any value;

        switch (lua_type(lua, stackIndex))
        {
        case LUA_TNUMBER:
        {
            lua_Number number = lua_tonumber(lua, stackIndex);
            value = AZStd::make_any<lua_Number>(number);
        }
        break;

        case LUA_TBOOLEAN:
        {
            value = AZStd::make_any<bool>(lua_toboolean(lua, stackIndex) == 1);
        }
        break;

        case LUA_TSTRING:
        {
            value = AZStd::make_any<AZStd::string>(lua_tostring(lua, stackIndex));
        }
        break;

        case LUA_TUSERDATA:
        case LUA_TLIGHTUSERDATA:
        {
            void* userData = nullptr;
            const BehaviorClass* sourceClass = nullptr;
            Internal::LuaGetClassInfo(lua, stackIndex, &userData, &sourceClass);

            // Can't copy value
            Script::Attributes::StorageType storage = Script::Attributes::StorageType::ScriptOwn;
            if (auto attribute = FindAttribute(Script::Attributes::Storage, sourceClass->m_attributes))
            {
                AttributeReader reader(nullptr, attribute);
                reader.Read<AZ::Script::Attributes::StorageType>(storage);
            }

            AZStd::any::type_info type;
            type.m_id = sourceClass->m_typeId;
            type.m_isPointer = false;
            type.m_useHeap = true;

            if (storage == AZ::Script::Attributes::StorageType::Value)
            {
                if (!sourceClass->m_allocate || !sourceClass->m_cloner || !sourceClass->m_mover || !sourceClass->m_destructor || !sourceClass->m_deallocate)
                {
                    return AZStd::any();
                }

                // If it's a value type, copy/move it around
                type.m_handler = [sourceClass](AZStd::any::Action action, AZStd::any* dest, const AZStd::any* source)
                {
                    switch (action)
                    {
                    case AZStd::any::Action::Reserve:
                    {
                        *reinterpret_cast<void**>(dest) = sourceClass->Allocate();
                        break;
                    }
                    case AZStd::any::Action::Copy:
                    {
                        sourceClass->m_cloner(AZStd::any_cast<void>(dest), AZStd::any_cast<void>(source), sourceClass->m_userData);
                        break;
                    }
                    case AZStd::any::Action::Move:
                    {
                        sourceClass->m_mover(AZStd::any_cast<void>(dest), AZStd::any_cast<void>(const_cast<AZStd::any*>(source)), sourceClass->m_userData);
                        break;
                    }
                    case AZStd::any::Action::Destroy:
                    {
                        sourceClass->Destroy(BehaviorObject(AZStd::any_cast<void>(dest), sourceClass->m_typeId));
                        break;
                    }
                    }
                };
            }
            else
            {
                // If not a value type, just move the pointer around
                type.m_handler = [](AZStd::any::Action action, AZStd::any* dest, const AZStd::any* source)
                {
                    switch (action)
                    {
                    case AZStd::any::Action::Reserve:
                    {
                        // No-op
                        break;
                    }
                    case AZStd::any::Action::Copy:
                    case AZStd::any::Action::Move:
                    {
                        *reinterpret_cast<void**>(dest) = AZStd::any_cast<void>(const_cast<AZStd::any*>(source));
                        break;
                    }
                    case AZStd::any::Action::Destroy:
                    {
                        *reinterpret_cast<void**>(dest) = nullptr;
                        break;
                    }
                    }
                };
            }

            value = AZStd::any(userData, type);
        }
        break;

        case LUA_TNIL:
        case LUA_TTABLE:
        case LUA_TFUNCTION:
        case LUA_TTHREAD:
        default:
            // Nil value, Tables, functions, and threads will never be convertible, as we have no structure to convert it to
            break;
        }

        return value;
    }

    //////////////////////////////////////////////////////////////////////////
    LuaNativeThread::LuaNativeThread(lua_State* rootState)
    {
        AZ_Assert(rootState, "Cannot pass nullptr to LuaNativeThread().");
        m_thread = lua_newthread(rootState);
        m_registryIndex = luaL_ref(rootState, LUA_REGISTRYINDEX);
    }

    //////////////////////////////////////////////////////////////////////////
    LuaNativeThread::~LuaNativeThread()
    {
        // Clear stack
        lua_pop(m_thread, lua_gettop(m_thread));
        // Release reference (GC will collect thread)
        luaL_unref(m_thread, LUA_REGISTRYINDEX, m_registryIndex);

        m_thread = nullptr;
        m_registryIndex = LUA_NOREF;
    }
}

namespace AZ
{

#ifndef AZ_USE_CUSTOM_SCRIPT_BIND

#define LUA_DEFAULT_ALIGNMENT 16

// 1 means use the allocator default alignment

// Allocate a variable on the stack and trim the name to _maxLengh characters if needed.
#define AZ_DBG_NAME_FIXER(_varName, _maxLength, _dbgNameSrc)                  \
    const char* _varName;                                                     \
    char  _varNameData[_maxLength];                                           \
    if ((_dbgNameSrc) != NULL) {                                              \
        size_t len = strlen(_dbgNameSrc);                                     \
        if (len > _maxLength - 1) {                                           \
            _varName = _varNameData;                                          \
            azstrncpy(_varNameData, _maxLength, _dbgNameSrc, _maxLength - 1); \
            _varNameData[_maxLength - 1] = '\0';                              \
                } else {                                                              \
            _varName = _dbgNameSrc;                                           \
                }                                                                     \
        } else {                                                                  \
        _varName = "?";                                                       \
        }

//=========================================================================
// Lua Memory manager hook
// [3/19/2012]
//=========================================================================
static void* LuaMemoryHook(void* userData, void* ptr, size_t osize, size_t nsize)
{
    (void)osize;
    IAllocator* allocator = reinterpret_cast<IAllocator*>(userData);
    if (nsize == 0)
    {
        if (ptr)
        {
            allocator->DeAllocate(ptr);
        }
        return nullptr;
    }
    else if (ptr == nullptr)
    {
        return allocator->Allocate(nsize, LUA_DEFAULT_ALIGNMENT);
    }
    else
    {
        return allocator->ReAllocate(ptr, nsize, LUA_DEFAULT_ALIGNMENT);
    }
}

//=========================================================================
// Lua Panic callback
// [3/19/2012]
//=========================================================================
static int LuaPanic(lua_State* l)
{
    ScriptContext::FromNativeContext(l)->Error(ScriptContext::ErrorType::Error, true, "PANIC: unprotected error in call to Lua API (%s)\n", lua_tostring(l, -1));
    return 0;  /* return to Lua to abort */
}

//=========================================================================
// Global__NewIndex
// [3/21/2012]
//=========================================================================
static int Global__NewIndex(lua_State* l)
{
    LSV_BEGIN(l, 0);

    // calling format __index(table,key,value)
    lua_rawgeti(l, LUA_REGISTRYINDEX, AZ_LUA_GLOBALS_TABLE_REF); // load the globals table
    lua_pushvalue(l, -3); // duplicate the key
    lua_rawget(l, -2); // load the value at this index
    lua_remove(l, -2); // remove AZ global table

    if (lua_isnil(l, -1))
    {
        // pop the nil property object
        lua_pop(l, 1);
        // push global table to store value in (only properties go in the AZ global table)
        lua_pushglobaltable(l);
        // push the original key and value again
        lua_pushvalue(l, -3);
        lua_pushvalue(l, -3);
        // set the property in the global table
        lua_rawset(l, -3);
        lua_pop(l, 1); // remove the global table
    }
    else
    {
        if (lua_tocfunction(l, -1) != &AZ::Internal::LuaPropertyTagHelper)
        {
            ScriptContext::FromNativeContext(l)->Error(ScriptContext::ErrorType::Warning, true, "Invalid global property '%s'!", lua_tostring(l, -3));
            // Not sure what's here, but we just want nil, so pop it and push nil
            lua_pop(l, 1);
            lua_pushnil(l);
            return 0;
        }

        lua_getupvalue(l, -1, 2);   // push on the stack the setter function
        lua_remove(l, -2); // remove the property object
        if (lua_isnil(l, -1))
        {
            ScriptContext::FromNativeContext(l)->Error(ScriptContext::ErrorType::Warning, true, "Property '%s' is read only", lua_tostring(l, -3));
        }
        else
        {
            lua_pushvalue(l, -2); // copy the value for the input
            lua_call(l, 1, 0);
        }
    }
    return 0;
}

//=========================================================================
// Global__Index
// [3/21/2012]
//=========================================================================
static int Global__Index(lua_State* l)
{
    LSV_BEGIN(l, 1);

    // calling format __index(table,key)
    lua_rawgeti(l, LUA_REGISTRYINDEX, AZ_LUA_GLOBALS_TABLE_REF); // load the globals table
    lua_pushvalue(l, -2); // duplicate the key
    lua_rawget(l, -2); // load the value at this index
    lua_remove(l, -2); // remove global table

    if (lua_isnil(l, -1))
    {
        // we did not found the element in the table trigger an error
        ScriptContext::FromNativeContext(l)->Error(ScriptContext::ErrorType::Warning, true, "Access to undeclared global variable '%s'", lua_tostring(l, -2));
    }
    else
    {
        if (lua_tocfunction(l, -1) == &AZ::Internal::LuaPropertyTagHelper) // if it's a property
        {
            lua_getupvalue(l, -1, 1);   // push on the stack the getter function
            lua_remove(l, -2); // remove the property itself

            if (lua_isnil(l, -1))
            {
                ScriptContext::FromNativeContext(l)->Error(ScriptContext::ErrorType::Warning, true, "Property '%s' is write only", lua_tostring(l, -2));
            }
            else
            {
                lua_call(l, 0, 1); // call a function with no arguments and 1 result
            }
        }
    }
    return 1;
}

//=========================================================================
// TypeId
//=========================================================================
static int Global_Typeid(lua_State* l)
{
    LSV_BEGIN(l, 1);

    AZ::Uuid typeId = AZ::Uuid::CreateNull();

    if (lua_gettop(l) == 1)
    {
        switch (lua_type(l, -1))
        {
        case LUA_TNIL:
        {
            typeId = azrtti_typeid<void>();
        }
        break;

        case LUA_TBOOLEAN:
        {
            typeId = azrtti_typeid<bool>();
        }
        break;

        case LUA_TNUMBER:
        {
            typeId = azrtti_typeid<lua_Number>();
        }
        break;

        case LUA_TSTRING:
        {
            typeId = azrtti_typeid<AZStd::string>();
        }
        break;

        case LUA_TTABLE:
        case LUA_TUSERDATA:
        case LUA_TLIGHTUSERDATA:
        {
            // load the class metatable
            if (lua_getmetatable(l, -1))
            {
                // load the class's BehaviorClass
                lua_rawgeti(l, -1, AZ_LUA_CLASS_METATABLE_BEHAVIOR_CLASS);
                BehaviorClass* behaviorClass = reinterpret_cast<BehaviorClass*>(lua_touserdata(l, -1));
                // capture the typeid
                typeId = behaviorClass->m_typeId;
                // remove the behavior class and metatable
                lua_pop(l, 2);
            }
        }
        break;

        default:
            break;
        }
    }
    else
    {
        ScriptContext::FromNativeContext(l)->Error(ScriptContext::ErrorType::Warning, false, "function 'typeid' requires exactly 1 argument.");
    }

    // Write the typeid (may be Null)
    AZ::Internal::LuaClassToStack(l, &typeId, azrtti_typeid<AZ::Uuid>());

    return 1;
}

#ifdef LUA_O3DE_EXTENSIONS

//=========================================================================
// LUA Dummy Node Extension
// Through an environment variable we can ensure that LUA's static
// dummynode is safe across DLLs
// [5/18/2016]
//=========================================================================

static AZ::EnvironmentVariable<Node> s_luaDummyNodeVariable;

LUA_API const Node* lua_getDummyNode()
{
    if (!s_luaDummyNodeVariable)
    {
        const AZ::u32 luaDummyNodeVariableID = AZ_CRC_CE("lua_DummyNode");

        s_luaDummyNodeVariable = AZ::Environment::FindVariable<Node>(luaDummyNodeVariableID);
        if (!s_luaDummyNodeVariable)
        {
            s_luaDummyNodeVariable = AZ::Environment::CreateVariable<Node>(luaDummyNodeVariableID);
            *s_luaDummyNodeVariable = {
                { { NULL }, LUA_TNIL },  /* value */
                { { { NULL }, LUA_TNIL, NULL } }  /* key */
            };
        }
    }

    return &(*s_luaDummyNodeVariable);
}

#endif // LUA_O3DE_EXTENSIONS

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // ScriptDataContext
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

    const char* ScriptDataContext::GetInterpreterVersion()
    {
        return LUA_VERSION;
    }

    //////////////////////////////////////////////////////////////////////////
    ScriptContext*
        ScriptDataContext::GetScriptContext() const
    {
        return ScriptContext::FromNativeContext(m_nativeContext);
    }

    //=========================================================================
    // Reset
    // [4/3/2012]
    //=========================================================================
    void
        ScriptDataContext::Reset()
    {
        if (m_mode != MD_INVALID && m_mode != MD_CALLEE && m_mode != MD_READ_STACK)   // MD_CALLEE - we need to leave the results for the script
        {
            int currentTop = lua_gettop(m_nativeContext);
            AZ_Assert(m_startVariableIndex <= currentTop ||
                (m_mode == MD_CALLER_EXECUTED && m_startVariableIndex <= currentTop + 1),
                "Invalid stack!");
            lua_pop(m_nativeContext, (currentTop - m_startVariableIndex) + 1);

            m_nativeContext = nullptr;
            m_startVariableIndex = 0;
            m_numArguments = 0;
            m_numResults = 0;
            m_mode = MD_INVALID;
        }
    }

    //=========================================================================
    // CacheValue
    // [4/4/2012]
    //=========================================================================
    int
        ScriptDataContext::CacheValue(int index)
    {
        LSV_BEGIN(m_nativeContext, 0);

        return AZ::Internal::LuaCacheValue(m_nativeContext,m_startVariableIndex + index);
    }

    //=========================================================================
    // ReleaseCached
    // [4/13/2012]
    //=========================================================================
    void
        ScriptDataContext::ReleaseCached(int cachedIndex)
    {
        LSV_BEGIN(m_nativeContext, 0);

        AZ::Internal::LuaReleaseCached(m_nativeContext,cachedIndex);
    }

    //=========================================================================
    // PushArgFromRegistryIndex
    // [9/13/2016]
    //=========================================================================
    void
        ScriptDataContext::PushArgFromRegistryIndex(int cachedIndex)
    {
        LSV_BEGIN(m_nativeContext, 1);

        lua_rawgeti(m_nativeContext, LUA_REGISTRYINDEX, cachedIndex);
        ++m_numArguments;
    }

    //=========================================================================
    // PushArgCached
    // [4/13/2012]
    //=========================================================================
    void
        ScriptDataContext::PushArgCached(int cachedIndex)
    {
        LSV_BEGIN(m_nativeContext, 1);

        Internal::LuaLoadCached(m_nativeContext,cachedIndex);
        ++m_numArguments;
    }

    //=========================================================================
    // PushArgScriptProperty
    // [7/18/2016]
    //=========================================================================
    void
        ScriptDataContext::PushArgScriptProperty(AZ::ScriptProperty* scriptProperty)
    {
        LSV_BEGIN(m_nativeContext, 1);

        if (scriptProperty == nullptr || !scriptProperty->Write((*GetScriptContext())))
        {
            // If we fail to write our arg, or the property is null, push a nil.
            lua_pushnil(m_nativeContext);
        }

        ++m_numArguments;
    }

    //=========================================================================
    // PushResultCached
    // [4/13/2012]
    //=========================================================================
    void
        ScriptDataContext::PushResultCached(int cachedIndex)
    {
        LSV_BEGIN(m_nativeContext, 1);

        Internal::LuaLoadCached(m_nativeContext, cachedIndex);
        ++m_numResults;
    }

    //=========================================================================
    // CheckTableElement
    // [4/3/2012]
    //=========================================================================
    bool
        ScriptDataContext::CheckTableElement(int tableIndex, const char* name)
    {
        LSV_BEGIN(m_nativeContext, 0);

        if (m_mode == MD_INSPECT)
        {
            bool result = false;
            lua_getfield(m_nativeContext, m_startVariableIndex + tableIndex, name);
            if (!lua_isnil(m_nativeContext, -1))
            {
                result = true;
            }
            lua_pop(m_nativeContext, 1);
            return result;
        }
        else
        {
            AZ_Warning("Script", false, "You can check table element only in INSPECT mode!");
        }

        return false;
    }

    //=========================================================================
    // CheckTableElement
    // [4/3/2012]
    //=========================================================================
    bool
        ScriptDataContext::CheckTableElement(const char* name)
    {
        LSV_BEGIN(m_nativeContext, 0);

        if (m_mode == MD_INSPECT)
        {
            if (lua_istable(m_nativeContext, m_startVariableIndex))
            {
                bool result = false;
                lua_getfield(m_nativeContext, m_startVariableIndex, name);
                if (!lua_isnil(m_nativeContext, -1))
                {
                    result = true;
                }
                lua_pop(m_nativeContext, 1);
                return result;
            }
            else
            {
                AZ_Warning("Script", false, "This context doesn't point to a table, use PushTableElement(tableIndex...)!");
            }
        }
        else
        {
            AZ_Warning("Script", false, "You can check table element only in INSPECT mode!");
        }

        return false;
    }

    //=========================================================================
    // PushTableElement
    // [11/29/2012]
    //=========================================================================
    bool
        ScriptDataContext::PushTableElement(int tableIndex, const char* name, int* valueIndex)
    {
        LSV_BEGIN_VARIABLE(m_nativeContext);

        if (m_mode == MD_INSPECT)
        {
            lua_getfield(m_nativeContext, m_startVariableIndex + tableIndex, name);
            if (!lua_isnil(m_nativeContext, -1))
            {
                if (valueIndex)
                {
                    *valueIndex = lua_gettop(m_nativeContext) - m_startVariableIndex;
                }

                LSV_END_VARIABLE(1);
                return true;
            }
            lua_pop(m_nativeContext, 1);
        }
        else
        {
            AZ_Warning("Script", false, "You can push table element only in INSPECT mode!");
        }

        LSV_END_VARIABLE(0);
        return false;
    }


    //=========================================================================
    // PushTableElement
    // [11/29/2012]
    //=========================================================================
    bool    ScriptDataContext::PushTableElement(const char* name, int* valueIndex)
    {
        LSV_BEGIN_VARIABLE(m_nativeContext);

        if (valueIndex)
        {
            *valueIndex = 0;
        }

        if (m_mode == MD_INSPECT)
        {
            if (lua_istable(m_nativeContext, m_startVariableIndex))
            {
                lua_getfield(m_nativeContext, m_startVariableIndex, name);
                if (!lua_isnil(m_nativeContext, -1))
                {
                    if (valueIndex)
                    {
                        *valueIndex = lua_gettop(m_nativeContext) - m_startVariableIndex;
                    }

                    LSV_END_VARIABLE(1);
                    return true;
                }
                lua_pop(m_nativeContext, 1);
            }
            else
            {
                AZ_Warning("Script", false, "This context doesn't point to a table, use PushTableElement(tableIndex...)!");
            }
        }
        else
        {
            AZ_Warning("Script", false, "You can push table element only in INSPECT mode!");
        }

        LSV_END_VARIABLE(0);
        return false;
    }

    //=========================================================================
    // ConstructTableScriptProperty
    // [03/07/2017]
    //=========================================================================
    ScriptProperty* ScriptDataContext::ConstructTableScriptProperty(const char* name, bool restrictToPropertyArrays)
    {
        ScriptProperty* constructedProperty = nullptr;

        if (m_mode == MD_INSPECT)
        {
            if (lua_istable(m_nativeContext, m_startVariableIndex))
            {
                lua_getfield(m_nativeContext, m_startVariableIndex, name);

                int index = lua_gettop(m_nativeContext) - m_startVariableIndex;

                constructedProperty = ConstructScriptProperty(index,name, restrictToPropertyArrays);
                lua_pop(m_nativeContext, 1);
            }
            else
            {
                AZ_Warning("Script", false, "This context doesn't point to a table, use PushTableElement(tableIndex...)!");
            }
        }
        else
        {
            AZ_Warning("Script", false, "You can push table element only in INSPECT mode!");
        }

        return constructedProperty;
    }

    //=========================================================================
    // InspectTable
    // [4/3/2012]
    //=========================================================================
    bool
        ScriptDataContext::InspectTable(int tableIndex, ScriptDataContext& dc)
    {
        AZ_Assert(&dc != this, "You must pass a new script data context, for each layer of operation!");
        dc.Reset();

        LSV_BEGIN_VARIABLE(m_nativeContext);

        if (lua_istable(m_nativeContext, m_startVariableIndex + tableIndex))
        {
            lua_pushvalue(m_nativeContext, m_startVariableIndex + tableIndex);
            dc.SetInspect(m_nativeContext, lua_gettop(m_nativeContext));
            lua_pushnil(m_nativeContext); // key
            lua_pushnil(m_nativeContext); // value

            LSV_END_VARIABLE(3);
            return true;
        }
        else
        {
            AZ_Warning("Script", false, "tableIndex %d doesn't point to a table!", tableIndex);
        }

        LSV_END_VARIABLE(0);
        return false;
    }

    //=========================================================================
    // InspectMetaTable
    // [11/29/2012]
    //=========================================================================
    bool
        ScriptDataContext::InspectMetaTable(int index, ScriptDataContext& dc)
    {
        AZ_Assert(&dc != this, "You must pass a new script data context, for each layer of operation!");
        dc.Reset();

        LSV_BEGIN_VARIABLE(m_nativeContext);

        int metaIndex = lua_getmetatable(m_nativeContext, m_startVariableIndex + index);
        if (metaIndex)
        {
            dc.SetInspect(m_nativeContext, lua_gettop(m_nativeContext));
            lua_pushnil(m_nativeContext); // key
            lua_pushnil(m_nativeContext); // value

            LSV_END_VARIABLE(3);
            return true;
        }

        LSV_END_VARIABLE(0);
        return false;
    }

    //=========================================================================
    // InspectNextElement
    // [4/3/2012]
    //=========================================================================
    bool
        ScriptDataContext::InspectNextElement(int& valueIndex, const char*& name, int& index, bool isProtectedElements)
    {
        LSV_BEGIN_VARIABLE(m_nativeContext);

        valueIndex = 0;
        name = nullptr;
        index = -1;
        if (m_mode == MD_INSPECT)
        {
            while (true)
            {
                lua_pop(m_nativeContext, 1); // pop the last value from the stack
                if (lua_next(m_nativeContext, m_startVariableIndex /*[m_inspectTableTop-1]*/))
                {
                    if (lua_type(m_nativeContext, -2) == LUA_TSTRING)
                    {
                        name = lua_tostring(m_nativeContext, -2);
                        if (!isProtectedElements && strncmp(name, "__", 2) == 0)
                        {
                            continue; // skip all protected methods
                        }
                    }
                    else
                    {
                        index = static_cast<int>(lua_tointeger(m_nativeContext, -2));
                    }
                    valueIndex = lua_gettop(m_nativeContext) - m_startVariableIndex;

                    LSV_END_VARIABLE(0);
                    return true;
                }
                break;
            }
        }
        else
        {
            AZ_Warning("Script", false, "You can inspect table only in INSPECT mode!");
        }

        LSV_END_VARIABLE(-2);
        return false;
    }

    //=========================================================================
    // CallStart
    // [4/4/2012]
    //=========================================================================
    bool
        ScriptDataContext::Call(int functionIndex, ScriptDataContext& dc)
    {
        if (lua_isfunction(m_nativeContext, m_startVariableIndex + functionIndex))
        {
            AZ_Assert(&dc != this, "You must pass a new script data context, for each layer of operation!");
            dc.Reset();

            LSV_BEGIN(m_nativeContext, 1);

            lua_pushvalue(m_nativeContext, m_startVariableIndex + functionIndex); // copy the function on the top
            dc.SetCaller(m_nativeContext, lua_gettop(m_nativeContext));

            return true;
        }

        return false;
    }

    //=========================================================================
    // CallStart
    // [4/4/2012]
    //=========================================================================
    bool
        ScriptDataContext::Call(int tableIndex, const char* functionName, ScriptDataContext& dc)
    {
        AZ_Assert(&dc != this, "You must pass a new script data context, for each layer of operation!");
        dc.Reset();

        LSV_BEGIN_VARIABLE(m_nativeContext);

        lua_getfield(m_nativeContext, m_startVariableIndex + tableIndex, functionName);
        if (lua_isfunction(m_nativeContext, -1))
        {
            dc.SetCaller(m_nativeContext, lua_gettop(m_nativeContext));

            LSV_END_VARIABLE(1);
            return true;
        }
        else
        {
            lua_pop(m_nativeContext, 1);
        }

        LSV_END_VARIABLE(0);
        return false;
    }

    //=========================================================================
    // CallExecute
    // [4/4/2012]
    //=========================================================================
    bool ScriptDataContext::CallExecute()
    {
        LSV_BEGIN_VARIABLE(m_nativeContext);

        if (m_mode == MD_CALLER)
        {
            int numArguments = m_numArguments; // used for stack validation
            bool success = Internal::LuaSafeCall(m_nativeContext, m_numArguments, LUA_MULTRET);
            m_numArguments = 0;
            m_mode = MD_CALLER_EXECUTED;
            if (success)
            {
                m_numResults = lua_gettop(m_nativeContext) - m_startVariableIndex + 1;

                LSV_END_VARIABLE(m_numResults - numArguments - 1);
                return true;
            }
        }
        else
        {
            AZ_Assert(false, "To execute a call you must be in CALLER mode! Do you forget to call CallStart?");
        }

        LSV_END_VARIABLE(0);
        return false;
    }

    //////////////////////////////////////////////////////////////////////////
    bool ScriptDataContext::IsFunction(int index) const
    {
        return lua_isfunction(m_nativeContext, m_startVariableIndex + index);
    }

    //////////////////////////////////////////////////////////////////////////
    bool ScriptDataContext::IsCFunction(int index) const
    {
        return lua_iscfunction(m_nativeContext, m_startVariableIndex + index) ? true : false;
    }

    //////////////////////////////////////////////////////////////////////////
    bool ScriptDataContext::IsNumber(int index, bool aceptNumberStrings) const
    {
        if (aceptNumberStrings)
        {
            return lua_isnumber(m_nativeContext, m_startVariableIndex + index) ? true : false;
        }
        else
        {
            return lua_type(m_nativeContext, m_startVariableIndex + index) == LUA_TNUMBER;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    bool ScriptDataContext::IsString(int index, bool acceptNumbers) const
    {
        if (acceptNumbers)
        {
            return lua_isstring(m_nativeContext, m_startVariableIndex + index) ? true : false;
        }
        else
        {
            return lua_type(m_nativeContext, m_startVariableIndex + index) == LUA_TSTRING;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    bool ScriptDataContext::IsBoolean(int index) const
    {
        return lua_isboolean(m_nativeContext, m_startVariableIndex + index);
    }

    //////////////////////////////////////////////////////////////////////////
    bool ScriptDataContext::IsNil(int index) const
    {
        return lua_isnil(m_nativeContext, m_startVariableIndex + index);
    }

    //////////////////////////////////////////////////////////////////////////
    bool ScriptDataContext::IsTable(int index) const
    {
        return lua_istable(m_nativeContext, m_startVariableIndex + index);
    }

    //////////////////////////////////////////////////////////////////////////
    bool ScriptDataContext::IsRegisteredClass(int index) const
    {
        return Internal::LuaIsClass(m_nativeContext, m_startVariableIndex + index, nullptr);
    }

    //////////////////////////////////////////////////////////////////////////
    bool ScriptDataContext::AcquireOwnership(int index, bool isNullPointer) const
    {
        if (IsRegisteredClass(index))
        {
            return Internal::LuaReleaseClassOwnership(m_nativeContext, m_startVariableIndex + index, isNullPointer);
        }
        return false;
    }

    bool ScriptDataContext::ReleaseOwnership(int index) const
    {
        if (IsRegisteredClass(index))
        {
            return Internal::LuaAcquireClassOwnership(m_nativeContext, m_startVariableIndex + index);
        }
        return false;
    }

    //////////////////////////////////////////////////////////////////////////
    AZ::ScriptProperty* ScriptDataContext::ConstructScriptProperty(int index, const char* name, bool restrictToPropertyArrays)
    {
        AZ::ScriptContext* context = GetScriptContext();

        if (context)
        {
            return context->ConstructScriptProperty((*this), index, name, restrictToPropertyArrays);
        }

        return nullptr;
    }

#endif // AZ_USE_CUSTOM_SCRIPT_BIND
} // namespace AZ

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
#include <AzCore/std/allocator_stack.h> // for method integral allocations

#include <AzCore/Script/ScriptContextAttributes.h>

    namespace AZ
    {
        //////////////////////////////////////////////////////////////////////////
        // Custom attributes

        // TODO: Move for users to use
        struct ExampleCustomStorageImplementation // this assumes custom ownership
        {
            // Should we pass typeID with the value
            // How do we push the value from C++ without extra specializations???

            // we will query size to get the storage
            static size_t GetStorageSize(void* value);
            // once we allocate the memory in LUA we will store the value
            static void  StoreValue(void* storage, void* value);
            // we can set the value when need
            static void* GetValue(void* storage);
            // we will call it when LUA no longer needs the value
            static void  DestroyStorage(void* storage);
        };

        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Method bind helper
        namespace Internal
        {
            template<class T>
            bool AllocateTempStorageLuaNative(BehaviorArgument& value, BehaviorClass* valueClass, ScriptContext::StackVariableAllocator& tempAllocator, AZStd::allocator* backupAllocator = nullptr)
            {
                static_assert(AZStd::is_pod<T>::value, "This should be use only for POD data types, as no ctor/dtor is called!");
                (void)valueClass;

                if (value.m_traits & BehaviorParameter::TR_POINTER)
                {
                    // we need value to pointer be pointer to a pointer
                    void* valueAddress = tempAllocator.allocate(sizeof(T) + sizeof(void*), AZStd::alignment_of<T>::value, 0);
                    void* valueAddressPtr = reinterpret_cast<AZ::u8*>(valueAddress)+sizeof(T);
                    *reinterpret_cast<void**>(valueAddressPtr) = valueAddress;
                    value.m_value = valueAddressPtr;
                }
                else // even references are stored by value as we need to convert from lua native type, i.e. there is not real reference for NativeTypes (numbers, strings, etc.)
                {
                    bool usedBackupAlloc = false;
                    if (backupAllocator != nullptr && sizeof(T) > AZStd::allocator_traits<decltype(tempAllocator)>::max_size(tempAllocator))
                    {
                        value.m_value = backupAllocator->allocate(sizeof(T), AZStd::alignment_of<T>::value);
                        usedBackupAlloc = true;
                    }
                    else
                    {
                        value.m_value = tempAllocator.allocate(sizeof(T), AZStd::alignment_of<T>::value);
                    }
                    memset(value.m_value, 0, sizeof(T));
                    return usedBackupAlloc;
                }

                return false;
            }

            bool AllocateTempStorage(BehaviorArgument& value, BehaviorClass* valueClass, ScriptContext::StackVariableAllocator& tempAllocator, AZStd::allocator* backupAllocator = nullptr)
            {
                if (value.m_traits & BehaviorParameter::TR_POINTER)
                {
                    // we need value to pointer be pointer to a pointer
                    void* valueAddress = tempAllocator.allocate(2 * sizeof(void*), valueClass->m_alignment, 0);
                    void* valueAddressPtr = reinterpret_cast<AZ::u8*>(valueAddress)+sizeof(void*);
                    ::memset(valueAddress, 0, sizeof(void*));
                    *reinterpret_cast<void**>(valueAddressPtr) = valueAddress;
                    value.m_value = valueAddressPtr;
                }
                else if (value.m_traits & BehaviorParameter::TR_REFERENCE)
                {
                    // do nothing, we will just assign the reference address
                }
                else // it's a value type
                {
                    bool usedBackupAlloc = false;
                    if (backupAllocator != nullptr && valueClass->m_size > AZStd::allocator_traits<decltype(tempAllocator)>::max_size(tempAllocator))
                    {
                        value.m_value = backupAllocator->allocate(valueClass->m_size, valueClass->m_alignment);
                        usedBackupAlloc = true;
                    }
                    else
                    {
                        value.m_value = tempAllocator.allocate(valueClass->m_size, valueClass->m_alignment);
                    }
                    if (valueClass->m_defaultConstructor)
                    {
                        valueClass->m_defaultConstructor(value.m_value, valueClass->m_userData);
                    }
                    return usedBackupAlloc;
                }

                return false;
            }

            // Helper to do increment operations, but not for bools
            template<class T>
            struct Incrementer
            {
                static void Increment(T& t)
                {
                    t += 1;
                }
                static void Decrement(T& t)
                {
                    t -= 1;
                }
            };
            template<>
            struct Incrementer<bool>
            {
                static void Increment(bool&)
                {
                }
                static void Decrement(bool&)
                {
                }
            };

            // move stack-push and stack read to the type reflection, remove dependency on ScriptValue
            template<class T>
            struct LuaScriptNumber
            {
                typedef T ValueType;
                static bool FromStack(lua_State* lua, int stackIndex, BehaviorArgument& value, BehaviorClass* valueClass, ScriptContext::StackVariableAllocator* tempAllocator)
                {
                    AZ::Locale::ScopedSerializationLocale scopedLocale;

                    value.m_typeId = AzTypeInfo<ValueType>::Uuid(); // we should probably store const &
                    if (value.m_value == nullptr)
                    {
                        AZ_Assert(tempAllocator, "When we don't have the result address ready we need temporary storage! Pass a valid tempData!");
                        AllocateTempStorageLuaNative<T>(value, valueClass, *tempAllocator);
                    }
                    void* valueAddress = value.GetValueAddress();
                    ValueType& actualValue = *reinterpret_cast<ValueType*>(valueAddress);
                    actualValue = ScriptValue<ValueType>::StackRead(lua, stackIndex);

                    // If the value is an index, subtract 1 (1 -> 0)
                    if (value.m_traits & BehaviorParameter::Traits::TR_INDEX)
                    {
                        Incrementer<ValueType>::Decrement(actualValue);
                    }

                    return true;
                }
                static void ToStack(lua_State* lua, BehaviorArgument& value)
                {
                    AZ::Locale::ScopedSerializationLocale scopedLocale;

                    void* valueAddress = value.GetValueAddress();
                    ValueType actualValue = *reinterpret_cast<ValueType*>(valueAddress);

                    // If the value is an index, add 1 (0 -> 1)
                    if (value.m_traits & BehaviorParameter::Traits::TR_INDEX)
                    {
                        Incrementer<ValueType>::Increment(actualValue);
                    }

                    ScriptValue<ValueType>::StackPush(lua, actualValue);
                }

                static bool FromStack(const BehaviorParameter* param, LuaLoadFromStack& arg)
                {
                    if (param->m_typeId == AzTypeInfo<ValueType>::Uuid())
                    {
                        arg = &FromStack;
                        return true;
                    }
                    return false;
                }

                static bool ToStack(const BehaviorParameter* param, LuaPushToStack& toStack, LuaPrepareValue* prepareValue = nullptr)
                {
                    if (param->m_typeId == AzTypeInfo<ValueType>::Uuid())
                    {
                        toStack = &ToStack;
                        if (prepareValue)
                        {
                            *prepareValue = &AllocateTempStorageLuaNative<T>;
                        }
                        return true;
                    }
                    return false;
                }
            };

            struct LuaScriptString
            {
                static bool FromStack(lua_State* lua, int stackIndex, BehaviorArgument& value, BehaviorClass* valueClass, ScriptContext::StackVariableAllocator* tempAllocator)
                {
                    if (lua_isstring(lua, stackIndex))
                    {
                        if (value.m_value == nullptr)
                        {
                            AZ_Assert(tempAllocator, "When we don't have the result address ready we need temporary storage! Pass a valid tempData!");
                            AllocateTempStorageLuaNative<const char*>(value, valueClass, *tempAllocator);
                        }
                        *reinterpret_cast<const char**>(value.m_value) = lua_tostring(lua, stackIndex);
                        value.m_typeId = AzTypeInfo<const char*>::Uuid();
                        return true;
                    }
                    else
                    {
                        value.m_name = luaL_typename(lua, stackIndex);
                        value.m_value = nullptr;
                        value.m_typeId = AZ::Uuid::CreateNull();
                    }
                    return false;
                }

                static void ToStack(lua_State* lua, BehaviorArgument& value)
                {
                    lua_pushstring(lua, *reinterpret_cast<const char**>(value.m_value));
                }

                static bool FromStack(const BehaviorParameter* param, LuaLoadFromStack& arg)
                {
                    if (param->m_typeId == AzTypeInfo<const char*>::Uuid() && (param->m_traits & BehaviorParameter::TR_POINTER)) // treat char pointers as strings
                    {
                        arg = &FromStack;
                        return true;
                    }
                    return false;
                }

                static bool ToStack(const BehaviorParameter* param, LuaPushToStack& toStack, LuaPrepareValue* prepareValue = nullptr)
                {
                    if (param->m_typeId == AzTypeInfo<const char*>::Uuid() && (param->m_traits & BehaviorParameter::TR_POINTER)) // treat char pointers as strings
                    {
                        toStack = &ToStack;
                        if (prepareValue)
                        {
                            *prepareValue = &AllocateTempStorageLuaNative<const char*>;
                        }
                        return true;
                    }
                    return false;
                }
            };

            struct LuaScriptGenericPointer
            {
                static bool FromStack(lua_State* lua, int stackIndex, BehaviorArgument& value, BehaviorClass* valueClass, ScriptContext::StackVariableAllocator* tempAllocator)
                {
                    if (lua_islightuserdata(lua, stackIndex))
                    {
                        if (value.m_value == nullptr)
                        {
                            AZ_Assert(tempAllocator, "When we don't have the result address ready we need temporary storage! Pass a valid tempData!");
                            AllocateTempStorageLuaNative<void*>(value, valueClass, *tempAllocator);
                        }
                        *reinterpret_cast<void**>(value.m_value) = lua_touserdata(lua, stackIndex);
                        return true;
                    }
                    else if (lua_isuserdata(lua, stackIndex))
                    {
                        // it's possible that we have a class that wraps an unregistered class
                        LuaUserData* userData = reinterpret_cast<LuaUserData*>(lua_touserdata(lua, stackIndex));
                        if (userData->magicData == Internal::AZLuaUserData) // make sure this is our user data (4 bytes userdata required)
                        {
                            if (userData->behaviorClass->m_unwrapper && value.m_typeId == userData->behaviorClass->m_wrappedTypeId)
                            {
                                BehaviorObject unwrappedObject;
                                userData->behaviorClass->m_unwrapper(userData->value, unwrappedObject, userData->behaviorClass->m_unwrapperUserData);
                                value.m_typeId = unwrappedObject.m_typeId;

                                if (value.m_traits & BehaviorParameter::TR_POINTER)
                                {
                                    if (value.m_value == nullptr)
                                    {
                                        AZ_Assert(tempAllocator, "When we don't have the result address ready we need temporary storage! Pass a valid tempData!");
                                        AllocateTempStorageLuaNative<void*>(value, valueClass, *tempAllocator);
                                    }
                                    // check custom storage we should get the full value with storage type in case of smart pointers
                                    *reinterpret_cast<void**>(value.m_value) = unwrappedObject.m_address;
                                }
                                else
                                {
                                    value.m_value = unwrappedObject.m_address;
                                }

                                return true;
                            }
                        }
                    }
                    else if (lua_isnil(lua, stackIndex))
                    {
                        if (value.m_value == nullptr)
                        {
                            AZ_Assert(tempAllocator, "When we don't have the result address ready we need temporary storage! Pass a valid tempData!");
                            AllocateTempStorageLuaNative<void*>(value, nullptr, *tempAllocator);
                        }
                        *reinterpret_cast<void**>(value.m_value) = nullptr;
                        return true;
                    }
                    else
                    {
                        value.m_value = nullptr;
                        value.m_typeId = AZ::Uuid::CreateNull();
                    }
                    return false;
                }

                static void ToStack(lua_State* lua, BehaviorArgument& value)
                {
                    lua_pushlightuserdata(lua, *reinterpret_cast<void**>(value.m_value)); // we should pass the type? we will need full userdata
                }

                static bool FromStack(const BehaviorParameter* param, LuaLoadFromStack& arg)
                {
                    (void)param;
                    arg = &FromStack;
                    return true;
                }

                static bool ToStack(const BehaviorParameter* param, LuaPushToStack& toStack, LuaPrepareValue* prepareValue = nullptr)
                {
                    (void)param;
                    toStack = &ToStack;
                    if (prepareValue)
                    {
                        *prepareValue = &AllocateTempStorageLuaNative<void*>;
                    }
                    return true;
                }
            };

            int LuaCacheRegisteredClass(lua_State* lua, void* value, const AZ::Uuid& typeId)
            {
                // If the value is nullptr, treat it as nil
                if (value == nullptr)
                {
                    return LUA_REFNIL;
                }

                BehaviorClass* behaviorClass = nullptr;
                int metatableIndex = -1;

                {
                    lua_rawgeti(lua, LUA_REGISTRYINDEX, AZ_LUA_CLASS_TABLE_REF); // load the class table
                    azlua_pushtypeid(lua, typeId); // add the key for the look up
                    lua_rawget(lua, -2); // load the value for this key
                    if (!lua_istable(lua, -1))
                    {
                        lua_pop(lua, 2); // pop class table and the non table value (should be nil)

                        // since we handle generic pointers elsewhere this should be an asset
                        AZ_Assert(false, "We should always have metatable as this function is called for reflected types, we handle this outside");
                        return LUA_REFNIL;
                    }
                    else
                    {
                        metatableIndex = lua_gettop(lua);
                    }
                }

                //////////////////////////////////////////////////////////////////////////
                // Read Class Attributes

                // get the behavior class
                lua_rawgeti(lua, metatableIndex, AZ_LUA_CLASS_METATABLE_BEHAVIOR_CLASS);
                behaviorClass = reinterpret_cast<BehaviorClass*>(lua_touserdata(lua, -1));

                lua_pop(lua, 1); // pop behavior class and storage type
                //////////////////////////////////////////////////////////////////////////

                // <UserData>
                LuaUserData* userData = reinterpret_cast<LuaUserData*>(lua_newuserdata(lua, sizeof(LuaUserData)));

                userData->magicData = Internal::AZLuaUserData;
                userData->behaviorClass = behaviorClass;
                userData->value = value;
                userData->storageType = (int)Script::Attributes::StorageType::RuntimeOwn;

#if defined(LUA_USERDATA_TRACKING)
                if (userData->behaviorClass->m_typeId == azrtti_typeid<AZStd::vector<AZ::EntityId>>())
                {
                    ScriptContext::FromNativeContext(lua)->AddAllocation(userData);
                }
#endif//defined(LUA_USERDATA_TRACKING)

                // <table>
                lua_pushvalue(lua, metatableIndex); // copy the metatable on the top of the stack
                lua_setmetatable(lua, -2);
                // </table>

                // </UserData>

                lua_replace(lua, -3); // replace the class table with the user data
                lua_pop(lua, 1); // and pop metatable

                return luaL_ref(lua, LUA_REGISTRYINDEX);
            }

            void RegisteredObjectToLua(lua_State* lua, void* value, const AZ::Uuid& typeId, ObjectToLua toLua, AcquisitionOnPush acquitionOnPush = AcquisitionOnPush::None)
            {
                LSV_BEGIN(lua, 1);

                // If the value is nullptr, treat it as nil
                if (value == nullptr)
                {
                    lua_pushnil(lua);
                    return;
                }

                BehaviorClass* behaviorClass = nullptr;
                int metatableIndex = -1;

                {
                    lua_rawgeti(lua, LUA_REGISTRYINDEX, AZ_LUA_CLASS_TABLE_REF); // load the class table
                    azlua_pushtypeid(lua, typeId); // add the key for the look up
                    lua_rawget(lua, -2); // load the value for this key
                    if (!lua_istable(lua, -1))
                    {
                        lua_pop(lua, 2); // pop class table and the non table value (should be nil)

                        ScriptContext* scriptContext = ScriptContext::FromNativeContext(lua);
                        AZ_Error("ScriptContext", scriptContext, "Unable to get the lua ScriptContext");

                        if (scriptContext)
                        {
                            scriptContext->Error(ScriptContext::ErrorType::Error,
                                true /*print callstack*/,
                                "Object has type Id %s, which is not reflected to the BehaviorContext, or it has the Script::Attributes::Ignore attribute assigned. Nil will be returned.",
                                typeId.ToString<AZStd::string>().c_str());
                        }

                        lua_pushnil(lua);
                        return;
                    }
                    else
                    {
                        metatableIndex = lua_gettop(lua);
                    }
                }

                //////////////////////////////////////////////////////////////////////////
                // Read Class Attributes

                // get the behavior class
                lua_rawgeti(lua, metatableIndex, AZ_LUA_CLASS_METATABLE_BEHAVIOR_CLASS);
                behaviorClass = reinterpret_cast<BehaviorClass*>(lua_touserdata(lua, -1));

                // get storage type
                lua_rawgeti(lua, metatableIndex, AZ_LUA_CLASS_METATABLE_STORAGE_TYPE_INDEX);
                Script::Attributes::StorageType storageType = (Script::Attributes::StorageType)lua_tointeger(lua, -1);

                lua_pop(lua, 2); // pop behavior class and storage type
                //////////////////////////////////////////////////////////////////////////

                bool isUserdataTrackingRequired = true;
                size_t valueSize = 0;
                if (toLua == ObjectToLua::ByValue || storageType == Script::Attributes::StorageType::Value)
                {
                    if (behaviorClass->m_alignment > 16)
                    {
                        AZ_Error("Script", false, "You are we are passing type %s by value, alignment is %d we support value types in lua only with alignment < 16 bytes!", typeId.ToString<AZStd::string>().c_str(), behaviorClass->m_alignment);
                        lua_pushnil(lua);
                        return;
                    }

                    if (!behaviorClass->m_cloner)
                    {
                        AZ_Error("Script", false, "You are we are passing type %s by value, but copy constructor is not available!", typeId.ToString<AZStd::string>().c_str());
                        lua_pushnil(lua);
                        return;
                    }

                    valueSize += behaviorClass->m_size;
                    // make sure we can properly align the userdata
                    if (behaviorClass->m_alignment > sizeof(L_Umaxalign))
                    {
                        valueSize += behaviorClass->m_alignment - sizeof(L_Umaxalign);
                    }

                    isUserdataTrackingRequired = storageType != Script::Attributes::StorageType::Value;
                }

                if (isUserdataTrackingRequired)
                {
                    // check if we already track this pointer in the lua VM
                    lua_rawgeti(lua, metatableIndex, AZ_LUA_CLASS_METATABLE_INSTANCES); // load the instance table
                    lua_pushlightuserdata(lua, value); // load the address
                    lua_rawget(lua, -2); // get the cached userdata
                    if (lua_isuserdata(lua, -1))
                    {
                        lua_remove(lua, -2); // remove the metatable instances
                        lua_remove(lua, -2); // remove the metatable
                        lua_remove(lua, -2); // remove the class table
                        return;
                    }
                    else
                    {
                        lua_pop(lua, 2); // pop resulting value and the instance metatable.
                    }
                }

                LuaUserData* userData = reinterpret_cast<LuaUserData*>(lua_newuserdata(lua, sizeof(LuaUserData) + valueSize));

                if (valueSize)
                {
                    storageType = Script::Attributes::StorageType::Value;
                    void* newValue = userData + 1; // value is after the user data
                    newValue = PointerAlignUp(newValue, behaviorClass->m_alignment);
                    behaviorClass->m_cloner(newValue, value, behaviorClass->m_userData);
                    value = newValue;
                }
                else if (toLua == ObjectToLua::ByReference && acquitionOnPush == AcquisitionOnPush::ScriptAcquire)
                {
                    storageType = Script::Attributes::StorageType::ScriptOwn;
                }
                else
                {
                    // we own only the object that we create, not the one that are passed in, unless the
                    // the developer explicit calls \ref ScrtiptDataContext::ReleaseOwnership.
                    storageType = Script::Attributes::StorageType::RuntimeOwn;
                }

                userData->magicData = Internal::AZLuaUserData;
                userData->behaviorClass = behaviorClass;
                userData->value = value;
                userData->storageType = (int)storageType;

#if defined(LUA_USERDATA_TRACKING)
                if (userData->behaviorClass->m_typeId == azrtti_typeid<AZStd::vector<AZ::EntityId>>())
                {
                    ScriptContext::FromNativeContext(lua)->AddAllocation(userData);
                }
#endif//defined(LUA_USERDATA_TRACKING)

                if (isUserdataTrackingRequired)
                {
                    lua_rawgeti(lua, metatableIndex, AZ_LUA_CLASS_METATABLE_INSTANCES); // load the instance table
                    lua_pushlightuserdata(lua, value); // load the address
                    lua_pushvalue(lua, -3); // copy the value
                    lua_rawset(lua, -3); // store value reference into the instance table
                    lua_pop(lua, 1); /// pop the instance table
                }

                lua_pushvalue(lua, metatableIndex); // copy the metatable on the top of the stack
                lua_setmetatable(lua, -2);
                // leave the user data on the stack

                lua_replace(lua, -3); // replace the class table with the user data
                lua_pop(lua, 1); // and pop metatable
            }

            struct LuaScriptReflectedType
            {
                static bool FromStack(lua_State* lua, int stackIndex, BehaviorArgument& value, BehaviorClass* valueClass, ScriptContext::StackVariableAllocator* tempAllocator)
                {
                    if (lua_isuserdata(lua, stackIndex))
                    {
                        LuaUserData* userData = reinterpret_cast<LuaUserData*>(lua_touserdata(lua, stackIndex));
                        if (userData->magicData == Internal::AZLuaUserData) // make sure this is our user data (4 bytes userdata required)
                        {
                            // Handle wrapped base classes
                            bool unwrap = false;
                            // If we have an unwrapper, see if it is for the value type
                            if (userData->behaviorClass->m_unwrapper && value.m_typeId != userData->behaviorClass->m_typeId)
                            {
                                if (unwrap = value.m_typeId == userData->behaviorClass->m_wrappedTypeId; !unwrap)
                                {
                                    const AZ::BehaviorClass* unwrappedClass = AZ::BehaviorContextHelper::GetClass(value.m_typeId);
                                    unwrap = unwrappedClass && unwrappedClass->m_azRtti && unwrappedClass->m_azRtti->ProvidesFullRtti() && unwrappedClass->m_azRtti->IsTypeOf(valueClass->m_typeId);
                                }
                            }

                            void* valueAddress = nullptr;

                            // If we found a proper unwrapper, use it
                            if (unwrap)
                            {
                                BehaviorObject unwrappedObject;
                                userData->behaviorClass->m_unwrapper(userData->value, unwrappedObject, userData->behaviorClass->m_unwrapperUserData);
                                valueAddress = unwrappedObject.m_address;
                                value.m_typeId = unwrappedObject.m_typeId;
                            }
                            else
                            {
                                // Update type name for use in type checking and error messages
                                value.m_name = userData->behaviorClass->m_name.c_str();
                                value.m_typeId = userData->behaviorClass->m_typeId;

                                // Check that the type of user data is the type requested (or convertible to it)
                                if (value.m_typeId != valueClass->m_typeId && (!userData->behaviorClass->m_azRtti || !userData->behaviorClass->m_azRtti->IsTypeOf(valueClass->m_typeId)))
                                {
                                    return false;
                                }
                                valueAddress = userData->value;
                            }

                            if (value.m_traits & BehaviorParameter::TR_POINTER)
                            {
                                if (value.m_value == nullptr)
                                {
                                    AZ_Assert(tempAllocator, "When we don't have the result address ready we need temporary storage! Pass a valid tempData!");
                                    AllocateTempStorage(value, userData->behaviorClass, *tempAllocator);
                                }
                                // check custom storage we should get the full value with storage type in case of smart pointers
                                *reinterpret_cast<void**>(value.m_value) = valueAddress;
                            }
                            else
                            {
                                value.m_value = valueAddress;
                            }

                            return true;
                        }
                    }
                    else if (lua_isnil(lua, stackIndex))
                    {
                        // Allow default construct from nil if the class has the appropriate shift
                        AZ::AttributeReader reader(nullptr, FindAttribute(AZ::Script::Attributes::ConstructibleFromNil, valueClass->m_attributes));
                        bool constructNil = false;
                        reader.Read<bool>(constructNil);

                        if (value.m_traits & BehaviorParameter::TR_POINTER)
                        {
                            // here we have a pointer (which can be null)
                            AZ_Assert(tempAllocator, "When we don't have the result address ready we need temporary storage! Pass a valid tempData!");
                            AllocateTempStorage(value, valueClass, *tempAllocator);
                            *reinterpret_cast<void**>(value.m_value) = nullptr;
                            return true;
                        }
                        else if (constructNil)
                        {
                            // here we have a value (which can be constructed to null)
                            AZ_Assert(tempAllocator, "When we don't have the result address ready we need temporary storage! Pass a valid tempData!");
                            AllocateTempStorage(value, valueClass, *tempAllocator);
                            return true;
                        }
                        else
                            {
                            // we can't have nil references or values
                            value.m_name = "nil";
                            value.m_typeId = AZ::Uuid::CreateNull(); // set the type to invalid to trigger parameter mismatch error
                            value.m_value = nullptr;
                        }
                    }
                    else
                    {
                        // We have a LUA native type, which we can't convert as a registered class type (which is what this function is for)
                        // If you want to convert from native types, insert your own Lua VM read/write function \ref AZStd::basic_string reflection
                        value.m_name = luaL_typename(lua, stackIndex);
                        value.m_typeId = AZ::Uuid::CreateNull();
                        value.m_value = nullptr;
                    }
                    return false;
                }
                static void ToStack(lua_State* lua, BehaviorArgument& value)
                {
                    void* valueAddress = value.m_value;
                    ObjectToLua toLua = ObjectToLua::ByReference;
                    if (value.m_traits & BehaviorParameter::TR_POINTER) // TODO: Use value->GetUnsafe in BehaviorArgument if we can adjust it for TR_POINTER
                    {
                        // we have pointer to a pointer
                        valueAddress = *reinterpret_cast<void**>(valueAddress);
                    }
                    else if ((value.m_traits & BehaviorParameter::TR_REFERENCE) == 0)
                    {
                        toLua = ObjectToLua::ByValue; // references are passed by pointer
                    }

                    RegisteredObjectToLua(lua, valueAddress, value.m_typeId, toLua);
                }
                static bool FromStack(LuaLoadFromStack& arg)
                {
                    arg = &FromStack;
                    return true;
                }
                static bool ToStack(LuaPushToStack& toStack, LuaPrepareValue* prepareValue = nullptr)
                {
                    toStack = &ToStack;
                    if (prepareValue)
                    {
                        *prepareValue = &AllocateTempStorage;
                    }
                    return true;
                }
            };

            int LuaCacheValue(lua_State* lua, int index)
            {
                LSV_BEGIN(lua, 0);

                // Early out for nils
                if (lua_isnil(lua, index))
                {
                    return LUA_REFNIL;
                }

                // Push the value on top of the stack, so that relative indices work (before we munge the stack with other stuff
                // #LUA If upgrading to lua 5.2 or greater, replace with `index = lua_absindex(lua, index);`
                lua_pushvalue(lua, index);
                int value = lua_gettop(lua);
                lua_rawgeti(lua, LUA_REGISTRYINDEX, AZ_LUA_WEAK_CACHE_TABLE_REF); // Load the weak cache table
                int weakCacheTable = lua_gettop(lua);

                // Get first free list element
                lua_rawgeti(lua, weakCacheTable, WeakCacheFreeListIndex);
                int ref = (int)lua_tointeger(lua, -1);
                lua_pop(lua, 1);

                // Get the next element in the free list
                lua_rawgeti(lua, weakCacheTable, ref);
                // If next ref is nil, it means we're at the end, and should just go to the next element in the table
                if (lua_isnil(lua, -1))
                {
                    lua_pop(lua, 1);

                    // Insert the new ref as the max index used
                    lua_pushinteger(lua, ref);
                    lua_rawseti(lua, weakCacheTable, WeakCacheMaxUsedIndex);

                    lua_pushinteger(lua, ref + 1);
                }
                // Set the next index
                lua_rawseti(lua, weakCacheTable, WeakCacheFreeListIndex);

                // Push the value given, and place it in weakCacheTable[ref]
                lua_pushvalue(lua, value);
                lua_rawseti(lua, weakCacheTable, ref);

                lua_pop(lua, 2); // Pop the cache table and the copy of value

                return ref;
            }

            void LuaLoadCached(lua_State* lua, int cachedIndex)
            {
                LSV_BEGIN(lua, 1);

                if (cachedIndex < WeakCacheUser)
                {
                    lua_pushnil(lua);
                }
                else
                {
                    lua_rawgeti(lua, LUA_REGISTRYINDEX, AZ_LUA_WEAK_CACHE_TABLE_REF); // Load the weak cache table
                    lua_rawgeti(lua, -1, cachedIndex); // get the value
                    lua_remove(lua, -2); // remove the cache table
                }
            }

            void LuaReleaseCached(lua_State* lua, int cachedIndex)
            {
                LSV_BEGIN(lua, 0);

                if (cachedIndex >= WeakCacheUser)
                {
                    lua_rawgeti(lua, LUA_REGISTRYINDEX, AZ_LUA_WEAK_CACHE_TABLE_REF); // Load the weak cache table
                    int weakCacheTable = lua_gettop(lua);

                    lua_rawgeti(lua, weakCacheTable, WeakCacheMaxUsedIndex);
                    int maxIndex = (int)lua_tointeger(lua, -1);
                    lua_pop(lua, 1);

                    AZ_Assert(cachedIndex <= maxIndex, "Internal error: cachedIndex has already been freed.");

                    // If not unrefing the end, just update the list
                    if (cachedIndex != maxIndex)
                    {
                        lua_rawgeti(lua, weakCacheTable, WeakCacheFreeListIndex);
                        lua_rawseti(lua, weakCacheTable, cachedIndex); // weakCacheTable[cachedIndex] = weakCacheTable[WeakCacheFreeListIndex]
                        lua_pushinteger(lua, cachedIndex);
                        lua_rawseti(lua, weakCacheTable, WeakCacheFreeListIndex); // weakCacheTable[WeakCacheFreeListIndex] = cachedIndex
                    }
                    else // If unrefing the last element, trim the list
                    {
                        // Map of index -> is free list node
                        // +1 size to mirror Lua's 1-based indexing
                        AZStd::vector<bool> isFreeListNode(maxIndex + 1, false);

                        // Iterate from the free list, traversing the list and setting the flag
                        // Stop when index == maxIndex + 1, as that's the sentinel
                        for (int index = WeakCacheFreeListIndex; index != maxIndex + 1; )
                        {
                            isFreeListNode[index] = true;

                            lua_rawgeti(lua, weakCacheTable, index);
                            index = (int)lua_tointeger(lua, -1);
                            lua_pop(lua, 1);
                        }
                        isFreeListNode[maxIndex] = true; // We know maxIndex is being unrefed, so explictly mark it as free

                        // Iterate backwards through the list, and trim elements until we find data
                        int index = maxIndex;
                        for (; index > WeakCacheFreeListIndex && isFreeListNode[index]; --index)
                        {
                            // Setting the element to nil clears the reference and lets the GC take over
                            lua_pushnil(lua);
                            lua_rawseti(lua, weakCacheTable, index);
                        }

                        // We've found the first data element (or FreeListIndex), so store as max used
                        lua_pushinteger(lua, index);
                        lua_rawseti(lua, weakCacheTable, WeakCacheMaxUsedIndex);

                        // The first free list node we find (so the last one in order)
                        // should point to the element after the last used element
                        int previousFreeNode = index + 1;

                        // Keep iterating backwards, and update references
                        for (; index >= WeakCacheFreeListIndex; --index)
                        {
                            // If this index is a free list node, set it to point to the last free list node we found
                            if (isFreeListNode[index])
                            {
                                lua_pushinteger(lua, previousFreeNode);
                                lua_rawseti(lua, weakCacheTable, index);

                                // Reset so the next node we find points to this one
                                previousFreeNode = index;
                            }
                        }
                    }

                    lua_pop(lua, 1); // Pop the cache table
                }
            }

            // Helper functions
            bool LuaIsClass(lua_State* lua, int stackIndex, const AZ::Uuid* typeId)
            {
                if (lua_isuserdata(lua, stackIndex) && !lua_islightuserdata(lua, stackIndex))
                {
                    LuaUserData* userData = reinterpret_cast<LuaUserData*>(lua_touserdata(lua, stackIndex));
                    if (userData->magicData == Internal::AZLuaUserData)
                    {
                        if (typeId)
                        {
                            return userData->behaviorClass->m_typeId == *typeId;
                        }
                        else
                        {
                            return true;
                        }
                    }
                }
                return false;
            }

            bool LuaGetClassInfo(lua_State* lua, int stackIndex, void** valueAddress, const BehaviorClass** behaviorClass)
            {
                if (lua_isuserdata(lua, stackIndex) && !lua_islightuserdata(lua, stackIndex))
                {
                    LuaUserData* userData = reinterpret_cast<LuaUserData*>(lua_touserdata(lua, stackIndex));
                    if (userData->magicData == Internal::AZLuaUserData)
                    {
                        if (valueAddress)
                        {
                            *valueAddress = userData->value;
                        }

                        if (behaviorClass)
                        {
                            *behaviorClass = userData->behaviorClass;
                        }

                        return true;
                    }
                }
                return false;
            }

            bool LuaReleaseClassOwnership(lua_State* lua, int stackIndex, bool nullThePointer)
            {
                if (lua_isuserdata(lua, stackIndex) && !lua_islightuserdata(lua, stackIndex))
                {
                    LuaUserData* userData = reinterpret_cast<LuaUserData*>(lua_touserdata(lua, stackIndex));
                    if (userData->magicData == Internal::AZLuaUserData)
                    {
                        userData->storageType = (int)Script::Attributes::StorageType::RuntimeOwn;
                        if (nullThePointer)
                        {
                            userData->value = nullptr;
                        }
                        return true;
                    }
                }
                return false;
            }

            bool LuaAcquireClassOwnership(lua_State* lua, int stackIndex)
            {
                if (lua_isuserdata(lua, stackIndex) && !lua_islightuserdata(lua, stackIndex))
                {
                    LuaUserData* userData = reinterpret_cast<LuaUserData*>(lua_touserdata(lua, stackIndex));
                    if (userData->magicData == Internal::AZLuaUserData)
                    {
                        userData->storageType = (int)Script::Attributes::StorageType::ScriptOwn;
                        return true;
                    }
                }
                return false;
            }

            void* LuaClassFromStack(lua_State* lua, int stackIndex, const AZ::Uuid& typeId)
            {
                if (lua_isuserdata(lua, stackIndex) && !lua_islightuserdata(lua, stackIndex))
                {
                    LuaUserData* userData = reinterpret_cast<LuaUserData*>(lua_touserdata(lua, stackIndex));
                    if (userData->magicData == Internal::AZLuaUserData)
                    {
                        if (userData->behaviorClass->m_typeId == typeId
                        || (userData->behaviorClass->m_azRtti && userData->behaviorClass->m_azRtti->IsTypeOf(typeId)))
                        {
                            return userData->value;
                        }
                    }
                }
                return nullptr;
            }

            void* LuaAnyClassFromStack(lua_State* lua, int stackIndex, AZ::Uuid* typeId)
            {
                if (lua_isuserdata(lua, stackIndex) && !lua_islightuserdata(lua, stackIndex))
                {
                    LuaUserData* userData = reinterpret_cast<LuaUserData*>(lua_touserdata(lua, stackIndex));
                    if (userData->magicData == Internal::AZLuaUserData)
                    {
                        if (typeId)
                        {
                            *typeId = userData->behaviorClass->m_typeId;
                        }

                        return userData->value;
                    }
                }
                return nullptr;
            }

            void LuaClassToStack(lua_State* lua, void* valueAddress, const AZ::Uuid& typeId, ObjectToLua toLua, AcquisitionOnPush acquitionOnPush)
            {
                RegisteredObjectToLua(lua, valueAddress, typeId, toLua, acquitionOnPush);
            }

            //////////////////////////////////////////////////////////////////////////
            // Helpers for LoadFromStream
            // Number of bytes to read from script at a time
            static const u64 s_luaFileChunkSize = 1024;

            // User data to track read progress
            struct LuaReadUserData
            {
                AZStd::string m_fileChunk;
                IO::GenericStream* m_fileStream = nullptr;
                u64 m_bytesLeftInFile = 0;
            };

            // lua_Reader function for reading a file piece by piece
            static const char* LuaStreamReader(lua_State*, void* userData, size_t* size)
            {
                LuaReadUserData* fileData = static_cast<LuaReadUserData*>(userData);

                // If done loading file, set size to 0 and return
                if (fileData->m_bytesLeftInFile == 0)
                {
                    *size = 0;
                    return nullptr;
                }

                u64 bytesToRead = fileData->m_bytesLeftInFile > s_luaFileChunkSize ? s_luaFileChunkSize : fileData->m_bytesLeftInFile;

                u64 actualBytesRead = fileData->m_fileStream->Read(bytesToRead, fileData->m_fileChunk.data());
                fileData->m_bytesLeftInFile -= actualBytesRead;

                *size = aznumeric_caster(actualBytesRead);
                return fileData->m_fileChunk.data();
            }

            static int LuaRequireHandler(lua_State* l)
            {
                auto context = static_cast<ScriptContext*>(lua_touserdata(l, lua_upvalueindex(1)));
                auto userFunction = reinterpret_cast<ScriptContext::RequireHook*>(lua_touserdata(l, lua_upvalueindex(2)));

                return (*userFunction)(l, context, luaL_checkstring(l, -1));
            }
        } //namespace Internal

        LuaLoadFromStack FromLuaStack(BehaviorContext* context, const BehaviorParameter* param, BehaviorClass*& behaviorClass)
        {
            LuaLoadFromStack result = nullptr;
            behaviorClass = nullptr;
            // handle integral types
            if (Internal::LuaScriptNumber<bool>::FromStack(param, result)) return result;
            else if (Internal::LuaScriptNumber<short>::FromStack(param, result)) return result;
            else if (Internal::LuaScriptNumber<AZ::s64>::FromStack(param, result)) return result;
            else if (Internal::LuaScriptNumber<long>::FromStack(param, result)) return result;
            else if (Internal::LuaScriptNumber<AZ::s8>::FromStack(param, result)) return result;
            else if (Internal::LuaScriptNumber<unsigned char>::FromStack(param, result)) return result;
            else if (Internal::LuaScriptNumber<unsigned short>::FromStack(param, result)) return result;
            else if (Internal::LuaScriptNumber<unsigned int>::FromStack(param, result)) return result;
            else if (Internal::LuaScriptNumber<AZ::u64>::FromStack(param, result)) return result;
            else if (Internal::LuaScriptNumber<unsigned long>::FromStack(param, result)) return result;
            else if (Internal::LuaScriptNumber<float>::FromStack(param, result)) return result;
            else if (Internal::LuaScriptNumber<double>::FromStack(param, result)) return result;
            else if (Internal::LuaScriptNumber<int>::FromStack(param, result)) return result;
            else if (Internal::LuaScriptString::FromStack(param, result)) return result;
            else if (Internal::LuaScriptNumber<char>::FromStack(param, result)) return result;

            auto classIt = context->m_typeToClassMap.find(param->m_typeId);
            if (classIt == context->m_typeToClassMap.end())
            {
                // not reflected type tread it as a void pointer (with TypeID???)
            }
            else
            {
                behaviorClass = classIt->second;

                // check custom attributes
                Attribute* readerWriterAttribute = FindAttribute(Script::Attributes::ReaderWriterOverride, behaviorClass->m_attributes);
                if (readerWriterAttribute)
                {
                    ScriptContext::CustomReaderWriter readerWriter(nullptr,nullptr);
                    AttributeReader customReaderWriter(nullptr, readerWriterAttribute);
                    customReaderWriter.Read<ScriptContext::CustomReaderWriter>(readerWriter);

                    result = readerWriter.m_fromLua;
                }
                else
                {
                    // handle specific pointer types
                    Internal::LuaScriptReflectedType::FromStack(result);
                }

                return result;
            }

            // generic pointer types
            if (param->m_traits & BehaviorParameter::TR_POINTER)
            {
                Internal::LuaScriptGenericPointer::FromStack(param, result);
            }

            return result;
        }

        bool StackRead(lua_State* lua, int index, AZ::BehaviorContext* context, AZ::BehaviorArgument& param, AZ::StackVariableAllocator* allocator)
        {
            AZ::BehaviorClass* bcClass = nullptr;
            LuaLoadFromStack fromStack = FromLuaStack(context, &param, bcClass);
            return fromStack(lua, index, param, bcClass, allocator);
        }

        bool StackRead(lua_State* lua, int index, AZ::BehaviorArgument& param, AZ::StackVariableAllocator* allocator)
        {
            return StackRead(lua, index, ScriptContext::FromNativeContext(lua)->GetBoundContext(), param, allocator);
        }

        void StackPush(lua_State* lua, AZ::BehaviorContext* context, AZ::BehaviorArgument& param)
        {
            AZ::BehaviorClass* unused = nullptr;
            LuaPrepareValue prepareValue = nullptr;
            LuaPushToStack pushToStack = ToLuaStack(context, &param, &prepareValue, unused);
            AZ_Assert(pushToStack != nullptr, "No LuaPushToStack function found for typeid: %s", param.m_typeId.ToString<AZStd::string>().data());
            pushToStack(lua, param);
        }

        void StackPush(lua_State* lua, AZ::BehaviorArgument& param)
        {
            StackPush(lua, ScriptContext::FromNativeContext(lua)->GetBoundContext(), param);
        }

        LuaPushToStack ToLuaStack(BehaviorContext* context, const BehaviorParameter* param, LuaPrepareValue* prepareParam, BehaviorClass*& behaviorClass)
        {
            LuaPushToStack result = nullptr;
            behaviorClass = nullptr;
            // handle integral types
            if (Internal::LuaScriptNumber<bool>::ToStack(param, result, prepareParam)) return result;
            else if (Internal::LuaScriptNumber<short>::ToStack(param, result, prepareParam)) return result;
            else if (Internal::LuaScriptNumber<int>::ToStack(param, result, prepareParam)) return result;
            else if (Internal::LuaScriptNumber<AZ::s64>::ToStack(param, result, prepareParam)) return result;
            else if (Internal::LuaScriptNumber<long>::ToStack(param, result, prepareParam)) return result;
            else if (Internal::LuaScriptNumber<unsigned char>::ToStack(param, result, prepareParam)) return result;
            else if (Internal::LuaScriptNumber<unsigned short>::ToStack(param, result, prepareParam)) return result;
            else if (Internal::LuaScriptNumber<unsigned int>::ToStack(param, result, prepareParam)) return result;
            else if (Internal::LuaScriptNumber<AZ::u64>::ToStack(param, result, prepareParam)) return result;
            else if (Internal::LuaScriptNumber<unsigned long>::ToStack(param, result, prepareParam)) return result;
            else if (Internal::LuaScriptNumber<float>::ToStack(param, result, prepareParam)) return result;
            else if (Internal::LuaScriptNumber<double>::ToStack(param, result, prepareParam)) return result;
            else if (Internal::LuaScriptString::ToStack(param, result, prepareParam)) return result;
            else if (Internal::LuaScriptNumber<char>::ToStack(param, result, prepareParam)) return result;

            // todo AZStd::string and generic strings???

            if (!result)
            {
                auto classIt = context->m_typeToClassMap.find(param->m_typeId);
                if (classIt == context->m_typeToClassMap.end())
                {
                    // not reflected type tread it as a void pointer (with TypID???)
                }
                else
                {
                    behaviorClass = classIt->second;

                    // check custom attributes
                    Attribute* readerWriterAttribute = FindAttribute(Script::Attributes::ReaderWriterOverride, behaviorClass->m_attributes);
                    if (readerWriterAttribute)
                    {
                        ScriptContext::CustomReaderWriter readerWriter(nullptr, nullptr);
                        AttributeReader customReaderWriter(nullptr, readerWriterAttribute);
                        customReaderWriter.Read<ScriptContext::CustomReaderWriter>(readerWriter);

                        result = readerWriter.m_toLua;
                        if (prepareParam)
                        {
                            *prepareParam = &Internal::AllocateTempStorage;
                        }
                    }
                    else
                    {
                        // handle specific pointer types
                        Internal::LuaScriptReflectedType::ToStack(result, prepareParam);
                    }
                    return result;
                }
            }

            // generic pointer types
            if (param->m_traits & BehaviorParameter::TR_POINTER)
            {
                Internal::LuaScriptGenericPointer::ToStack(param, result, prepareParam);
            }

            return result;
        }

        class LuaScriptCaller : public LuaCaller
        {
        public:
            AZ_CLASS_ALLOCATOR(LuaScriptCaller, AZ::SystemAllocator);

            LuaScriptCaller(BehaviorContext* context, BehaviorMethod* method)
            {
                (void)context;
                m_method = method;

                for (int iArg = 0; iArg < static_cast<int>(m_method->GetNumArguments()); ++iArg)
                {
                    const BehaviorParameter* arg = method->GetArgument(iArg);
                    BehaviorClass* argClass = nullptr;
                    LuaLoadFromStack fromStack = FromLuaStack(context, arg, argClass);
                    AZ_Assert(fromStack,
                        "The argument type: %s for method: %s is not serialized and/or reflected for scripting.\n"
                        "Make sure %s is added to the SerializeContext and reflected to the BehaviorContext\n"
                        "For example, verify these two exist and are being called in a Reflect function:\n"
                        "serializeContext->Class<%s>();\n"
                        "behaviorContext->Class<%s>();\n"
                        "%s will not be available for scripting unless these requirements are met."
                        , arg->m_name, method->m_name.c_str(), arg->m_name, arg->m_name, arg->m_name, method->m_name.c_str());

                    m_fromLua.push_back(AZStd::make_pair(fromStack, argClass));
                }

                if (method->HasResult())
                {
                    m_resultToLua = ToLuaStack(context, method->GetResult(), &m_prepareResult, m_resultClass);
                }
                else
                {
                    m_resultToLua = nullptr;
                }
            }

            int ManualCall(lua_State* lua) override
            {
                return Call(lua);
            }

            void PushClosure(lua_State* lua, const char* debugDescription) override
            {
                LSV_BEGIN(lua, 1);

                lua_pushlightuserdata(lua, this); // if there is no reason to keep the data in the "methods" we can use full user data and rely on __gc to clean it
                lua_pushstring(lua, debugDescription);
                lua_pushcclosure(lua, &Internal::LuaMethodTagHelper, 0);
                lua_pushcclosure(lua, &LuaScriptCaller::Call, 3);
            }

            static int Call(lua_State* lua)
            {
                LuaScriptCaller* thisPtr = reinterpret_cast<LuaScriptCaller*>(lua_touserdata(lua, lua_upvalueindex(1)));

                // check number of arguments
                int numElementsOnStack = lua_gettop(lua);
                if (numElementsOnStack < static_cast<int>(thisPtr->m_method->GetMinNumberOfArguments()))
                {
                    // we can here load default parameters
                    ScriptContext::FromNativeContext(lua)->Error(ScriptContext::ErrorType::Error, true, "Not enough arguments for %s(%s) method, we expected %d arguments (left to right), provided %d!", thisPtr->m_method->m_name.c_str(), lua_tostring(lua, lua_upvalueindex(2)), thisPtr->m_method->GetMinNumberOfArguments(), numElementsOnStack);
                    return 0;
                }

                // there's no limit inherently in BehaviorContext (as there is no document limit in C++), but the LY supported limits default to 40 for Lua, ScriptCanvas, and ScriptEvents.
                // this limit of 40 is however implicit, for now.
                BehaviorArgument arguments[40];
                BehaviorArgument result;
                ScriptContext::StackVariableAllocator tempData;
                AZStd::allocator backupAllocator;
                bool usedBackupAlloc  = false;

                int numArguments = GetMin(static_cast<int>(thisPtr->m_method->GetNumArguments()), numElementsOnStack);
                AZ_Assert(static_cast<int>(AZ_ARRAY_SIZE(arguments)) >= numArguments, "Increase the argument array size!");

                // for each argument read a variable from the stack to a BehaviorArgument
                for (int i = 0; i < numArguments; ++i)
                {
                    const AZ::BehaviorParameter* parameter = thisPtr->m_method->GetArgument(i);
                    arguments[i].Set(*parameter); // store the type of result we expect (pointer, const, etc.)
                    if (!thisPtr->m_fromLua[i].first(lua, i + 1, arguments[i], thisPtr->m_fromLua[i].second, &tempData))
                    {
                        ScriptContext::FromNativeContext(lua)->Error(ScriptContext::ErrorType::Error, true, "Lua failed to call method: cannot convert parameter %d from %s to %s",
                            i + 1, arguments[i].m_name, parameter->m_name);
                        return 0;
                    }
                }

                // If this pointer passed, ensure it isn't nil
                if (thisPtr->m_method->IsMember() &&
                    *arguments[0].GetAsUnsafe<void*>() == nullptr)
                {
                    ScriptContext::FromNativeContext(lua)->Error(ScriptContext::ErrorType::Error, true, "Cannot pass nil as 'this' ptr to member function %s.", thisPtr->m_method->m_name.c_str());
                    return 0;
                }
                int numResults = 0;

                if (thisPtr->m_resultToLua)
                {
                    result.Set(*thisPtr->m_method->GetResult());

                    if (thisPtr->m_prepareResult)
                    {
                        usedBackupAlloc  = thisPtr->m_prepareResult(result, thisPtr->m_resultClass, tempData, &backupAllocator); // pass temp memory and class info
                    }

                    // TODO: Make it optional for EBuses only, make it light weight too, probably a virtual function for the store result.
                    result.m_onAssignedResult = AZStd::function<void()>([&]()
                    {
                        if (result.m_value)
                        {
                            thisPtr->m_resultToLua(lua, result);
                            ++numResults;
                        }
                    });
                }

                bool isCalled = thisPtr->m_method->Call(arguments, numArguments, thisPtr->m_resultToLua ? &result : nullptr);

                if (!isCalled)
                {
                    ScriptContext::FromNativeContext(lua)->Error(ScriptContext::ErrorType::Error, true, "Lua failed to call %s method!", thisPtr->m_method->m_name.c_str());
                }

                if (thisPtr->m_resultToLua)
                {
                    // push result back to lua
                    if (result.m_value)
                    {
                        if (numResults == 0)
                        {
                            lua_pushnil(lua);
                            ++numResults;
                        }

                        // destroy any value parameters
                        if (thisPtr->m_resultClass && thisPtr->m_resultClass->m_destructor && (result.m_traits & AZ::BehaviorParameter::TR_POINTER) == 0)
                        {
                            void* valueAddress = result.GetValueAddress();
                            if (tempData.inrange(valueAddress))
                            {
                                thisPtr->m_resultClass->m_destructor(valueAddress, thisPtr->m_resultClass->m_userData);
                            }
                        }
                    }
                    else
                    {
                        lua_pushnil(lua); // we have result but no value (true for null pointers too
                        ++numResults;
                    }
                }

                // free temp memory and call any dtors
                if (usedBackupAlloc)
                {
                    backupAllocator.deallocate(result.m_value, thisPtr->m_resultClass->m_size, thisPtr->m_resultClass->m_alignment);
                }
                for (int i = 0; i < numArguments; ++i)
                {
                    BehaviorClass* argClass = thisPtr->m_fromLua[i].second;
                    if (argClass && argClass->m_destructor)
                    {
                        void* valueAddress = arguments[i].GetValueAddress();
                        if (tempData.inrange(valueAddress))
                        {
                            argClass->m_destructor(valueAddress, argClass->m_userData);
                        }
                    }
                }

                return numResults;
            }

            AZStd::vector<AZStd::pair<LuaLoadFromStack, BehaviorClass*>> m_fromLua;
            LuaPushToStack m_resultToLua;
            LuaPrepareValue m_prepareResult;
            BehaviorClass* m_resultClass;

            bool m_isResult;
        };

        class LuaGenericCaller : public LuaCaller
        {
        public:
            AZ_CLASS_ALLOCATOR(LuaGenericCaller, AZ::SystemAllocator);

            LuaGenericCaller(BehaviorContext* context, BehaviorMethod* method)
            {
                (void)context;
                m_method = method;
                m_classPtr = nullptr;
                m_classPtrClass = nullptr;
                AZ_Assert(!method->HasResult(), "Generic calls don't have result!");
                // Member functions have "ThisPtr" + "ScriptDataContext&"
                // Global functions have "ScriptDataContext&
                AZ_Assert(method->GetNumArguments() == 1 || method->GetNumArguments() == 2, "You can have only 1 argument ScriptDataContext& to your global or member functions!");
                AZ_Assert(method->GetArgument(method->GetNumArguments() - 1)->m_typeId == AzTypeInfo<ScriptDataContext>::Uuid(),
                    "Generic calls have only one argument and it's type is ScriptDataContext!");

                if (method->GetNumArguments() == 2) // member functions
                {
                    // should be a registered type (can't integral or generic)

                    m_classPtr = FromLuaStack(context, method->GetArgument(0), m_classPtrClass);
                }

            }

            int ManualCall(lua_State* lua) override
            {
                return Call(lua);
            }

            void PushClosure(lua_State* lua, const char* debugDescription) override
            {
                LSV_BEGIN(lua, 1);

                lua_pushlightuserdata(lua, this); // if there is no reason to keep the data in the "methods" we can use full user data and rely on __gc to clean it
                lua_pushstring(lua, debugDescription);
                lua_pushcclosure(lua, &Internal::LuaMethodTagHelper, 0);
                lua_pushcclosure(lua, &LuaGenericCaller::Call, 3);
            }

            static int Call(lua_State* lua)
            {
                BehaviorArgument methodArgs[2];
                ScriptContext::StackVariableAllocator tempData;

                LuaGenericCaller* thisPtr = reinterpret_cast<LuaGenericCaller*>(lua_touserdata(lua, lua_upvalueindex(1)));

                int argc = lua_gettop(lua);
                int luaNumArguments;
                int luaStackIndex;
                int methodNumArguments;

                if (thisPtr->m_classPtr)
                {
                    luaNumArguments = argc - 1; // skip this ptr
                    luaStackIndex = (argc == 1) ? 1 : 2;
                    methodArgs[0].m_traits = BehaviorParameter::TR_POINTER; // class pointer is always a pointer
                    thisPtr->m_classPtr(lua, 1, methodArgs[0], thisPtr->m_classPtrClass, &tempData);
                    methodNumArguments = 2;
                }
                else
                {
                    luaNumArguments = argc;
                    luaStackIndex = argc ? 1 : 0;
                    methodNumArguments = 1;
                }

                ScriptDataContext callContext(lua, luaNumArguments, luaStackIndex);
                methodArgs[methodNumArguments - 1].Set(&callContext);
                thisPtr->m_method->Call(methodArgs, methodNumArguments, nullptr);

                return callContext.GetNumResults();
            }

            LuaLoadFromStack m_classPtr; ///< Used only for member generic functions
            BehaviorClass* m_classPtrClass; ///< Pointer to the class behavior class, when available.
        };

        static const char* s_luaEBusHandlerMetatableName =  "LuaEBusHandlerMetatable";

        //=========================================================================
        // Check that value at _index is LuaEBusHandler full userdata
        // Basically functionally equivalent to luaL_checkudata function but
        // specific to our LuaEBusHandler metatable.
        // This is needed because right now LuaEBusHandler isn't a reflected type,
        // we manually add closures in the Register() function.
        // This isn't generalized since it's not something we encourage, the metatable
        // living in the global table is something everyone sees and it's not ideal
        // to have by-name search each time - it's preferable to use the reflection
        // system that stores type information and use that (having verification in memory
        // instead of just that it has the same metatable), which is something
        // we can do in the future for this class
        //=========================================================================
        static bool CheckUserDataIsLuaEBusHandler(lua_State* _lua, int _index)
        {
            LSV_BEGIN(_lua, 0);

            bool result = false;
            if (lua_touserdata(_lua, _index)) // verify value at index is userdata
            {
                if (lua_getmetatable(_lua, _index)) // push metatable of userdata to top of stack
                {
                    Internal::azlua_getglobal(_lua, s_luaEBusHandlerMetatableName); // push value of LuaEBusHandler metatable from global
                    if (lua_rawequal(_lua, -1, -2)) // check if the metatables are the same
                    {
                        result = true;
                    }
                    lua_pop(_lua, 2);
                }
            }
            return result;
        }

        static void CallDestructorOnBehaviorValueParameter(BehaviorClass* idClass, BehaviorArgument* parameter)
        {
            if (idClass && idClass->m_destructor && (parameter->m_traits & AZ::BehaviorParameter::TR_POINTER) == 0)
            {
                void* valueAddress = parameter->GetValueAddress();
                if (valueAddress)
                {
                    idClass->m_destructor(valueAddress, idClass->m_userData);
                }
            }

        }


        class LuaEBusHandler
        {
        public:
            AZ_TYPE_INFO(LuaEBusHandler, "{425e6ac2-d36d-4bba-b2b4-989ef3f69597}");
            //AZ_CLASS_ALLOCATOR(LuaEBusHandler, AZ::SystemAllocator);

            struct HookUserData
            {
                int m_luaFunctionIndex; ///< Cached index of the lua function to call
                AZStd::vector<LuaPushToStack> m_args;
                LuaLoadFromStack m_result;
                BehaviorClass* m_resultClass;
                LuaEBusHandler* m_luaHandler;
            };

            LuaEBusHandler(BehaviorContext* behaviorContext, BehaviorEBus* bus, BehaviorClass* idClass, ScriptDataContext& scriptTable, BehaviorArgument* connectionId /*= nullptr*/, bool autoConnect)
                : m_context(behaviorContext)
                , m_bus(bus)
                , m_handler(nullptr)
                , m_boundTableCachedIndex(-1)
            {
                m_lua = scriptTable.m_nativeContext;

                if (bus->m_createHandler)
                {
                    bus->m_createHandler->InvokeResult(m_handler);
                }

                if (m_handler)
                {
#if !defined(_RELEASE)
                    const AZStd::string_view luaString("Lua");
                    lua_Debug info;
                    for (int level = 0; lua_getstack(m_lua, level, &info); ++level)
                    {
                        lua_getinfo(m_lua, "S", &info);
                        if (AZStd::string_view(info.what) == luaString)
                        {
                            m_handler->SetScriptPath(info.short_src);
                            break;
                        }
                    }
#endif

                    BindEvents(scriptTable);

                    if (autoConnect)
                    {
                        m_handler->Connect(connectionId);

                        CallDestructorOnBehaviorValueParameter(idClass, connectionId);
                    }
                }
            }

            ~LuaEBusHandler()
            {
                if (m_boundTableCachedIndex != -1)
                {
                    Internal::LuaReleaseCached(m_lua, m_boundTableCachedIndex);
                }
                for (HookUserData& userData : m_hooks)
                {
                    Internal::LuaReleaseCached(userData.m_luaHandler->m_lua, userData.m_luaFunctionIndex);
                }

                if (m_handler)
                {
                    m_bus->m_destroyHandler->Invoke(m_handler);
                }
            }

            void BindEvents(AZ::ScriptDataContext & dc)
            {
                LSV_BEGIN(m_lua, 0);

                int funcIndex;
                const char* fieldName;
                int fieldIndex;
                AZ::ScriptDataContext bindingTable;
                if (dc.InspectMetaTable(0, bindingTable))
                {
                    // First inspect metatable (recursively) and possibly override with real table variables.
                    BindEvents(bindingTable);
                }
                dc.InspectTable(0, bindingTable);
                if (bindingTable.IsTable(0))
                {
                    // If table already cached, release the old one, as this one's probably closer to what we want
                    if (m_boundTableCachedIndex > 0)
                    {
                        Internal::LuaReleaseCached(dc.m_nativeContext, m_boundTableCachedIndex);
                    }

                    // cache the self table
                    m_boundTableCachedIndex = Internal::LuaCacheValue(dc.m_nativeContext, bindingTable.m_startVariableIndex);
                }

                while (bindingTable.InspectNextElement(funcIndex, fieldName, fieldIndex))
                {
                    if (bindingTable.IsFunction(funcIndex))
                    {
                        const BehaviorEBusHandler::EventArray& events = m_handler->GetEvents();
                        for (int iEvent = 0; iEvent < static_cast<int>(events.size()); ++iEvent)
                        {
                            const BehaviorEBusHandler::BusForwarderEvent& e = events[iEvent];
                            if (strcmp(fieldName, e.m_name) == 0)
                            {
                                HookUserData* userData = &m_hooks.emplace_back();

                                // make the object, cache the function, find the pushers for each argument
                                userData->m_luaHandler = this;
                                userData->m_luaFunctionIndex = Internal::LuaCacheValue(bindingTable.m_nativeContext, bindingTable.m_startVariableIndex + funcIndex);
                                userData->m_args.resize(e.m_parameters.size() - 2);// remove the result + userData

                                int numArguments = static_cast<int>(userData->m_args.size());
                                for (int iArg = 0; iArg < numArguments; ++iArg)
                                {
                                    const BehaviorParameter& arg = e.m_parameters[iArg + 2];
                                    BehaviorClass* argClass = nullptr;
                                    userData->m_args[iArg] = ToLuaStack(m_context, &arg, nullptr, argClass);
                                }

                                // result
                                userData->m_result = FromLuaStack(m_context, &e.m_parameters[0], userData->m_resultClass);

                                // install handler
                                m_handler->InstallGenericHook(iEvent, &LuaEBusHandler::OnEventGenericHook, userData);
                                break;
                            }
                        }
                    }
                }
            }

            static int CreateHandler(lua_State* lua)
            {
                LSV_BEGIN(lua, 1);

                int numArguments = lua_gettop(lua);
                bool autoConnect = (lua_toboolean(lua, lua_upvalueindex(6)) != 0); // Used to tell whether script used CreateHandler (autoConnect=false) or Connect (autoConnect=true)
                if (numArguments >= 1)
                {
                    if (lua_istable(lua, 1))
                    {
                        ScriptDataContext scriptTable(lua, numArguments, 1);

                        LuaLoadFromStack idFromLua = reinterpret_cast<LuaLoadFromStack>(lua_touserdata(lua, lua_upvalueindex(4)));
                        BehaviorClass* idClass = reinterpret_cast<BehaviorClass*>(lua_touserdata(lua, lua_upvalueindex(5)));
                        BehaviorEBus* ebus = reinterpret_cast<BehaviorEBus*>(lua_touserdata(lua, lua_upvalueindex(2)));

                        if (!autoConnect && idFromLua && numArguments > 1)
                        {
                            ScriptContext::FromNativeContext(lua)->Error(ScriptContext::ErrorType::Warning, true,
                                "CreateHandler function called with busId. Please change instances of BusName.CreateHandler(<table>, <id>) to BusName.Connect(<table>, <id>) to avoid loss of functionality in future releases!");
                            autoConnect = true;
                        }

                        BehaviorArgument idParam;
                        ScriptContext::StackVariableAllocator tempData;
                        idParam.Set(ebus->m_idParam);
                        if (idFromLua && autoConnect)
                        {
                            if (numArguments < 2)
                            {
                                // Need ID, don't have one
                                ScriptContext::FromNativeContext(lua)->Error(ScriptContext::ErrorType::Error, true,
                                    "Connect function called without address ID parameter on bus that expects one, please provide address! (e.g., BusName.Connect(self, self.entityId) for component buses)");
                                lua_pushnil(lua);
                                return 1;
                            }
                            else if (!idFromLua(lua, 2, idParam, idClass, &tempData))
                            {
                                ScriptContext::FromNativeContext(lua)->Error(ScriptContext::ErrorType::Error, true,
                                    "Connect function called with address ID of type %s, when type %s was expected.", idParam.m_name, ebus->m_idParam.m_name);
                                lua_pushnil(lua);
                                return 1;
                            }
                        }

                        if (ebus->m_createHandler)
                        {
                            // create handler
                            new(lua_newuserdata(lua, sizeof(LuaEBusHandler)))
                                LuaEBusHandler(reinterpret_cast<BehaviorContext*>(lua_touserdata(lua, lua_upvalueindex(1))), ebus, idClass, scriptTable, (idFromLua && autoConnect) ? &idParam : nullptr, autoConnect);
                            lua_pushvalue(lua, lua_upvalueindex(3)); // set the metatable for the user data
                            lua_setmetatable(lua, -2);
                        }
                        else
                        {
                            ScriptContext::FromNativeContext(lua)->Error(ScriptContext::ErrorType::Error, true, "Create Handler called on a non-reflected class. Please reflect an appropriate EBusHandler into the BehaviorContext in order to successfully create the EBusHandler.");
                            lua_pushnil(lua);
                        }
                    }
                    else
                    {
                        // Non-table first argument
                        const char* functionName = autoConnect ? "Connect" : "CreateHandler";
                        ScriptContext::FromNativeContext(lua)->Error(ScriptContext::ErrorType::Error, true,
                            "Bus function %s called without lua table as first argument, please update with correct syntax (likely forgot to pass 'self'/handling table) (Bad: BusName.%s(), Good: BusName.%s(self))",
                            functionName, functionName, functionName);
                        lua_pushnil(lua);
                    }
                }
                else
                {
                    // No arguments
                    const char* functionName = autoConnect ? "Connect" : "CreateHandler";
                    ScriptContext::FromNativeContext(lua)->Error(ScriptContext::ErrorType::Error, true,
                        "Bus function %s called without any arguments, please update with correct syntax (likely forgot to pass 'self'/handling table) (Bad: BusName.%s(), Good: BusName.%s(self))",
                        functionName, functionName, functionName);
                    lua_pushnil(lua);
                }
                return 1;
            }

            static int Connect(lua_State* lua)
            {
                LSV_BEGIN(lua, 0);

                const int numArguments = lua_gettop(lua);
                if (numArguments >= 1)
                {
                    if (CheckUserDataIsLuaEBusHandler(lua, 1))
                    {
                        LuaEBusHandler* ebusHandler = reinterpret_cast<LuaEBusHandler*>(lua_touserdata(lua, 1));
                        BehaviorClass* idClass = nullptr;
                        LuaLoadFromStack idFromLua = FromLuaStack(ebusHandler->m_context, &ebusHandler->m_bus->m_idParam, idClass);

                        BehaviorArgument idParam;
                        ScriptContext::StackVariableAllocator tempData;
                        idParam.Set(ebusHandler->m_bus->m_idParam);
                        if (idFromLua)
                        {
                            if (numArguments < 2)
                            {
                                // Need ID, don't have one
                                ScriptContext::FromNativeContext(lua)->Error(ScriptContext::ErrorType::Error, true,
                                    "Connect function called without address ID parameter on bus that expects one, please provide address! (e.g., handler:Connect(self.entityId) for component buses)");
                                return 0;
                            }
                            else if (!idFromLua(lua, 2, idParam, idClass, &tempData))
                            {
                                ScriptContext::FromNativeContext(lua)->Error(ScriptContext::ErrorType::Error, true,
                                    "Connect function called with address ID of type %s, when type %s was expected.", idParam.m_name, ebusHandler->m_bus->m_idParam.m_name);
                                lua_pushnil(lua);
                                return 1;
                            }
                        }

                        ebusHandler->m_handler->Connect(idFromLua ? &idParam : nullptr);

                        CallDestructorOnBehaviorValueParameter(idClass, &idParam);
                    }
                    else
                    {
                        // Handler/table not first argument
                        ScriptContext::FromNativeContext(lua)->Error(ScriptContext::ErrorType::Error, true,
                            "Connect function called on bus handler without the handler as first argument, please update with correct syntax (likely using . instead of : on handler) (Bad: handler.Connect(id), Good: handler:Connect(id)");
                    }
                }
                else
                {
                    // No arguments
                    ScriptContext::FromNativeContext(lua)->Error(ScriptContext::ErrorType::Error, true,
                        "Connect function called on bus handler without any arguments, please update with correct syntax (likely using . instead of : on bus with no ID) (Bad: handler.Connect(), Good: handler:Connect())");
                }
                return 0;
            }

            static int DestroyHandler(lua_State* lua)
            {
                LSV_BEGIN(lua, 0);

                LuaEBusHandler* ebusHandler = reinterpret_cast<LuaEBusHandler*>(lua_touserdata(lua, -1));
                ebusHandler->~LuaEBusHandler(); // just call the dtor as we allocate the handler part of the userdata
                return 0;
            }

            static int Disconnect(lua_State* lua)
            {
                LSV_BEGIN(lua, 0);

                const int numArguments = lua_gettop(lua);
                if (numArguments >= 1)
                {
                    if (CheckUserDataIsLuaEBusHandler(lua, 1))
                    {
                        LuaEBusHandler* ebusHandler = reinterpret_cast<LuaEBusHandler*>(lua_touserdata(lua, 1));
                        ebusHandler->m_handler->Disconnect();
                    }
                    else
                    {
                        // Handler/table not first argument
                        ScriptContext::FromNativeContext(lua)->Error(ScriptContext::ErrorType::Error, true,
                            "Disconnect function called on bus handler without the handler as first argument, please update with correct syntax (likely using . instead of : on handler) (Expected: handler:Disconnect())");
                    }
                }
                else
                {
                    // No arguments
                    ScriptContext::FromNativeContext(lua)->Error(ScriptContext::ErrorType::Error, true,
                        "Disconnect function called on bus handler without any arguments, please update with correct syntax (likely using . instead of : on handler) (Bad: handler.Disconnect(), Good: handler:Disconnect())");
                }
                return 0;
            }

            static int IsConnected(lua_State* L)
            {
                LSV_BEGIN(L, 1);

                const int numArgs = lua_gettop(L);
                if (numArgs < 1) // no argument
                {
                    ScriptContext::FromNativeContext(L)->Error(ScriptContext::ErrorType::Error, true,
                        "IsConnected function called on an EBus handler without 'self' parameter, please update to the correct syntax, e.g. myHandler:IsConnected()");
                    lua_pushboolean(L, false);
                    return 1;
                }

                if (!CheckUserDataIsLuaEBusHandler(L, 1))
                {
                    // Make sure we have the handler as the first argument.
                    ScriptContext::FromNativeContext(L)->Error(ScriptContext::ErrorType::Error, true,
                        "IsConnected function called on an EBus handler without the handler as the first argument, please update to the correct syntax, e.g. handler:IsConnected()");
                    lua_pushboolean(L, false);
                    return 1;
                }

                LuaEBusHandler* ebusHandler = reinterpret_cast<LuaEBusHandler*>(lua_touserdata(L, 1));
                bool isConnected = ebusHandler->m_handler->IsConnected();
                lua_pushboolean(L, isConnected);
                return 1;
            }

            static int IsConnectedId(lua_State* L)
            {
                LSV_BEGIN(L, 1);

                const int numArgs = lua_gettop(L);
                if (numArgs < 1) // no argument
                {
                    ScriptContext::FromNativeContext(L)->Error(ScriptContext::ErrorType::Error, true,
                        "IsConnectedId function called on an EBus handler without 'self' parameter, please update to the correct syntax, e.g. myHandler:IsConnected(some_id)");
                    lua_pushboolean(L, false);
                    return 1;
                }

                if (!CheckUserDataIsLuaEBusHandler(L, 1))
                {
                    // Make sure we have the handler as the first argument.
                    ScriptContext::FromNativeContext(L)->Error(ScriptContext::ErrorType::Error, true,
                        "IsConnectedId function called on an EBus handler without the handler as the first argument, please update to the correct syntax, e.g. handler:IsConnectedId(some_id)");
                    lua_pushboolean(L, false);
                    return 1;
                }

                if (numArgs < 2)
                {
                    ScriptContext::FromNativeContext(L)->Error(ScriptContext::ErrorType::Error, true,
                        "IsConnectedId expects an argument of Id with which the handler of interest was connected with.");
                    lua_pushboolean(L, false);
                    return 1;
                }

                LuaEBusHandler* ebusHandler = reinterpret_cast<LuaEBusHandler*>(lua_touserdata(L, 1));
                BehaviorClass* idClass = nullptr;
                LuaLoadFromStack idFromLua = FromLuaStack(ebusHandler->m_context, &ebusHandler->m_bus->m_idParam, idClass);

                if (idFromLua)
                {
                    BehaviorArgument idParam;
                    ScriptContext::StackVariableAllocator tempData;
                    idParam.Set(ebusHandler->m_bus->m_idParam);

                    if (idFromLua(L, 2, idParam, idClass, &tempData))
                    {
                        bool isConnected = ebusHandler->m_handler->IsConnectedId(&idParam);
                        lua_pushboolean(L, isConnected);
                        return 1;
                    }
                    else
                    {
                        ScriptContext::FromNativeContext(L)->Error(ScriptContext::ErrorType::Error, true,
                            "IsConnectedId function called with address Id of type %s, when type %s was expected.", idParam.m_name, ebusHandler->m_bus->m_idParam.m_name);
                        lua_pushboolean(L, false);
                        return 1;
                    }
                }
                else // This handler was connected without an Id.
                {
                    ScriptContext::FromNativeContext(L)->Error(ScriptContext::ErrorType::Error, true,
                        "IsConnectedId function called on an EBus handler that was initially connected without an Id. Please use IsConnected() instead.");
                    lua_pushboolean(L, false);
                    return 1;
                }
            }

            static void Register(lua_State* lua)
            {
                LSV_BEGIN(lua, 0);

                // register LuaHandler metatable
                // TODO: consider just reflecting the class and using the reflection, we don't do it as we don't want to modify behavior context)
                lua_createtable(lua, 0, 0);

                // Collect the handler
                lua_pushliteral(lua, "__gc");
                lua_pushcclosure(lua, &LuaEBusHandler::DestroyHandler, 0);
                lua_rawset(lua, -3);

                // Connect explicitly
                lua_pushliteral(lua, "Connect");
                lua_pushcclosure(lua, &LuaEBusHandler::Connect, 0);
                lua_rawset(lua, -3);

                // Disconnect explicitly
                lua_pushliteral(lua, "Disconnect");
                lua_pushcclosure(lua, &LuaEBusHandler::Disconnect, 0);
                lua_rawset(lua, -3);

                // IsConnected explicitly
                lua_pushliteral(lua, "IsConnected");
                lua_pushcclosure(lua, &LuaEBusHandler::IsConnected, 0);
                lua_rawset(lua, -3);

                // IsConnectedId explicitly
                lua_pushliteral(lua, "IsConnectedId");
                lua_pushcclosure(lua, &LuaEBusHandler::IsConnectedId, 0);
                lua_rawset(lua, -3);

                // the the __index to the table itself
                lua_pushliteral(lua, "__index");
                lua_pushvalue(lua, -2); // copy the table
                lua_rawset(lua, -3);

                // Other functions


                // Register the metatable
                Internal::azlua_setglobal(lua, s_luaEBusHandlerMetatableName);
            }

            static void OnEventGenericHook(void* userData, const char* eventName, int eventIndex, BehaviorArgument* result, int numParameters, BehaviorArgument* parameters)
            {
                (void)eventName;
                (void)eventIndex;

                HookUserData* ud = reinterpret_cast<HookUserData*>(userData);
                lua_State* lua = ud->m_luaHandler->m_lua;

                AZ_Assert(ScriptContext::FromNativeContext(lua)->DebugIsCallingThreadTheOwner(), "Lua EBus handler called from a non-owner thread.");

                LSV_BEGIN(lua, 0);

                int numArguments = static_cast<int>(ud->m_args.size());
                if (numParameters != numArguments)
                {
                    ScriptContext::FromNativeContext(lua)->Error(ScriptContext::ErrorType::Error, true, "Number of parameters required (%d) and arguments provided (%d) should match!", numParameters, numArguments);
                    return;
                }

                Internal::LuaLoadCached(lua, ud->m_luaFunctionIndex); // load the function
                if (!lua_isfunction(lua, -1))
                {
                    AZ_Error("ScriptContext", false, "Internal Error: Callback bound to invalid function (%s)", luaL_typename(lua, -1));
                    lua_pop(lua, 1);
                    return;
                }

                Internal::LuaLoadCached(lua, ud->m_luaHandler->m_boundTableCachedIndex); // load the bound table 'self'
                if (!lua_istable(lua, -1))
                {
                    AZ_Error("ScriptContext", false, "Internal Error: Callback bound to invalid function. Script: %s: function type: %s", ud->m_luaHandler->m_handler->GetScriptPath().data(), luaL_typename(lua, -1));
                    lua_pop(lua, 2);
                    return;
                }

                for (int iParam = 0; iParam < numParameters; ++iParam)
                {
                    ud->m_args[iParam](lua, parameters[iParam]);
                }

                // call
                bool callSuccess = Internal::LuaSafeCall(lua, numParameters + 1, result ? 1 : 0);  // + 1 for the table parameter
                if (callSuccess && result)
                {
                    ud->m_result(lua, -1, *result, ud->m_resultClass, nullptr);
                    lua_pop(lua, 1); // remove the return value now that we've captured it
                }
            }

            // LuaPushToStack

            // install create the handler store it
            lua_State* m_lua;
            BehaviorContext* m_context;
            BehaviorEBus* m_bus;
            BehaviorEBusHandler* m_handler; // create from behavior context
            int m_boundTableCachedIndex;
            AZStd::list<HookUserData> m_hooks;
        };

        class LuaEBusGenericFunctionQueue
        {
        public:
            static void FromBehaviorContext(void* userData1, void* userData2)
            {
                lua_State* lua = reinterpret_cast<lua_State*>(userData1);
                int cachedIndex = static_cast<int>(reinterpret_cast<size_t>(userData2));

                lua_rawgeti(lua, LUA_REGISTRYINDEX, cachedIndex);
                luaL_unref(lua, LUA_REGISTRYINDEX, cachedIndex); // release the references
                int functionIndex = lua_gettop(lua);

                // push function and arguments
                int numValues = 0;
                for (; numValues < 256; ++numValues)
                {
                    if (lua_getupvalue(lua, functionIndex, numValues + 1) == nullptr)
                    {
                        break;
                    }
                }

                Internal::LuaSafeCall(lua, numValues - 1, 0);  // -1 as the first value is the function
            }

            static int FromLua(lua_State* lua)
            {
                int numArguments = lua_gettop(lua);

                // cache the function and all parameter for a later call
                BehaviorEBus* ebus = reinterpret_cast<BehaviorEBus*>(lua_touserdata(lua, lua_upvalueindex(1)));

                if (lua_isfunction(lua, -numArguments))
                {
                    // push all arguments as a fake closure
                    lua_pushcclosure(lua, &FromLua, numArguments);

                    // reference the closure until we execute the call
                    int cachedIndex = luaL_ref(lua, LUA_REGISTRYINDEX);

                    void* userData1 = lua;
                    void* userData2 = reinterpret_cast<void*>(static_cast<size_t>(cachedIndex));

                    // register the callback
                    ebus->m_queueFunction->Invoke(&FromBehaviorContext, userData1, userData2);
                }
                else
                {
                    // TODO: Report that we expect QueueFunction(Function, ...)
                }

                return 0;
            }
        };

        // end of Method Bind Helper
        //////////////////////////////////////////////////////////////////////////

        class ScriptContextImpl : public BehaviorContextBus::Handler
        {
        public:
            typedef ScriptProperty* (*ScriptTypeFactory)(ScriptDataContext& context, int valueIndex, const char* name);

            AZ_CLASS_ALLOCATOR(ScriptContextImpl, AZ::SystemAllocator);

            //////////////////////////////////////////////////////////////////////////
            ScriptContextImpl(ScriptContext* owner, IAllocator* allocator, lua_State* nativeContext)
                : m_owner(owner)
                , m_context(nullptr)
                , m_debug(nullptr)
                , m_errorHook(nullptr)
                , m_scriptPropertyFactories(
                    {
                        // Nil
                        &AZ::ScriptPropertyNil::TryCreateProperty,

                        // Values
                        &AZ::ScriptPropertyBoolean::TryCreateProperty,
                        &AZ::ScriptPropertyNumber::TryCreateProperty,
                        &AZ::ScriptPropertyString::TryCreateProperty,
                        &AZ::ScriptPropertyGenericClass::TryCreateProperty,
                    }
                )
                , m_scriptPropertyArrayFactories(
                    {
                        // Value Arrays
                        &AZ::ScriptPropertyBooleanArray::TryCreateProperty,
                        &AZ::ScriptPropertyNumberArray::TryCreateProperty,
                        &AZ::ScriptPropertyStringArray::TryCreateProperty,
                        &AZ::ScriptPropertyGenericClassArray::TryCreateProperty,
                    }
                )
                , m_scriptPropertyTableFactory(&AZ::ScriptPropertyTable::TryCreateProperty)
            {
                m_ownerThreadId = AZStd::this_thread::get_id();

                m_lua = nativeContext;
                m_isCustomLuaVM = nativeContext != nullptr;

                if (!m_isCustomLuaVM)
                {
                    if (!allocator)
                    {
                        allocator = &m_luaAllocator;
                    }
                    m_lua = lua_newstate(&LuaMemoryHook, allocator);
                    AZ_Assert(m_lua, "Failed to create new LUA state!");
                }

                lua_atpanic(m_lua, &LuaPanic);
                OpenLuaLibraries(m_lua);

                lua_pushglobaltable(m_lua);       ///< push the _G on the stack

                // setup typeid() function
                lua_pushliteral(m_lua, "typeid");
                lua_pushcfunction(m_lua, &Global_Typeid);
                lua_rawset(m_lua, -3);

                if (lua_getmetatable(m_lua, -1) == 0) /// Make sure it doesn't have a meta-table
                {
                    // Disable tracking "Access to undeclared global variable" errors for custom contexts.
                    if (!m_isCustomLuaVM)
                    {
                        lua_createtable(m_lua, 0, 0);       // create metatable

                        lua_pushliteral(m_lua, "__index"); // add the __index function
                        lua_pushcclosure(m_lua, &Global__Index, 0);
                        lua_rawset(m_lua, -3);

                        lua_pushliteral(m_lua, "__newindex");  // add the __newindex function
                        lua_pushcclosure(m_lua, &Global__NewIndex, 0);
                        lua_rawset(m_lua, -3);

                        lua_pushliteral(m_lua, "__metatable"); // protect the metatable
                        lua_pushboolean(m_lua, 1); // anything
                        lua_rawset(m_lua, -3);

                        lua_setmetatable(m_lua, -2);   // set the metatable as the _G metatable
                    }

                    // store the script context in lua, if we need to access it
                    lua_pushlightuserdata(m_lua, m_owner);
                    int tableRef = luaL_ref(m_lua, LUA_REGISTRYINDEX);
                    (void)tableRef;
                    AZ_Assert(tableRef == AZ_LUA_SCRIPT_CONTEXT_REF, "Table reference should match %d but is instead %d!", AZ_LUA_SCRIPT_CONTEXT_REF, tableRef);

                    // create a AZGlobals table, we can use internal unodered_map if it's faster (TODO: test which is faster, or if there is a benefit keeping in la)
                    lua_createtable(m_lua, 0, 1024); // pre allocate some values in the hash
                    tableRef = luaL_ref(m_lua, LUA_REGISTRYINDEX); // create ref (for quick access) to the AZGlobalProperties
                    AZ_Assert(tableRef == AZ_LUA_GLOBALS_TABLE_REF, "Table referece should match %d !", AZ_LUA_GLOBALS_TABLE_REF);

                    // create AZClassMap
                    lua_createtable(m_lua, 0, 1024); // create table with preallocated 1024 entries
                    tableRef = luaL_ref(m_lua, LUA_REGISTRYINDEX); // create a ref to the AZClassMap table
                    AZ_Assert(tableRef == AZ_LUA_CLASS_TABLE_REF, "Table referece should match %d !", AZ_LUA_CLASS_TABLE_REF);

                    // create weak reference cache table (used when we want to cache a values for faster access from C++)
                    lua_createtable(m_lua, 1024, 0); // create the table with indices as we will use them
                    lua_createtable(m_lua, 0, 1); // create metatable to control the "weak" setting
                    lua_pushliteral(m_lua, "__mode");
                    lua_pushliteral(m_lua, "kv");
                    lua_rawset(m_lua, -3); // set __mode = "kv"
                    lua_setmetatable(m_lua, -2); // set the cache table metatable
                    // Set table[WeakCacheFreeListIndex] = WeakCacheFreeListIndex + 1
                    lua_pushinteger(m_lua, Internal::WeakCacheUser);
                    lua_rawseti(m_lua, -2, Internal::WeakCacheFreeListIndex);

                    tableRef = luaL_ref(m_lua, LUA_REGISTRYINDEX);
                    AZ_Assert(tableRef == AZ_LUA_WEAK_CACHE_TABLE_REF, "Table reference should match %d !", AZ_LUA_WEAK_CACHE_TABLE_REF);

                    lua_pushcfunction(m_lua, &Internal::LuaSafeCallErrorHandler);
                    tableRef = luaL_ref(m_lua, LUA_REGISTRYINDEX);
                    AZ_Assert(tableRef == AZ_LUA_ERROR_HANDLER_FUN_REF, "Function reference should match %d! ", AZ_LUA_ERROR_HANDLER_FUN_REF);
                }
                else
                {
                    // _G already has a metatable
                }
            }

            //////////////////////////////////////////////////////////////////////////
            ~ScriptContextImpl() override
            {
                if (BusIsConnected())
                {
                    BusDisconnect();
                }
                for (LuaScriptCaller* binder : m_methods)
                {
                    delete binder;
                }

                for (LuaGenericCaller* binder : m_genericMethods)
                {
                    delete binder;
                }

                delete m_debug;

                if (m_lua && !m_isCustomLuaVM)
                {
                    lua_pushglobaltable(m_lua);
                    luaL_unref(m_lua, LUA_REGISTRYINDEX, AZ_LUA_SCRIPT_CONTEXT_REF);
                    luaL_unref(m_lua, LUA_REGISTRYINDEX, AZ_LUA_GLOBALS_TABLE_REF);
                    luaL_unref(m_lua, LUA_REGISTRYINDEX, AZ_LUA_CLASS_TABLE_REF);
                    lua_pop(m_lua, 1);
                    lua_close(m_lua);
                }
            }

            //////////////////////////////////////////////////////////////////////////
            struct DefaultBehaviorCaller
            {
                static int Create(lua_State* lua)
                {
                    LSV_BEGIN(lua, 1);

                    // the metatable is on the stack
                    int result = lua_getmetatable(lua, 1); // the table is first argument
                    if (result == 0)
                    {
                        AZ_Assert(result != 0, "All of the registered types that implemnt __call, should have a metatatable!");
                        lua_pushnil(lua);
                        return 1; // push nil object and return it
                    }

                    // replace the source table with the metatable
                    int metatableIndex = 1;
                    lua_replace(lua, 1);

                    // get the behavior class
                    lua_rawgeti(lua, metatableIndex, AZ_LUA_CLASS_METATABLE_BEHAVIOR_CLASS);
                    BehaviorClass* behaviorClass = reinterpret_cast<BehaviorClass*>(lua_touserdata(lua, -1));

                    // get storage type
                    lua_rawgeti(lua, metatableIndex, AZ_LUA_CLASS_METATABLE_STORAGE_TYPE_INDEX);
                    Script::Attributes::StorageType storageType = (Script::Attributes::StorageType)lua_tointeger(lua, -1);

                    lua_pop(lua, 2); // pop class and storage type

                    // check for user storage type
                    size_t valueSize = 0;
                    if (storageType == Script::Attributes::StorageType::Value)
                    {
                        // alignment is already checked at reflection type
                        valueSize += behaviorClass->m_size;
                        if (behaviorClass->m_alignment > sizeof(L_Umaxalign)) // make sure we can properly align the userdata
                        {
                            valueSize += behaviorClass->m_alignment - sizeof(L_Umaxalign);
                        }
                    }

                    LuaUserData* userData = reinterpret_cast<LuaUserData*>(lua_newuserdata(lua, sizeof(LuaUserData) + valueSize));
                    userData->magicData = Internal::AZLuaUserData;
                    userData->behaviorClass = behaviorClass;
                    userData->storageType = (int)storageType;

                    if (valueSize)
                    {
                        userData->value = PointerAlignUp(userData + 1, behaviorClass->m_alignment);
                    }
                    else
                    {
                        userData->value = behaviorClass->Allocate();
                    }

#if defined(LUA_USERDATA_TRACKING)
                    if (userData->behaviorClass->m_typeId == azrtti_typeid<AZStd::vector<AZ::EntityId>>())
                    {
                        ScriptContext::FromNativeContext(lua)->AddAllocation(userData);
                    }
#endif//defined(LUA_USERDATA_TRACKING)

                    // store the value instance in the cache, so we receive that pointer from C++
                    // we don't need to recreate the user data
                    lua_rawgeti(lua, metatableIndex, AZ_LUA_CLASS_METATABLE_INSTANCES); // load the instance table
                    lua_pushlightuserdata(lua, userData->value); // load the address
                    lua_pushvalue(lua, -3); // copy the value
                    lua_rawset(lua, -3); // store value reference into the instance table
                    lua_pop(lua, 1); // pop the instance table

                    lua_pushvalue(lua, metatableIndex); // copy the metatable on the top of the stack
                    lua_setmetatable(lua, -2); // set the metatable for the userdata

                    // Construct the value we just allocated
                    LuaCaller* customConstructor = reinterpret_cast<LuaCaller*>(lua_touserdata(lua, lua_upvalueindex(1)));
                    if (customConstructor)
                    {
                        // currently we have the the class table and the instance on the stack
                        // pop the table so we can actually use the default lua call (so it should not be on the stack)
                        // this is "thisPtr/Value" + parameters
                        lua_replace(lua, metatableIndex);
                        customConstructor->ManualCall(lua);
                        // push back the value to the top
                        lua_pushvalue(lua, metatableIndex);
                    }
                    else
                    {
                        behaviorClass->m_defaultConstructor(userData->value, behaviorClass->m_userData);
                    }
                    return 1;
                }

                static int Destroy(lua_State* lua)
                {
                    LSV_BEGIN(lua, 0);

                    if (lua_istable(lua, 1))
                    {
                        // tables have finalizers as of 5.2, including the table used to provide this finalizer for user data
                        return 0;
                    }

                    // Lua: userdata
                    LuaUserData* userData = reinterpret_cast<LuaUserData*>(lua_touserdata(lua, 1));

#if defined(LUA_USERDATA_TRACKING)
                    if (userData->behaviorClass->m_typeId == azrtti_typeid<AZStd::vector<AZ::EntityId>>())
                    {
                        ScriptContext::FromNativeContext(lua)->RemoveAllocation(userData);
                    }
#endif//defined(LUA_USERDATA_TRACKING)

                    // clear the (weak) reference to the table -- needs safety checks, as the order is not guaranteed
                    // prove this test by using the same memory twice on a cached instance (one not run-time owned or by value)
                    lua_rawgeti(lua, LUA_REGISTRYINDEX, AZ_LUA_CLASS_TABLE_REF);
                    // Lua: userdata, ?
                    if (lua_istable(lua, -1))
                    {
                        // Lua: userdata, class_table
                        Internal::azlua_pushtypeid(lua, userData->behaviorClass->m_typeId);
                        // Lua: userdata, class_table, typeid
                        lua_rawget(lua, -2);
                        // Lua: userdata, class_table, ?
                        if (lua_istable(lua, -1))
                        {
                            // Lua: userdata, class_table, class_mt
                            lua_rawgeti(lua, -1, AZ_LUA_CLASS_METATABLE_INSTANCES);
                            // Lua: userdata, class_table, class_mt, instances
                            lua_pushlightuserdata(lua, userData->value);
                            // Lua: userdata, class_table, class_mt, instances, value
                            lua_pushnil(lua);
                            // Lua: userdata, class_table, class_mt, instances, value, nil
                            lua_rawset(lua, -3);
                            // Lua: userdata, class_table, class_mt, instances
                            lua_pop(lua, 3);
                        }
                        else
                        {
                            // Lua: userdata, class_table, ?
                            lua_pop(lua, 2);
                        }
                    }
                    else
                    {
                        lua_pop(lua, 1);
                    }

                    // Lua: userdata
                    if (userData->storageType != (u32)Script::Attributes::StorageType::RuntimeOwn)
                    {
                        // Lua is responsible for destruction
                        userData->behaviorClass->m_destructor(userData->value, userData->behaviorClass->m_userData);

                        if (userData->storageType == (u32)Script::Attributes::StorageType::ScriptOwn)
                        {
                            // host-supplied function is responsible for deallocation
                            userData->behaviorClass->Deallocate(userData->value);
                        }
                        // else Lua will free memory when garbage collection cycle completes
                    }

                    // if userstorage destroy it
                    return 0;
                }
            };

            LuaCaller* CreateLuaCaller(BehaviorContext* behaviorContext, BehaviorMethod* method)
            {
                LSV_BEGIN(m_lua, 0);

                LuaCaller* binder;

                // Check for specialized function for scripting void (ScriptContextData&), both as function and as custom attribute
                if ((method->GetNumArguments() && method->GetArgument(method->GetNumArguments() - 1)->m_typeId == AzTypeInfo<ScriptDataContext>::Uuid()))
                {
                    //
                    LuaGenericCaller* caller = aznew LuaGenericCaller(behaviorContext, method);
                    binder = caller;
                    m_genericMethods.insert(caller);
                }
                else
                {
                    // Create the generic lua script caller
                    LuaScriptCaller* caller = aznew LuaScriptCaller(behaviorContext, method);
                    binder = caller;
                    m_methods.insert(caller);
                }

                return binder;
            }

            void BindMethodOnStack(BehaviorContext* behaviorContext, BehaviorMethod* method)
            {
                LSV_BEGIN(m_lua, 1);

                // Check for function override
                Attribute* functionOverrideAttribute = FindAttribute(Script::Attributes::MethodOverride, method->m_attributes);
                if (functionOverrideAttribute)
                {
                    method = reinterpret_cast<BehaviorMethod*>(functionOverrideAttribute->GetContextData());
                    if (!method)
                    {
                        AZ_Error("Script", false, "Script override function for method %s should have signature void (ScriptDataContext&)!", lua_tostring(m_lua, 1));
                        lua_pushnil(m_lua);
                        return;
                    }
                }

                LuaCaller* binder = CreateLuaCaller(behaviorContext, method);
                if (method->m_debugDescription)
                {
                    binder->PushClosure(m_lua, method->m_debugDescription);
                }
                else
                {
                    // There is no substitution for developer debug description, as we will
                    // get context on the issue. When this is missing we will list the types
                    // of the parameters we expect to pass from Lua
                    AZStd::string functionName;
                    if (method->HasResult())
                    {
                        AZStd::string resultName = method->GetResult()->m_name;
                        // strip all qualifiers from the result name, or look it up by type in the behavior context
                        AZ::StripQualifiers(resultName);

                        functionName += "[=";
                        functionName += AZ::ReplaceCppArtifacts(resultName); // make sure the name is converted to a lua compatible name (so it looks as it will be called from Lua)
                        functionName += "]";
                        if (method->GetNumArguments() > 0)
                        {
                            functionName += " ";
                        }
                    }

                    for (size_t i = 0; i < method->GetNumArguments(); ++i)
                    {
                        AZStd::string paramName = method->GetArgument(i)->m_name;
                        // strip all qualifiers from the param name, or look it up by type in the behavior context
                        AZ::StripQualifiers(paramName);

                        functionName += AZ::ReplaceCppArtifacts(paramName); // make sure param name is converted to a lua compatible name (so it looks as it will be called from Lua)
                        if (i != method->GetNumArguments() - 1)
                        {
                            functionName += ", ";
                        }
                    }

                    // ScriptDataContext is the parameter when we have generic input/out, the user should provide info what actually are input/output parameters

                    if (AZ::StringFunc::Replace(functionName, "ScriptDataContext", "...", true))
                    {
                        functionName.insert(0, "[=...] ");
                    }

                    binder->PushClosure(m_lua, functionName.c_str());
                }
            }

            //////////////////////////////////////////////////////////////////////////
            bool HandleClassOperators(BehaviorClass* behaviorClass, const char*& methodName, BehaviorMethod*& method)
            {
                LSV_BEGIN(m_lua, 0);

                const char* operatorName = nullptr;
                Attribute* operatorAttr = FindAttribute(Script::Attributes::Operator, method->m_attributes);
                if (operatorAttr)
                {
                    // check if we have a custom function for this operator
                    // TODO: should we just pass this in from the method loop as we might need it anyway
                    Attribute* scriptOverride = FindAttribute(Script::Attributes::MethodOverride, method->m_attributes);
                    if (scriptOverride)
                    {
                        method = reinterpret_cast<BehaviorMethod*>(scriptOverride->GetContextData());
                        if (!method)
                        {
                            AZ_Error("Script", false, "Script override function for method %s should have signature void (ScriptDataContext&)!", lua_tostring(m_lua, 1));
                            return false;
                        }
                    }

                    // read the operator type
                    AttributeReader operatorAttrReader(nullptr, operatorAttr);
                    Script::Attributes::OperatorType operatorType;
                    operatorAttrReader.Read<Script::Attributes::OperatorType>(operatorType);

                    switch (operatorType)
                    {
                        case AZ::Script::Attributes::OperatorType::Add:
                            if (!operatorName) operatorName = "__add";
                            //break;
                        case AZ::Script::Attributes::OperatorType::Sub:
                            if (!operatorName) operatorName = "__sub";
                            //break;
                        case AZ::Script::Attributes::OperatorType::Mul:
                            if (!operatorName) operatorName = "__mul";
                            //break;
                        case AZ::Script::Attributes::OperatorType::Div:
                            if (!operatorName) operatorName = "__div";
                            //break;
                        case AZ::Script::Attributes::OperatorType::Mod:
                            if (!operatorName) operatorName = "__mod";
                            //break;
                        case AZ::Script::Attributes::OperatorType::Pow:
                            if (!operatorName) operatorName = "__pow";
                            //break;
                        case AZ::Script::Attributes::OperatorType::Concat:
                            if (!operatorName) operatorName = "__concat";
                            if ((method->GetNumArguments() == 2 && method->GetArgument(0)->m_typeId == behaviorClass->m_typeId) &&
                                ((!method->HasResult() && method->GetArgument(1)->m_typeId == AzTypeInfo<ScriptDataContext>::Uuid()) // generic function
                                || (method->HasResult() && method->GetResult()->m_typeId == behaviorClass->m_typeId && method->GetArgument(1)->m_typeId == behaviorClass->m_typeId)))
                            {
                                methodName = operatorName; // override the method name for reflection
                            }
                            else
                            {
                                AZ_Error("Script", false, "Script required function format is %s* %s(%s*, %s*)!", behaviorClass->m_name.c_str(), operatorName, behaviorClass->m_name.c_str(), behaviorClass->m_name.c_str());
                                return false;
                            }
                            break;
                        case AZ::Script::Attributes::OperatorType::Unary:
                            if ((!method->HasResult() && method->GetNumArguments() == 2 && method->GetArgument(1)->m_typeId == AzTypeInfo<ScriptDataContext>::Uuid()) || // generic function
                                (method->HasResult() && method->GetResult()->m_typeId == behaviorClass->m_typeId && method->GetNumArguments() == 1 && method->GetArgument(0)->m_typeId == behaviorClass->m_typeId))
                            {
                                methodName = "__unm"; // override the method name for reflection
                            }
                            else
                            {
                                AZ_Error("Script", false, "Script required function format is %s* Unary(%s*)!", behaviorClass->m_name.c_str(), behaviorClass->m_name.c_str());
                                return false;
                            }
                            break;
                        case AZ::Script::Attributes::OperatorType::Length:
                            if ((!method->HasResult() && method->GetNumArguments() == 2 && method->GetArgument(1)->m_typeId == AzTypeInfo<ScriptDataContext>::Uuid()) || // generic function
                                (method->HasResult() && method->GetResult()->m_typeId == AzTypeInfo<int>::Uuid() && // should we check for int or all numeric types?
                                method->GetNumArguments() == 1 && method->GetArgument(0)->m_typeId == behaviorClass->m_typeId))
                            {
                                methodName = "__len"; // override the method name for reflection
                            }
                            else
                            {
                                AZ_Error("Script", false, "Script required function format is int Length(%s*)!", behaviorClass->m_name.c_str());
                                return false;
                            }
                            break;
                        case AZ::Script::Attributes::OperatorType::Equal:
                            if (!operatorName) operatorName = "__eq";
                            //break;
                        case AZ::Script::Attributes::OperatorType::LessThan:
                            if (!operatorName) operatorName = "__lt";
                            //break;
                        case AZ::Script::Attributes::OperatorType::LessEqualThan:
                            if (!operatorName) operatorName = "__le";
                            if ((!method->HasResult() && method->GetNumArguments() == 2 && method->GetArgument(1)->m_typeId == AzTypeInfo<ScriptDataContext>::Uuid()) || // generic function
                                (method->HasResult() && method->GetResult()->m_typeId == AzTypeInfo<bool>::Uuid() &&
                                method->GetNumArguments() == 2 && method->GetArgument(0)->m_typeId == behaviorClass->m_typeId && method->GetArgument(1)->m_typeId == behaviorClass->m_typeId))
                            {
                                methodName = operatorName; // override the method name for reflection
                            }
                            else
                            {
                                AZ_Error("Script", false, "Script required function format is bool %s(%s*,%s*)!", operatorName, behaviorClass->m_name.c_str(), behaviorClass->m_name.c_str());
                                return false;
                            }
                            break;
                        case AZ::Script::Attributes::OperatorType::ToString:
                            if ((!method->HasResult() && method->GetNumArguments() == 2 && method->GetArgument(1)->m_typeId == AzTypeInfo<ScriptDataContext>::Uuid()) || // generic function
                                (method->HasResult() && (method->GetResult()->m_typeId == AzTypeInfo<const char*>::Uuid() || method->GetResult()->m_typeId == AzTypeInfo<AZStd::string>::Uuid())))
                            {
                                methodName = "__tostring"; // override the method name for reflection
                            }
                            else
                            {
                                AZ_Error("Script", false, "Script required function format is AZStd::string ToString(%s*)!", behaviorClass->m_name.c_str());
                                return false;
                            }
                            break;
                        case AZ::Script::Attributes::OperatorType::IndexRead:
                            if ((!method->HasResult() && method->GetNumArguments() == 2 && method->GetArgument(1)->m_typeId == AzTypeInfo<ScriptDataContext>::Uuid()) || // generic function
                                (method->GetNumArguments() == 2 && method->GetArgument(0)->m_typeId == behaviorClass->m_typeId && // it should accept that class pointer + index
                                method->HasResult())) // we should return result
                            {
                                methodName = "__AZ_Index"; /// internal name
                            }
                            else
                            {
                                AZ_Error("Script", false, "Script required function format for IndexRead is ResultType(%s*,IndexType)!", behaviorClass->m_name.c_str());
                                return false;
                            }
                            break;
                        case AZ::Script::Attributes::OperatorType::IndexWrite:
                            if ((!method->HasResult() && method->GetNumArguments() == 2 && method->GetArgument(1)->m_typeId == AzTypeInfo<ScriptDataContext>::Uuid()) || // generic function
                                (method->GetNumArguments() == 3 && method->GetArgument(0)->m_typeId == behaviorClass->m_typeId)) // it should accept that class pointer + index + value
                            {
                                methodName = "__AZ_NewIndex"; /// internal name
                            }
                            else
                            {
                                AZ_Error("Script", false, "Script required function format for IndexWrite is void(%s*,IndexType,Value)!", behaviorClass->m_name.c_str());
                                return false;
                            }
                            break;
                        default:
                            AZ_Error("Script", false, "Unknown script operator attribute!");
                            break;
                    }
                }

                return true;
            }

            // Generally types have well formed names by default, however when you involve templates, this is not the case
            // Are trying to make sure the user provides good names for those automatic template, types, however sometimes the user doesn't care
            // and we need to make sure we cover the generic case. Lua support alphanumeric + '_' so we will need to make sure the name is a valid lua name.
            const char* ValidateName(const char* source)
            {
                AZStd::string newName = AZ::ReplaceCppArtifacts(source);
                if (newName != source)
                {
                    source = m_modifiedClassNames.insert(newName).first->c_str();
                }
                return source;
            }

            //////////////////////////////////////////////////////////////////////////
            void BindClassMethodAndProperties(BehaviorClass* behaviorClass)
            {
                LSV_BEGIN(m_lua, 0);

                // methods
                for (auto methodIt : behaviorClass->m_methods)
                {
                    BehaviorMethod* method = methodIt.second;
                    const char* methodName = ValidateName(methodIt.first.c_str());

                    // Check for "ignore" attribute
                    if (FindAttribute(Script::Attributes::Ignore, method->m_attributes))
                    {
                        continue; // skip this method
                    }

                    // find special operators (we can replace the methodName and method itself
                    if (!HandleClassOperators(behaviorClass, methodName, method))
                    {
                        continue; // skip this operator
                    }

                    if (method->m_overload)
                    {
                        auto overload = method;
                        auto overloadNames = GetOverloadNames(*method, methodName);
                        auto overloadName = overloadNames.begin();

                        do
                        {
                            lua_pushstring(m_lua, overloadName->data()); // push method name on the stack
                            BindMethodOnStack(m_context, overload); // push the method
                            lua_rawset(m_lua, -3); // member function set it in the class table
                            ++overloadName;
                            overload = overload->m_overload;
                        }
                        while (overload);
                    }
                    else
                    {
                        lua_pushstring(m_lua, methodName); // push method name on the stack
                        BindMethodOnStack(m_context, method); // push the method
                        lua_rawset(m_lua, -3); // member function set it in the class table
                    }
                }

                // properties
                for (auto propertyIt : behaviorClass->m_properties)
                {
                    BehaviorProperty* prop = propertyIt.second;

                    // Check for "ignore" attribute
                    if (FindAttribute(Script::Attributes::Ignore, prop->m_attributes))
                    {
                        continue; // skip this method
                    }

                    lua_pushstring(m_lua, ValidateName(propertyIt.first.c_str()));

                    if (prop->m_getter)
                    {
                        BindMethodOnStack(m_context, prop->m_getter);
                    }
                    else
                    {
                        lua_pushnil(m_lua);
                    }

                    if (prop->m_setter)
                    {
                        BindMethodOnStack(m_context, prop->m_setter);
                    }
                    else
                    {
                        lua_pushnil(m_lua);
                    }

                    lua_pushcclosure(m_lua, &Internal::LuaPropertyTagHelper, 2); // register property

                    lua_rawset(m_lua, -3); // push the property in the class table
                }

                // pull in all base classes members and properties
                for (const AZ::Uuid& baseTypeId : behaviorClass->m_baseClasses)
                {
                    auto baseClassIt = m_context->m_typeToClassMap.find(baseTypeId);
                    if (baseClassIt != m_context->m_typeToClassMap.end())
                    {
                        BindClassMethodAndProperties(baseClassIt->second);
                    }
                }

                // if we wrap a type pull it's members and properties
                if (behaviorClass->m_unwrapper)
                {
                    auto wrappedClassIt = m_context->m_typeToClassMap.find(behaviorClass->m_wrappedTypeId);
                    if (wrappedClassIt != m_context->m_typeToClassMap.end())
                    {
                        BindClassMethodAndProperties(wrappedClassIt->second);
                    }
                }
            }

            //////////////////////////////////////////////////////////////////////////
            void BindGlobalMethod(const char* methodName, BehaviorMethod* method, int globalTableIndex)
            {
                LSV_BEGIN(m_lua, 0);

                // Check for "ignore" attribute
                if (FindAttribute(Script::Attributes::Ignore, method->m_attributes))
                {
                    return; // skip this method
                }

                // register in VM
                lua_pushstring(m_lua, ValidateName(methodName));

                BindMethodOnStack(m_context, method);

                lua_rawset(m_lua, globalTableIndex); // member function set it in the table
            }

            //////////////////////////////////////////////////////////////////////////
            void BindGlobalProperty(BehaviorProperty* prop, int globalTableIndex)
            {
                LSV_BEGIN(m_lua, 0);

                // Check for "ignore" attribute
                if (FindAttribute(Script::Attributes::Ignore, prop->m_attributes))
                {
                    return; // skip this method
                }

                // TODO: check attributes for a "special custom call"

                lua_pushstring(m_lua, ValidateName(prop->m_name.c_str()));

                if (prop->m_getter)
                {
                    BindMethodOnStack(m_context, prop->m_getter);
                }
                else
                {
                    lua_pushnil(m_lua);
                }

                if (prop->m_setter)
                {
                    BindMethodOnStack(m_context, prop->m_setter);
                }
                else
                {
                    lua_pushnil(m_lua);
                }

                lua_pushcclosure(m_lua, &Internal::LuaPropertyTagHelper, 2);
                lua_rawset(m_lua, globalTableIndex);
            }

            //////////////////////////////////////////////////////////////////////////
            void BindClass(BehaviorClass* behaviorClass)
            {
                LSV_BEGIN(m_lua, 0);

                // Do not bind if this class should not be available in Lua
                if (!AZ::Internal::IsAvailableInLua(behaviorClass->m_attributes))
                {
                    return;
                }

                // set storage type
                Script::Attributes::StorageType storageType = Script::Attributes::StorageType::ScriptOwn;
                Attribute* ownershipAttribute = FindAttribute(Script::Attributes::Storage, behaviorClass->m_attributes);
                if (ownershipAttribute)
                {
                    AttributeReader ownershipAttrReader(nullptr, ownershipAttribute);
                    ownershipAttrReader.Read<Script::Attributes::StorageType>(storageType);

                    if (storageType == Script::Attributes::StorageType::Value)
                    {
                        bool isError = false;
                        if (behaviorClass->m_cloner == nullptr)
                        {
                            AZ_Error("Script", false, "Class %s was reflected to be stored by value, however class can't be copy constructed!", behaviorClass->m_name.c_str());
                            isError = true;
                        }

                        if (behaviorClass->m_alignment > 16)
                        {
                            AZ_Error("Script", false, "Class %s was reflected to be stored by value, however it has alignment %d which is more than maximum support of 16 bytes!", behaviorClass->m_name.c_str(), behaviorClass->m_alignment);
                            isError = true;
                        }

                        if (isError)
                        {
                            return;
                        }
                    }
                }

                //////////////////////////////////////////////////////////////////////////
                // Register class and it's metatables
                lua_createtable(m_lua, 0, 5);

                // set the () operator
                // check for custom constructor and if so attach it
                // \note For ultimate flexibility we can choose constructor depending on the parameters, but for we will preset it.
                BehaviorMethod* customConstructorMethod = nullptr;
                {
                    AZ::Attribute* customConstructorAttr = FindAttribute(Script::Attributes::ConstructorOverride, behaviorClass->m_attributes);
                    if (customConstructorAttr)
                    {
                        customConstructorMethod = reinterpret_cast<BehaviorMethod*>(customConstructorAttr->GetContextData());
                    }
                    // Check all constructors if they have use ScriptDataContext and if so choose this one
                    if (!customConstructorMethod)
                    {
                        int overrideIndex = -1;
                        AZ::AttributeReader(nullptr, FindAttribute
                            ( Script::Attributes::DefaultConstructorOverrideIndex, behaviorClass->m_attributes)).Read<int>(overrideIndex);

                        int methodIndex = 0;
                        for (BehaviorMethod* method : behaviorClass->m_constructors)
                        {
                            if (methodIndex == overrideIndex)
                            {
                                customConstructorMethod = method;
                                break;
                            }

                            if (method->GetNumArguments() && method->GetArgument(method->GetNumArguments() - 1)->m_typeId == AZ::AzTypeInfo<ScriptDataContext>::Uuid())
                            {
                                customConstructorMethod = method;
                                break;
                            }

                            ++methodIndex;
                        }
                    }

                    if (!customConstructorMethod && behaviorClass->m_defaultConstructor == nullptr)
                    {
                        // grab the first constructor if we don't have a default one
                        if (!behaviorClass->m_constructors.empty())
                        {
                            customConstructorMethod = behaviorClass->m_constructors[0];
                        }
                    }
                }
                if (behaviorClass->m_defaultConstructor || customConstructorMethod)
                {
                    lua_pushliteral(m_lua, "__call");

                    // if we can actually create the class in LUA
                    LuaCaller* customConstructor = nullptr;
                    if (customConstructorMethod)
                    {
                        customConstructor = CreateLuaCaller(m_context, customConstructorMethod);
                    }

                    lua_pushlightuserdata(m_lua, customConstructor);
                    lua_pushcclosure(m_lua, &DefaultBehaviorCaller::Create, 1);
                    lua_rawset(m_lua, -3);
                }

                lua_pushliteral(m_lua, "__gc");
                // check attribute for a custom destructor
                lua_pushcclosure(m_lua, &DefaultBehaviorCaller::Destroy, 0);
                lua_rawset(m_lua, -3);

                {
                    lua_pushliteral(m_lua, "__index");

                    if (FindAttribute(Script::Attributes::UseClassIndexAllowNil, behaviorClass->m_attributes))
                    {
                        lua_pushcclosure(m_lua, &Internal::Class__IndexAllowNil, 0);
                    }
                    else
                    {
                        lua_pushcclosure(m_lua, &Internal::Class__Index, 0);
                    }

                    lua_rawset(m_lua, -3);
                }

                lua_pushliteral(m_lua, "__newindex");
                lua_pushcclosure(m_lua, &Internal::Class__NewIndex, 0);
                lua_rawset(m_lua, -3);

                lua_pushliteral(m_lua, "__tostring");
                lua_pushcclosure(m_lua, &Internal::Class__ToString, 0);
                lua_rawset(m_lua, -3);
                //////////////////////////////////////////////////////////////////////////

                // push the 2 values (class name and storage policy for quick retrieval)
                // TODO: Check class name override attribute
                lua_pushstring(m_lua, ValidateName(behaviorClass->m_name.c_str()));
                lua_rawseti(m_lua, -2, AZ_LUA_CLASS_METATABLE_NAME_INDEX);

                lua_pushlightuserdata(m_lua, behaviorClass);
                lua_rawseti(m_lua, -2, AZ_LUA_CLASS_METATABLE_BEHAVIOR_CLASS);

                Internal::azlua_pushtypeid(m_lua, behaviorClass->m_typeId);
                lua_rawseti(m_lua, -2, AZ_LUA_CLASS_METATABLE_TYPE_INDEX);


                lua_pushinteger(m_lua, static_cast<int>(storageType));
                lua_rawseti(m_lua, -2, AZ_LUA_CLASS_METATABLE_STORAGE_TYPE_INDEX);

                //lua_pushlightuserdata(l, (void*)storageCreator);
                //lua_rawseti(l, -2, AZ_LUA_CLASS_METATABLE_STORAGE_CREATOR_INDEX);

                BindClassMethodAndProperties(behaviorClass);

                if (FindAttribute(AZ::Script::Attributes::EventHandlerCreationFunction, behaviorClass->m_attributes))
                {
                    BindEventSupport();
                }

                if (storageType != Script::Attributes::StorageType::Value)
                {
                    lua_pushliteral(m_lua, "AcquireOwnership"); // method name

                    // If we are not stored by value add Acquire/Release Ownweship semantics
                    lua_pushlightuserdata(m_lua, nullptr);
                    lua_pushliteral(m_lua, "Takes ownership of the object and the object will be deleted from Lua when the last reference is gone.");
                    lua_pushcclosure(m_lua, &Internal::LuaMethodTagHelper, 0);
                    lua_pushcclosure(m_lua, &Internal::ClassAcquireOwnership, 3);
                    lua_rawset(m_lua, -3); // member function set it in the class table

                    lua_pushliteral(m_lua, "ReleaseOwnership");

                    lua_pushlightuserdata(m_lua, nullptr);
                    lua_pushliteral(m_lua, "Releases ownership of the object, the object should be deleted from the C++ runtime.");
                    lua_pushcclosure(m_lua, &Internal::LuaMethodTagHelper, 0);
                    lua_pushcclosure(m_lua, &Internal::ClassReleaseOwnership, 3);
                    lua_rawset(m_lua, -3); // member function set it in the class table
                }

                int metatableIndex = lua_gettop(m_lua);

                // create a reference to the metatable and associate it with the typeId->Metatable ID
                lua_rawgeti(m_lua, LUA_REGISTRYINDEX, AZ_LUA_CLASS_TABLE_REF); // load the class table
                AZ_Assert(!lua_isnil(m_lua, -1), "Could not find AZClassMap table! It should be initialize on script creation!");

                // TODO: If all base class offsets are 0 set the LuaDataObject::FL_SKIP_TYPE_CAST
                // to skip the RTTI in release for performance
                Internal::azlua_pushtypeid(m_lua, behaviorClass->m_typeId);
                lua_pushvalue(m_lua, metatableIndex); // copy the metatable
                lua_rawset(m_lua, -3);  // set the table entry typeId(key)->metatable(value)
                lua_pop(m_lua, 1); // pop the class table

                // Create instance table with weak references, so we know about all instances and avoid creating
                // multiple structures in LUA
                lua_createtable(m_lua, 0, 0); // create the instance table
                // Lua: instances
                lua_createtable(m_lua, 0, 1); // create instance table metatable
                // Lua: instances, instances_mt
                lua_pushliteral(m_lua, "__mode");
                // Lua: instances, instances_mt, __mode
                lua_pushliteral(m_lua, "v");
                // Lua: instances, instances_mt, __mode, "v"
                lua_rawset(m_lua, -3); // set __mode = "v"
                // Lua: instances, instances_mt
                lua_setmetatable(m_lua, -2); // set the instance table metatable
                // Lua: instances
                lua_rawseti(m_lua, -2, AZ_LUA_CLASS_METATABLE_INSTANCES); // set the instance table

                lua_createtable(m_lua, 0, 0); // create table so we can forward the calls to properties and methods to the metatable
                lua_pushvalue(m_lua, -2); // copy the class table and use it a base
                lua_setmetatable(m_lua, -2); // set the class table as a metatable for the class metatable.

                Internal::azlua_setglobal(m_lua, ValidateName(behaviorClass->m_name.c_str())); // set the metatable global name so we can call constructors and override __init and __finalize

                lua_pop(m_lua, 1);   // pop instance metatable
            }

            //////////////////////////////////////////////////////////////////////////
            void BindEBus(const char* ebusName, BehaviorEBus* ebus)
            {
                LSV_BEGIN(m_lua, 0);

                // Do not bind if this bus should not be available in Lua
                if (!AZ::Internal::IsAvailableInLua(ebus->m_attributes))
                {
                    return;
                }

                lua_createtable(m_lua, 0, 0); // create ebus table

                // events
                bool isTableUsed = false;

                if (!FindAttribute(Script::Attributes::DisallowBroadcast, ebus->m_attributes))
                {
                    // events broadcast
                    lua_pushliteral(m_lua, "Broadcast");
                    lua_createtable(m_lua, 0, 0);
                    for (auto eventIt : ebus->m_events)
                    {
                        const BehaviorEBusEventSender& eventSender = eventIt.second;

                        if (eventSender.m_broadcast)
                        {
                            // Check for "ignore" attribute
                            if (FindAttribute(Script::Attributes::Ignore, eventSender.m_broadcast->m_attributes))
                            {
                                continue; // skip this event
                            }
                            lua_pushstring(m_lua, ValidateName(eventIt.first.c_str())); // push even name
                            BindMethodOnStack(m_context, eventSender.m_broadcast);
                            lua_rawset(m_lua, -3); // push the event in the broadcast table
                            isTableUsed = true;
                        }
                    }
                    if (isTableUsed)
                    {
                        lua_rawset(m_lua, -3); // store the broadcast table
                        isTableUsed = false;
                    }
                    else
                    {
                        lua_pop(m_lua, 2); // table was never used don't store it
                    }
                }

                // events
                lua_pushliteral(m_lua, "Event");
                lua_createtable(m_lua, 0, 0);
                for (auto eventIt : ebus->m_events)
                {
                    const BehaviorEBusEventSender& eventSender = eventIt.second;

                    if (eventSender.m_event)
                    {
                        // Check for "ignore" attribute
                        if (FindAttribute(Script::Attributes::Ignore, eventSender.m_event->m_attributes))
                        {
                            continue; // skip this event
                        }
                        lua_pushstring(m_lua, ValidateName(eventIt.first.c_str())); // push even name
                        BindMethodOnStack(m_context, eventSender.m_event);
                        lua_rawset(m_lua, -3); // push the event in the event table
                        isTableUsed = true;
                    }
                }
                if (isTableUsed)
                {
                    lua_rawset(m_lua, -3); // store the event table
                    isTableUsed = false;
                }
                else
                {
                    lua_pop(m_lua, 2); // table was never used don't store it
                }


                if (!FindAttribute(Script::Attributes::DisallowBroadcast, ebus->m_attributes))
                {
                    // queue broadcast
                    lua_pushliteral(m_lua, "QueueBroadcast");
                    lua_createtable(m_lua, 0, 0);
                    for (auto eventIt : ebus->m_events)
                    {
                        const BehaviorEBusEventSender& eventSender = eventIt.second;

                        if (eventSender.m_queueBroadcast)
                        {
                            // Check for "ignore" attribute
                            if (FindAttribute(Script::Attributes::Ignore, eventSender.m_queueBroadcast->m_attributes))
                            {
                                continue; // skip this event
                            }
                            lua_pushstring(m_lua, ValidateName(eventIt.first.c_str())); // push even name
                            BindMethodOnStack(m_context, eventSender.m_queueBroadcast);
                            lua_rawset(m_lua, -3); // push the event in the queue broadcast table
                            isTableUsed = true;
                        }
                    }
                    if (isTableUsed)
                    {
                        lua_rawset(m_lua, -3); // store the queue broadcast table
                        isTableUsed = false;
                    }
                    else
                    {
                        lua_pop(m_lua, 2); // table was never used don't store it
                    }
                }

                // queue event
                lua_pushliteral(m_lua, "QueueEvent");
                lua_createtable(m_lua, 0, 0);
                for (auto eventIt : ebus->m_events)
                {
                    const BehaviorEBusEventSender& eventSender = eventIt.second;

                    if (eventSender.m_queueEvent)
                    {
                        // Check for "ignore" attribute
                        if (FindAttribute(Script::Attributes::Ignore, eventSender.m_queueEvent->m_attributes))
                        {
                            continue; // skip this event
                        }
                        lua_pushstring(m_lua, ValidateName(eventIt.first.c_str())); // push even name
                        BindMethodOnStack(m_context, eventSender.m_queueEvent);
                        lua_rawset(m_lua, -3); // push the event in the queue event table
                        isTableUsed = true;
                    }
                }
                if (isTableUsed)
                {
                    lua_rawset(m_lua, -3); // store the queue event table
                    isTableUsed = false;
                }
                else
                {
                    lua_pop(m_lua, 2); // table was never used don't store it
                }

                // Queue function
                if (ebus->m_queueFunction)
                {
                    lua_pushliteral(m_lua, "QueueFunction");
                    lua_pushlightuserdata(m_lua, ebus);
                    lua_pushcclosure(m_lua, &LuaEBusGenericFunctionQueue::FromLua, 1);
                    lua_rawset(m_lua, -3); // store the QueueFunction in the table
                }

                // handlers
                BehaviorClass* idClass = nullptr;
                LuaLoadFromStack idFromLua = FromLuaStack(m_context, &ebus->m_idParam, idClass);

                // CreateHandler lua function
                // Example usage:
                //  handler = TickBus.CreateHandler(handlerTable);
                //  handler:Connect([connectionId]);
                lua_pushliteral(m_lua, "CreateHandler");
                lua_pushlightuserdata(m_lua, m_context);
                lua_pushlightuserdata(m_lua, ebus);
                Internal::azlua_getglobal(m_lua, s_luaEBusHandlerMetatableName); // load the LuaEBusHandler metatable
                lua_pushlightuserdata(m_lua, (void*)idFromLua);
                lua_pushlightuserdata(m_lua, idClass);
                lua_pushboolean(m_lua, false); // Whether to autoConnect handler to bus
                lua_pushcclosure(m_lua, &LuaEBusHandler::CreateHandler, 6);
                lua_rawset(m_lua, -3); // store the CreateHandler in the table

                // Connect lua function (shortcut for CreateHandler + calling Connect manually on that handler)
                // Example usage:
                //  handler = TickBus.Connect(handlerTable[, connectionId]);
                lua_pushliteral(m_lua, "Connect");
                lua_pushlightuserdata(m_lua, m_context);
                lua_pushlightuserdata(m_lua, ebus);
                Internal::azlua_getglobal(m_lua, s_luaEBusHandlerMetatableName); // load the LuaEBusHandler metatable
                lua_pushlightuserdata(m_lua, (void*)idFromLua);
                lua_pushlightuserdata(m_lua, idClass);
                lua_pushboolean(m_lua, true); // Whether to autoConnect handler to bus
                lua_pushcclosure(m_lua, &LuaEBusHandler::CreateHandler, 6);
                lua_rawset(m_lua, -3); // store the Connect function in the table

                // current ID if available
                if (ebus->m_getCurrentId)
                {
                    lua_pushliteral(m_lua, "GetCurrentBusId");
                    BindMethodOnStack(m_context, ebus->m_getCurrentId);
                    lua_rawset(m_lua, -3);
                }

                Internal::azlua_setglobal(m_lua, ValidateName(ebusName));
            }

            static int ConnectToExposedEvent(lua_State* lua)
            {
                if (!(lua_isuserdata(lua, -2) && !lua_islightuserdata(lua, -2)))
                {
                    ScriptContext::FromNativeContext(lua)->Error(ScriptContext::ErrorType::Error, true, "1st argument to ConnectToExposedEvent is not userdata");
                    return 0;
                }

                if (!lua_isfunction(lua, -1))
                {
                    ScriptContext::FromNativeContext(lua)->Error(ScriptContext::ErrorType::Error, true, "2nd argument to ConnectToExposedEvent is not a function (lambda need to get around atypically routed arguments)");
                    return 0;
                }

                auto userData = reinterpret_cast<AZ::LuaUserData*>(lua_touserdata(lua, -2));
                AZ_Assert(userData && userData->magicData == AZ_CRC_CE("AZLuaUserData"), "1st argument is not user AZ supported userdata");
                AZ::Attribute* eventHandlerCreationFunctionAttribute = FindAttribute(AZ::Script::Attributes::EventHandlerCreationFunction, userData->behaviorClass->m_attributes);
                AZ_Assert(eventHandlerCreationFunctionAttribute, "failure to expose AZ::Event type in class reflected to Lua");
                AZ::AttributeReader attributeReader(nullptr, eventHandlerCreationFunctionAttribute);
                AZ::EventHandlerCreationFunctionHolder holder;
                attributeReader.Read<EventHandlerCreationFunctionHolder>(holder);

                // Lua: ExposedEvent, lambda
                lua_pushvalue(lua, -1);
                // Lua: ExposedEvent, lambda, lambda
                auto handlerAndType = AZStd::invoke(holder.m_function, userData->value, AZStd::move(ExposedLambda(lua)));
                // Lua: ExposedEvent, lambda
                Internal::RegisteredObjectToLua(lua, handlerAndType.m_address, handlerAndType.m_typeId, ObjectToLua::ByReference, AcquisitionOnPush::ScriptAcquire);
                // Lua: ExposedEvent, lambda, handler
                return 1;
            }

            //////////////////////////////////////////////////////////////////////////
            void BindEventSupport()
            {
                // Lua: ..., class_mt
                lua_pushliteral(m_lua, "Connect");
                // Lua: ..., class_mt, "Connect"
                lua_pushcfunction(m_lua, &ConnectToExposedEvent);
                // Lua: ..., class_mt, "Connect", ConnectToExposedEvent
                lua_rawset(m_lua, -3);
                // Lua: ..., class_mt
            }

            //////////////////////////////////////////////////////////////////////////
            void BindTo(BehaviorContext* behaviorContext)
            {
                // LUA Must execute in the "C" Locale.  We add this around this root
                // call so that it doesn't have to be added to every function call inside.
                // It only truly matters around parsing of float values (commas versus periods)
                // so it doesn't need to be absolutely everywhere.  Just when its possible for
                // such code to be encountered.
                AZ::Locale::ScopedSerializationLocale scopedLocale;

                LSV_BEGIN(m_lua, 0);

                if (m_context)
                {
                    // unbind
                    BehaviorContextBus::Handler::BusDisconnect();
                }

                m_context = behaviorContext;

                if (m_context)
                {
                    // load global table
                    lua_rawgeti(m_lua, LUA_REGISTRYINDEX, AZ_LUA_GLOBALS_TABLE_REF);      // load the globals table

                    // bind global methods
                    for (auto methodIt : behaviorContext->m_methods)
                    {
                        BindGlobalMethod(methodIt.first.c_str(), methodIt.second, lua_gettop(m_lua));
                    }

                    // bind global properties
                    for (auto propertyIt : behaviorContext->m_properties)
                    {
                        BindGlobalProperty(propertyIt.second, lua_gettop(m_lua));
                    }

                    // bind classes
                    for (auto classIt : behaviorContext->m_classes)
                    {
                        BindClass(classIt.second);
                    }

                    // bind EBuses
                    LuaEBusHandler::Register(m_lua);
                    for (auto ebusIt : behaviorContext->m_ebuses)
                    {
                        BindEBus(ebusIt.first.c_str(), ebusIt.second);
                    }

                    lua_pop(m_lua, 1); // pop the global table

                    // Listen for changes in the behavior context we are bound to
                    BehaviorContextBus::Handler::BusConnect(m_context);
                }
            }

            //////////////////////////////////////////////////////////////////////////
            //////////////////////////////////////////////////////////////////////////
            // BehaviorContextBus

            //////////////////////////////////////////////////////////////////////////
            void OnAddGlobalMethod(const char* methodName, BehaviorMethod* method) override
            {
                LSV_BEGIN(m_lua, 0);

                lua_rawgeti(m_lua, LUA_REGISTRYINDEX, AZ_LUA_GLOBALS_TABLE_REF);      // load the globals table

                BindGlobalMethod(methodName, method, lua_gettop(m_lua));

                lua_pop(m_lua, 1); // pop the global table
            }

            //////////////////////////////////////////////////////////////////////////
            void OnAddGlobalProperty(const char* propertyName, BehaviorProperty* prop) override
            {
                LSV_BEGIN(m_lua, 0);

                lua_rawgeti(m_lua, LUA_REGISTRYINDEX, AZ_LUA_GLOBALS_TABLE_REF);      // load the globals table

                (void)propertyName;
                BindGlobalProperty(prop, lua_gettop(m_lua));

                lua_pop(m_lua, 1); // pop the global table
            }

            //////////////////////////////////////////////////////////////////////////
            void OnAddClass(const char* /*className*/, BehaviorClass* behaviorClass) override
            {
                LSV_BEGIN(m_lua, 0);

                BindClass(behaviorClass);
            }

            //////////////////////////////////////////////////////////////////////////
            void OnRemoveClass(const char* className, BehaviorClass* behaviorClass) override
            {
                LSV_BEGIN(m_lua, 0);

                // create a reference to the metatable and associate it with the typeId->Metatable ID
                lua_rawgeti(m_lua, LUA_REGISTRYINDEX, AZ_LUA_CLASS_TABLE_REF); // load the class table
                AZ_Assert(!lua_isnil(m_lua, -1), "Could not find AZClassMap table! It should be initialize on script creation!");

                // TODO: If all base class offsets are 0 set the LuaDataObject::FL_SKIP_TYPE_CAST
                // to skip the RTTI in release for performance
                Internal::azlua_pushtypeid(m_lua, behaviorClass->m_typeId);
                lua_pushnil(m_lua); // push nil so the entry is cleared
                lua_rawset(m_lua, -3); // set the table entry typeId(key)->metatable(value)
                lua_pop(m_lua, 1); // pop the class table

                // Remove the global class name
                lua_pushnil(m_lua);
                Internal::azlua_setglobal(m_lua, ValidateName(className));
            }

            //////////////////////////////////////////////////////////////////////////
            void OnAddEBus(const char* ebusName, BehaviorEBus* ebus) override
            {
                LSV_BEGIN(m_lua, 0);

                BindEBus(ebusName, ebus);
            }

            //////////////////////////////////////////////////////////////////////////
            void OnRemoveEBus(const char* ebusName, BehaviorEBus*) override
            {
                LSV_BEGIN(m_lua, 0);

                lua_pushnil(m_lua);
                Internal::azlua_setglobal(m_lua, ValidateName(ebusName));
            }

            //////////////////////////////////////////////////////////////////////////
            /// execute all script loaded though load function, plus the one you supply as string
            bool Execute(const char* script = nullptr, const char* dbgName = nullptr, size_t dataSize = 0)
            {
                AZ::Locale::ScopedSerializationLocale scopedLocale;

                LSV_BEGIN(m_lua, 0);

                if (script)
                {
                    dbgName = dbgName ? dbgName : script;
                    AZ_DBG_NAME_FIXER(dbgTrimmedName, ScriptContext::m_maxDbgName, dbgName);

                    int parseResult = luaL_loadbuffer(m_lua, script, dataSize == 0 ? strlen(script) : dataSize, dbgTrimmedName);
                    if (0 != parseResult)
                    {
                        m_owner->Error(ScriptContext::ErrorType::Error, false, "Lua parse error (%d): %s", parseResult, lua_tostring(m_lua, -1));
                        lua_pop(m_lua, 1);
                        return false;
                    }
                }
                if (m_debug)
                {
                    m_debug->PushCodeCallstack();
                }
                bool callSuccess = Internal::LuaSafeCall(m_lua, 0, 0);
                if (m_debug)
                {
                    m_debug->PopCallstack();
                }
                return callSuccess;
            }

            //////////////////////////////////////////////////////////////////////////
            // Load a stream as a function (module)
            bool LoadFromStream(IO::GenericStream* stream, const char* debugName, const char* mode, lua_State* lua)
            {
                AZ::Locale::ScopedSerializationLocale scopedLocale;
                LSV_BEGIN(m_lua, 1);

                using namespace Internal;

                lua_State* thread = lua ? lua : NativeContext();

                LuaReadUserData userData;
                userData.m_bytesLeftInFile = stream->GetLength();
                userData.m_fileStream = stream;

                // If the file is smaller than the chunk size, just allocate the file size.
                size_t allocationSize = static_cast<size_t>(userData.m_bytesLeftInFile < s_luaFileChunkSize ? userData.m_bytesLeftInFile : s_luaFileChunkSize);
                userData.m_fileChunk.resize(allocationSize, 0);

                // Load file
                // lua_load should ALWAYS place a function on the stack, but be sure just in case.
                if (lua_load(thread, LuaStreamReader, &userData, debugName, mode) == 0)
                {
                    return lua_isfunction(thread, -1);
                }

                // If load didn't return 0, handle error

                // If not string, pop it and push a string on.
                if (!lua_isstring(thread, -1))
                {
                    lua_pop(thread, 1);
                    lua_pushfstring(thread, "Failed to load script %s.", debugName);
                }

                AZ_Warning("LuaContext",false, "Failed to load script: %s", debugName);

                return false;
            }

            //////////////////////////////////////////////////////////////////////////
            inline lua_State* NativeContext()
            {
                return m_lua;
            }

            //////////////////////////////////////////////////////////////////////////
            void GarbageCollect()
            {
                lua_gc(m_lua, LUA_GCCOLLECT, 0);
            }

            //////////////////////////////////////////////////////////////////////////
            // Debugging
            void Error(ScriptContext::ErrorType error, bool doStackTrace, const char* format, va_list mark)
            {
                // max size due to requirements of \Legacy\CrySystem\Log.h MAX_TEMP_LENGTH_SIZE(2048) minus room for the time stamp (128)
                // otherwise if the script passes in a string larger, their script will cause an assert
                char message[4096 - 128];
                azvsnprintf(message, AZ_ARRAY_SIZE(message), format, mark);

                if (doStackTrace)
                {
                    auto messageLength = strlen(message);
                    AZ::Internal::LuaStackTrace(NativeContext(), message + messageLength, AZ_ARRAY_SIZE(message) - messageLength);
                }

                if (m_errorHook)
                {
                    m_errorHook(m_owner, error, message);
                }
                else
                {
#if (!defined(_RELEASE))
                    switch (error)
                    {
                    case ScriptContext::ErrorType::Error:
                        AZ_Error("Script", false, "%s", message);
                        break;
                    case ScriptContext::ErrorType::Warning:
                        AZ_Warning("Script", false, "%s", message);
                        break;
                    case ScriptContext::ErrorType::Log:
                        AZ_Printf("Script", "%s", message);
                        break;
                    }
#endif
                }
            }

            //////////////////////////////////////////////////////////////////////////
            ScriptContextDebug* GetDebugContext()
            {
                return m_debug;
            }

            //////////////////////////////////////////////////////////////////////////
            void SetErrorHook(ScriptContext::ErrorHook cb)
            {
                m_errorHook = cb;
            }

            //////////////////////////////////////////////////////////////////////////
            ScriptContext::ErrorHook GetErrorHook() const
            {
                return m_errorHook;
            }

            //////////////////////////////////////////////////////////////////////////
            void SetRequireHook(ScriptContext::RequireHook hook)
            {
                LSV_BEGIN(m_lua, 0);

                lua_State* lua = NativeContext();

                // Get the package table
                lua_getglobal(lua, "package");

                // We'll call this newloaders
                lua_createtable(lua, 2, 0);

                // Stack: package, newloaders

                // Push our loader
                lua_pushlightuserdata(lua, m_owner);
                auto hookPtr = lua_newuserdata(lua, sizeof(ScriptContext::RequireHook));
                new (hookPtr) ScriptContext::RequireHook(hook);
                lua_pushcclosure(lua, Internal::LuaRequireHandler, 2);

                // Stack: package, newloaders, ourloader

                // Set our loader as the second element in new loaders
                lua_rawseti(lua, -2, 1);

                // Stack: package, newloaders

                // Set newloaders as the global loaders table
                lua_setfield(lua, -2, "searchers");

                // Remove package from the stack
                lua_pop(lua, 1);
            }

            //////////////////////////////////////////////////////////////////////////
            ScriptProperty* ConstructScriptProperty(ScriptDataContext& sdc, int valueIndex, const char* name, bool restrictToPropertyArrays)
            {
                LSV_BEGIN(m_lua, 0);

                AZ::ScriptProperty* scriptProperty = nullptr;

                for (ScriptTypeFactory factory : m_scriptPropertyFactories)
                {
                    scriptProperty = factory(sdc, valueIndex, name);

                    if (scriptProperty != nullptr)
                    {
                        break;
                    }
                }

                if (scriptProperty == nullptr)
                {
                    if (restrictToPropertyArrays)
                    {
                        // If we are restricting ourselves to vectors of objects(in essence). Need to try to construct
                        // the individual types of arrays.
                        for (ScriptTypeFactory factory : m_scriptPropertyArrayFactories)
                        {
                            scriptProperty = factory(sdc,valueIndex,name);

                            if (scriptProperty != nullptr)
                            {
                                break;
                            }
                        }
                    }
                    else
                    {
                        scriptProperty = m_scriptPropertyTableFactory(sdc,valueIndex,name);
                    }
                }

                return scriptProperty;
            }

            ScriptContext* m_owner;
            AZStd::unordered_set<LuaScriptCaller*> m_methods;
            AZStd::unordered_set<LuaGenericCaller*> m_genericMethods;
            AZStd::unordered_set<LuaEBusHandler*> m_ebusHandler;
            AZStd::unordered_set<AZStd::string> m_modifiedClassNames;
            BehaviorContext* m_context;
            lua_State* m_lua;
            bool m_isCustomLuaVM;
            ScriptContextDebug* m_debug;
            ScriptContext::ErrorHook m_errorHook; ///< Optional script context error hook (to change default behavior).
            AZStd::vector< ScriptTypeFactory >  m_scriptPropertyFactories;
            AZStd::vector< ScriptTypeFactory >  m_scriptPropertyArrayFactories;
            ScriptTypeFactory                   m_scriptPropertyTableFactory;
            Internal::LuaSystemAllocator m_luaAllocator;
            AZStd::thread::id m_ownerThreadId; // Check if Lua methods (including EBus handlers) are called from background threads.
        };

    ScriptContext::ScriptContext(ScriptContextId id, IAllocator* allocator, lua_State* nativeContext)
    {
        m_id = id;
        m_impl = aznew ScriptContextImpl(this, allocator, nativeContext);
    }
    ScriptContext::~ScriptContext()
    {
        delete m_impl;
    }

    void ScriptContext::BindTo(BehaviorContext* behaviorContext)
    {
        m_impl->BindTo(behaviorContext);
    }

    BehaviorContext* ScriptContext::GetBoundContext() const
    {
        return m_impl->m_context;
    }

    /// execute all script loaded though load function, plus the one you supply as string
    bool ScriptContext::Execute(const char* script, const char* dbgName, size_t dataSize)
    {
        return m_impl->Execute(script, dbgName, dataSize);
    }

    bool ScriptContext::LoadFromStream(IO::GenericStream* stream, const char* debugName, const char* mode, lua_State* thread)
    {
        return m_impl->LoadFromStream(stream, debugName, mode, thread);
    }

    //////////////////////////////////////////////////////////////////////////
    void ScriptContext::GarbageCollect()
    {
        lua_gc(m_impl->m_lua, LUA_GCCOLLECT, 0);
    }

    //////////////////////////////////////////////////////////////////////////
    void ScriptContext::GarbageCollectStep(int numberOfSteps)
    {
        lua_gc(m_impl->m_lua, LUA_GCSTEP, numberOfSteps);
    }

    //////////////////////////////////////////////////////////////////////////
    size_t ScriptContext::GetMemoryUsage() const
    {
        int kbytes = lua_gc(m_impl->m_lua, LUA_GCCOUNT, 0);
        int remainderBytes = lua_gc(m_impl->m_lua, LUA_GCCOUNTB, 0);

        // return total bytes of usage
        return (static_cast<size_t>(kbytes) * 1024) + remainderBytes;
    }

    //////////////////////////////////////////////////////////////////////////
    lua_State* ScriptContext::NativeContext()
    {
        return m_impl->NativeContext();
    }


    //////////////////////////////////////////////////////////////////////////
    bool ScriptContext::ExistsGlobal(const char* name)
    {
        Internal::azlua_getglobal(m_impl->m_lua, name);
        bool isValue = !(lua_isnil(m_impl->m_lua, -1));
        lua_pop(m_impl->m_lua, 1);
        return isValue;
    }

    //////////////////////////////////////////////////////////////////////////
    bool ScriptContext::FindGlobal(const char* name, ScriptDataContext& dc)
    {
        dc.Reset();

        Internal::azlua_getglobal(m_impl->m_lua, name);
        if (lua_isnil(m_impl->m_lua, -1))
        {
            lua_pop(m_impl->m_lua, 1);
            return false;
        }
        dc.SetInspect(m_impl->m_lua, lua_gettop(m_impl->m_lua));
        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    void ScriptContext::RemoveGlobal(const char* name)
    {
        lua_pushnil(m_impl->m_lua);
        Internal::azlua_setglobal(m_impl->m_lua, name);
    }

    //////////////////////////////////////////////////////////////////////////
    int ScriptContext::CacheGlobal(const char* name)
    {
        Internal::azlua_getglobal(m_impl->m_lua, name);
        int ref = Internal::LuaCacheValue(m_impl->m_lua, -1);
        lua_pop(m_impl->m_lua, 1); // Pop the global
        return ref;
    }

    //////////////////////////////////////////////////////////////////////////
    bool ScriptContext::FindGlobal(int cachedIndex, ScriptDataContext& dc)
    {
        if (cachedIndex > -1)
        {
            dc.Reset();
            Internal::LuaLoadCached(m_impl->m_lua,cachedIndex);
            if (!lua_isnil(m_impl->m_lua, -1))
            {
                dc.SetInspect(m_impl->m_lua, lua_gettop(m_impl->m_lua));
                return true;
            }
        }
        return false;
    }

    //////////////////////////////////////////////////////////////////////////
    void ScriptContext::ReleaseCached(int cacheIndex)
    {
        Internal::LuaReleaseCached(m_impl->m_lua, cacheIndex);
    }

    //////////////////////////////////////////////////////////////////////////
    bool ScriptContext::Call(const char* functionName, ScriptDataContext& dc)
    {
        dc.Reset();
        Internal::azlua_getglobal(m_impl->m_lua, functionName);
        if (lua_isfunction(m_impl->m_lua, -1))
        {
            dc.SetCaller(m_impl->m_lua, lua_gettop(m_impl->m_lua));
            return true;
        }
        else
        {
            lua_pop(m_impl->m_lua, 1);
        }
        return false;
    }

    //////////////////////////////////////////////////////////////////////////
    bool ScriptContext::CallCached(int cachedIndex, ScriptDataContext& dc)
    {
        dc.Reset();
        Internal::LuaLoadCached(m_impl->m_lua,cachedIndex);
        if (lua_isfunction(m_impl->m_lua, -1))
        {
            dc.SetCaller(m_impl->m_lua, lua_gettop(m_impl->m_lua));
            return true;
        }
        else
        {
            lua_pop(m_impl->m_lua, 1);
        }
        return false;
    }

    //////////////////////////////////////////////////////////////////////////
    bool ScriptContext::InspectTable(const char* tableName, ScriptDataContext& dc)
    {
        dc.Reset();
        Internal::azlua_getglobal(m_impl->m_lua, tableName);
        if (lua_istable(m_impl->m_lua, -1))
        {
            dc.SetInspect(m_impl->m_lua, lua_gettop(m_impl->m_lua));  // -1 to account for the actual table
            lua_pushnil(m_impl->m_lua); // key
            lua_pushnil(m_impl->m_lua); // value
            return true;
        }
        else
        {
            lua_pop(m_impl->m_lua, 1);
            AZ_Warning("Script", false, "Table %s doesn't exist!", tableName);
        }
        return false;
    }

    //////////////////////////////////////////////////////////////////////////
    bool ScriptContext::InspectTableCached(int cachedIndex, ScriptDataContext& dc)
    {
        dc.Reset();
        Internal::LuaLoadCached(m_impl->m_lua,cachedIndex);
        if (lua_istable(m_impl->m_lua, -1))
        {
            dc.SetInspect(m_impl->m_lua, lua_gettop(m_impl->m_lua));  // -1 to account for the actual table
            lua_pushnil(m_impl->m_lua); // key
            lua_pushnil(m_impl->m_lua); // value
            return true;
        }
        else
        {
            lua_pop(m_impl->m_lua, 1);
            AZ_Warning("Script", false, "CacheIndex %d is not a table!", cachedIndex);
        }
        return false;
    }

    //////////////////////////////////////////////////////////////////////////
    bool ScriptContext::ReadStack(ScriptDataContext& dc)
    {
        dc.Reset();

        if (lua_gettop(m_impl->m_lua) != 0)
        {
            dc.ReadStack(m_impl->m_lua);
            return true;
        }

        return false;
    }

    //////////////////////////////////////////////////////////////////////////
    ScriptContext* ScriptContext::FromNativeContext(lua_State* nativeContext)
    {
        lua_rawgeti(nativeContext, LUA_REGISTRYINDEX, AZ_LUA_SCRIPT_CONTEXT_REF);
        ScriptContext* sc = reinterpret_cast<ScriptContext*>(lua_touserdata(nativeContext, -1));
        lua_pop(nativeContext, 1);
        return sc;
    }

    //////////////////////////////////////////////////////////////////////////
    void ScriptContext::EnableDebug()
    {
        if (m_impl->m_debug == nullptr)
        {
            m_impl->m_debug = aznew ScriptContextDebug(*this);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    void ScriptContext::DisableDebug()
    {
        delete m_impl->m_debug;
        m_impl->m_debug = nullptr;
    }

    //////////////////////////////////////////////////////////////////////////
    ScriptContextDebug* ScriptContext::GetDebugContext()
    {
        return m_impl->m_debug;
    }

    //////////////////////////////////////////////////////////////////////////
    void ScriptContext::DebugSetOwnerThread(AZStd::thread::id ownerThreadId)
    {
        AZ_UNUSED(ownerThreadId);
#ifndef _RELEASE
        // By default the thread that creates the script context is the owner. This method allows to override this default behaviour.
        m_impl->m_ownerThreadId = ownerThreadId;
#endif // !_RELEASE
    }

    //////////////////////////////////////////////////////////////////////////
    bool ScriptContext::DebugIsCallingThreadTheOwner() const
    {
        return m_impl->m_ownerThreadId == AZStd::this_thread::get_id();
    }

    //////////////////////////////////////////////////////////////////////////
    void ScriptContext::SetErrorHook(ErrorHook cb)
    {
        m_impl->SetErrorHook(cb);
    }

    //////////////////////////////////////////////////////////////////////////
    ScriptContext::ErrorHook ScriptContext::GetErrorHook() const
    {
        return m_impl->GetErrorHook();
    }

    //////////////////////////////////////////////////////////////////////////
    void ScriptContext::Error(ErrorType error, bool doStackTrace, const char* format, ...)
    {
        va_list mark;
        va_start(mark, format);
        m_impl->Error(error, doStackTrace, format, mark);
        va_end(mark);
    }

    //////////////////////////////////////////////////////////////////////////
    void ScriptContext::SetRequireHook(RequireHook hook)
    {
        m_impl->SetRequireHook(hook);
    }

    //////////////////////////////////////////////////////////////////////////
    AZ::ScriptProperty* ScriptContext::ConstructScriptProperty(ScriptDataContext& sdc, int valueIndex, const char* name, bool restrictToPropertyArrays)
    {
        return m_impl->ConstructScriptProperty(sdc, valueIndex, name, restrictToPropertyArrays);
    }
} // namespace AZ

#undef AZ_DBG_NAME_FIXER
