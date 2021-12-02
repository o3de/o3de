/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ExecutionInterpretedAPI.h"

#include <AzCore/Math/Crc.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Script/ScriptSystemBus.h>
#include <AzCore/Script/lua/lua.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Core/Nodeable.h>
#include <ScriptCanvas/Core/NodeableOut.h>
#include <ScriptCanvas/Execution/Interpreted/ExecutionStateInterpreted.h>
#include <ScriptCanvas/Execution/Interpreted/ExecutionStateInterpretedUtility.h>
#include <ScriptCanvas/Execution/NodeableOut/NodeableOutNative.h>
#include <ScriptCanvas/Grammar/PrimitivesDeclarations.h>
#include <ScriptCanvas/Libraries/Math/MathNodeUtilities.h>
#include <ScriptCanvas/Utils/BehaviorContextUtils.h>

#include "ExecutionInterpretedCloningAPI.h"
#include "ExecutionInterpretedDebugAPI.h"
#include "ExecutionInterpretedEBusAPI.h"
#include "ExecutionInterpretedOut.h"

namespace ExecutionInterpretedAPICpp
{
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Execution;

    constexpr size_t k_StringFastSize = 32;

    constexpr size_t k_UuidSize = 16;

    constexpr unsigned char k_Bad = 77;

    constexpr const char* const k_fastDigits = "0123456789ABCDEF";

    constexpr unsigned char k_fastValues[] =
    {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, k_Bad,k_Bad,k_Bad,k_Bad,k_Bad,k_Bad,k_Bad, 10, 11, 12, 13, 14, 15
    };

    [[maybe_unused]] constexpr unsigned char k_FastValuesIndexSentinel = 'G' - '0';

    template<typename T>
    T* GetAs(AZ::BehaviorValueParameter& argument)
    {
        return argument.m_typeId == azrtti_typeid<T>()
            ? reinterpret_cast<T*>(argument.GetValueAddress())
            : nullptr;
    }

    int DeleteNodeable(lua_State* lua)
    {
        AZ::LuaUserData* userData = reinterpret_cast<AZ::LuaUserData*>(lua_touserdata(lua, -1));
        AZ_Assert(userData && userData->magicData == AZ_CRC_CE("AZLuaUserData"), "this isn't user data");
        delete reinterpret_cast<ScriptCanvas::Nodeable*>(userData->value);
        userData->value = nullptr;
        return 0;
    }

    int ErrorHandler(lua_State* lua)
    {
        if (lua_isstring(lua, -1))
        {
            AZ::ScriptContext::FromNativeContext(lua)->Error(AZ::ScriptContext::ErrorType::Error, true, lua_tostring(lua, -1));
        }
        else
        {
            AZ_Warning("ScriptCanvas", false, "First argument to ScriptCanvas interpreted ErrorHandler must be a string, not %s.", luaL_typename(lua, -1));
        }

        lua_pop(lua, 1);
        return 0;
    }

    int ProxyNewIndexMethod(lua_State* lua)
    {
        // Lua: ud, k, v 
        lua_pushvalue(lua, lua_upvalueindex(1));
        // Lua: ud, k, v, ?
        AZ_Assert(lua_istable(lua, -1), "the first upvalue must be a table");
        // Lua: ud, k, v, proxy
        lua_replace(lua, -4);
        // Lua: proxy k, v
        lua_rawset(lua, -3);
        // Lua: proxy
        lua_pop(lua, 1);
        // Lua: 
        return 0;
    }

    int TypeSafeEBusResultBoolean(lua_State* lua)
    {
        if (lua_isnil(lua, -1))
        {
            lua_pop(lua, 1);
            lua_pushboolean(lua, false);
        }

        return 1;
    }

    int TypeSafeEBusResultFromEntityId(lua_State* lua)
    {
        if (lua_isnil(lua, -1))
        {
            lua_pop(lua, 1);
            AZ::ScriptValue<Data::EntityIDType>::StackPush(lua, Data::EntityIDType());
        }

        return 1;
    }

    int TypeSafeEBusResultFromNamedEntityId(lua_State* lua)
    {
        if (lua_isnil(lua, -1))
        {
            lua_pop(lua, 1);
            AZ::ScriptValue<Data::NamedEntityIDType>::StackPush(lua, Data::NamedEntityIDType());
        }

        return 1;
    }

    int TypeSafeEBusResultNumber(lua_State* lua)
    {
        if (lua_isnil(lua, -1))
        {
            lua_pop(lua, 1);
            lua_pushnumber(lua, lua_Number(0.0f));
        }

        return 1;
    }

    int TypeSafeEBusResultString(lua_State* lua)
    {
        if (lua_isnil(lua, -1))
        {
            lua_pop(lua, 1);
            lua_pushstring(lua, "");
        }

        return 1;
    }

    template<typename t_Type>
    int TypeSafeEBusResultFromBehaviorContextObjectThatIsValueType(lua_State* lua)
    {
        if (lua_isnil(lua, -1))
        {
            lua_pop(lua, 1);
            AZ::ScriptValue<t_Type>::StackPush(lua, Data::Traits<t_Type>::GetDefault());
        }

        return 1;
    }

    int TypeSafeEBusMultipleReturnResults(lua_State* lua)
    {
        // Lua: ?, typeId
        if (lua_isnil(lua, -2))
        {
            AZ_Assert(lua_isstring(lua, -1), "error in compiled lua file. TypeSafeEBusMultipleReturnResults expected string 2nd argument, got %s", lua_typename(lua, -1));
            // Lua: nil, aztypeidStr
            const char* aztypeidStr = lua_tostring(lua, -1);
            AZ::TypeId typeId = CreateIdFromStringFast(aztypeidStr);
            lua_pop(lua, 2);
            // Lua: 
            AZStd::pair<void*, AZ::BehaviorContext*> multipleResults = ScriptCanvas::BehaviorContextUtils::ConstructTupleGetContext(typeId);
            AZ_Assert(multipleResults.first, "failure to construct a tuple by typeid from behavior context");
            AZ::BehaviorValueParameter parameter;
            parameter.m_value = multipleResults.first;
            parameter.m_typeId = typeId;
            AZ::StackPush(lua, multipleResults.second, parameter);
        }
        else
        {
            // Lua: AZStd::tuple, typeId
            lua_pop(lua, 1);
        }
        // Lua: AZStd::tuple
        return 1;
    }

    void RegisterTypeSafeEBusResultFunctions(lua_State* lua)
    {
        // all the single return types that are value types in ScriptCanvas, but could be returned as nil by event-results calls from Lua...
        lua_register(lua, Grammar::ToTypeSafeEBusResultName(Data::Type::AABB()).data(), &TypeSafeEBusResultFromBehaviorContextObjectThatIsValueType<Data::AABBType>);
        lua_register(lua, Grammar::ToTypeSafeEBusResultName(Data::Type::AssetId()).data(), &TypeSafeEBusResultFromBehaviorContextObjectThatIsValueType<Data::AssetIdType>);
        lua_register(lua, Grammar::ToTypeSafeEBusResultName(Data::Type::Boolean()).data(), &TypeSafeEBusResultBoolean);
        lua_register(lua, Grammar::ToTypeSafeEBusResultName(Data::Type::Color()).data(), &TypeSafeEBusResultFromBehaviorContextObjectThatIsValueType<Data::ColorType>);
        lua_register(lua, Grammar::ToTypeSafeEBusResultName(Data::Type::CRC()).data(), &TypeSafeEBusResultFromBehaviorContextObjectThatIsValueType<Data::CRCType>);
        lua_register(lua, Grammar::ToTypeSafeEBusResultName(Data::Type::EntityID()).data(), &TypeSafeEBusResultFromEntityId);
        lua_register(lua, Grammar::ToTypeSafeEBusResultName(Data::Type::Matrix3x3()).data(), &TypeSafeEBusResultFromBehaviorContextObjectThatIsValueType<Data::Matrix3x3Type>);
        lua_register(lua, Grammar::ToTypeSafeEBusResultName(Data::Type::Matrix4x4()).data(), &TypeSafeEBusResultFromBehaviorContextObjectThatIsValueType<Data::Matrix3x3Type>);
        lua_register(lua, Grammar::ToTypeSafeEBusResultName(Data::Type::NamedEntityID()).data(), &TypeSafeEBusResultFromNamedEntityId);
        lua_register(lua, Grammar::ToTypeSafeEBusResultName(Data::Type::Number()).data(), &TypeSafeEBusResultNumber);
        lua_register(lua, Grammar::ToTypeSafeEBusResultName(Data::Type::OBB()).data(), &TypeSafeEBusResultFromBehaviorContextObjectThatIsValueType<Data::OBBType>);
        lua_register(lua, Grammar::ToTypeSafeEBusResultName(Data::Type::Plane()).data(), &TypeSafeEBusResultFromBehaviorContextObjectThatIsValueType<Data::PlaneType>);
        lua_register(lua, Grammar::ToTypeSafeEBusResultName(Data::Type::Quaternion()).data(), &TypeSafeEBusResultFromBehaviorContextObjectThatIsValueType<Data::QuaternionType>);
        lua_register(lua, Grammar::ToTypeSafeEBusResultName(Data::Type::String()).data(), &TypeSafeEBusResultString);
        lua_register(lua, Grammar::ToTypeSafeEBusResultName(Data::Type::Transform()).data(), &TypeSafeEBusResultFromBehaviorContextObjectThatIsValueType<Data::TransformType>);
        lua_register(lua, Grammar::ToTypeSafeEBusResultName(Data::Type::Vector2()).data(), &TypeSafeEBusResultFromBehaviorContextObjectThatIsValueType<Data::Vector2Type>);
        lua_register(lua, Grammar::ToTypeSafeEBusResultName(Data::Type::Vector3()).data(), &TypeSafeEBusResultFromBehaviorContextObjectThatIsValueType<Data::Vector3Type>);
        lua_register(lua, Grammar::ToTypeSafeEBusResultName(Data::Type::Vector4()).data(), &TypeSafeEBusResultFromBehaviorContextObjectThatIsValueType<Data::Vector4Type>);
        // multiple results in the form of tuple are all handled the same way
        lua_register(lua, Grammar::k_TypeSafeEBusMultipleResultsName, &TypeSafeEBusMultipleReturnResults);
    }
}

namespace ScriptCanvas
{
    namespace Execution
    {
        //////////////////////////////////////////////////////////////////////////
        // \note make sure temporary allocations with methods that have a result are handled properly with memory model
        // \see LuaScriptCaller(BehaviorContext* context, BehaviorMethod* method)
        // \see m_resultToLua = ToLuaStack(context, method->GetResult(), &m_prepareResult, m_resultClass);
        // \note make sure the ebus case doesn't allocate extra space as it will always have space for the result
        //////////////////////////////////////////////////////////////////////////

        void ActivateInterpreted()
        {
            AZ::ScriptContext* scriptContext{};
            AZ::ScriptSystemRequestBus::BroadcastResult(scriptContext, &AZ::ScriptSystemRequests::GetContext, AZ::ScriptContextIds::DefaultScriptContextId);
            AZ_Assert(scriptContext, "Must have a default script context");
            RegisterAPI(scriptContext->NativeContext());
        }

        void SetInterpretedExecutionMode(BuildConfiguration configuration)
        {
            switch (configuration)
            {
            case ScriptCanvas::BuildConfiguration::Debug:
                SetInterpretedExecutionModeDebug();
                break;
            case ScriptCanvas::BuildConfiguration::Performance:
                SetInterpretedExecutionModePerformance();
                break;
            case ScriptCanvas::BuildConfiguration::Release:
                SetInterpretedExecutionModeRelease();
                break;
            default:
                AZ_Assert(false, "unhandled BuildConfiguration type");
                break;
            }
        }

        void SetInterpretedExecutionModeDebug()
        {
            AZ::ScriptContext* scriptContext{};
            AZ::ScriptSystemRequestBus::BroadcastResult(scriptContext, &AZ::ScriptSystemRequests::GetContext, AZ::ScriptContextIds::DefaultScriptContextId);
            AZ_Assert(scriptContext, "Must have a default script context");
            lua_State* lua = scriptContext->NativeContext();

            lua_pushboolean(lua, false);
            lua_setglobal(lua, Grammar::k_InterpretedConfigurationRelease);

            lua_pushboolean(lua, false);
            lua_setglobal(lua, Grammar::k_InterpretedConfigurationPerformance);
        }

        void SetInterpretedExecutionModePerformance()
        {
            AZ::ScriptContext* scriptContext{};
            AZ::ScriptSystemRequestBus::BroadcastResult(scriptContext, &AZ::ScriptSystemRequests::GetContext, AZ::ScriptContextIds::DefaultScriptContextId);
            AZ_Assert(scriptContext, "Must have a default script context");
            lua_State* lua = scriptContext->NativeContext();

            lua_pushboolean(lua, false);
            lua_setglobal(lua, Grammar::k_InterpretedConfigurationRelease);

            lua_pushboolean(lua, true);
            lua_setglobal(lua, Grammar::k_InterpretedConfigurationPerformance);
        }

        void SetInterpretedExecutionModeRelease()
        {
            AZ::ScriptContext* scriptContext{};
            AZ::ScriptSystemRequestBus::BroadcastResult(scriptContext, &AZ::ScriptSystemRequests::GetContext, AZ::ScriptContextIds::DefaultScriptContextId);
            AZ_Assert(scriptContext, "Must have a default script context");
            lua_State* lua = scriptContext->NativeContext();

            lua_pushboolean(lua, true);
            lua_setglobal(lua, Grammar::k_InterpretedConfigurationRelease);

            lua_pushboolean(lua, false);
            lua_setglobal(lua, Grammar::k_InterpretedConfigurationPerformance);
        }

        AZ::IRttiHelper* GetRttiHelper(const AZ::Uuid& azTypeId, AZ::BehaviorContext& behaviorContext)
        {
            auto bcClassIter = behaviorContext.m_typeToClassMap.find(azTypeId);
            return bcClassIter != behaviorContext.m_typeToClassMap.end() ? bcClassIter->second->m_azRtti : nullptr;
        }

        AZ::BehaviorValueParameter BehaviorValueParameterFromTypeIdString(const char* aztypeidStr, AZ::BehaviorContext& behaviorContext)
        {
            AZ::BehaviorValueParameter bvp;
            bvp.m_typeId = CreateIdFromStringFast(aztypeidStr);
            bvp.m_azRtti = GetRttiHelper(bvp.m_typeId, behaviorContext);
            return bvp;
        }

        AZStd::string CreateStringFastFromId(const AZ::Uuid& uuid)
        {
            using namespace ExecutionInterpretedAPICpp;
            static_assert(sizeof(AZ::Uuid) == k_UuidSize, "size of uuid has changed");

            AZStd::string fastString;
            fastString.resize(k_StringFastSize);
            char* fastDigit = fastString.begin();

            for (size_t i = 0; i < k_UuidSize; ++i)
            {
                const unsigned char value = uuid.data[i];
                *fastDigit = k_fastDigits[value >> 4];
                ++fastDigit;
                *fastDigit = k_fastDigits[value & 15];
                ++fastDigit;
            }

            return fastString;
        }

        AZ::Uuid CreateIdFromStringFast(const char* string)
        {
            using namespace ExecutionInterpretedAPICpp;

            AZ_Assert(string, "string argument must not be null");
            AZ_Assert(strlen(string) == k_StringFastSize, "invalid length of string, fast format must be: 0123456789ABCDEF0123456789ABCDEF");
            const char* current = string;

            AZ::Uuid id;
            auto data = id.begin();
            auto sentinel = id.end();

            do
            {
                const auto index0 = *current - '0';
                AZ_Assert(index0 < k_FastValuesIndexSentinel, "invalid value index in found in fast string, fast format must be: 0123456789ABCDEF0123456789ABCDEF");
                const auto value0 = k_fastValues[index0];
                AZ_Assert(value0 != k_Bad, "invalid value in fast string, fast format must be: 0123456789ABCDEF0123456789ABCDEF");
                ++current;

                const auto index1 = *current - '0';
                AZ_Assert(index1 < k_FastValuesIndexSentinel, "invalid value index in found in fast string, fast format must be: 0123456789ABCDEF0123456789ABCDEF");
                const auto value1 = k_fastValues[index1];
                AZ_Assert(value1 != k_Bad, "invalid value in fast string, fast format must be: 0123456789ABCDEF0123456789ABCDEF");
                ++current;

                *data = (value0 << 4) | value1;
            }             while ((++data) != sentinel);

            return id;
        }

        int OverrideNodeableMetatable(lua_State* lua)
        {
            using namespace ExecutionInterpretedAPICpp;

            /**
            Here is the function in Lua for easier reading
            function OverrideNodeableMetatable(userdata, class_mt)
                local proxy = setmetatable({}, class_mt) -- class_mt from the user graph definition
                local instance_mt = {
                    __index = proxy
                    __newindex = function(t, k, v)
                        proxy[k] = v
                    end,
                }
                --[[ -- not being done
                for name, method in pairs(metamethods) do
                    if name ~= '__index' and name ~= '__newindex' do
                        instance_mt[name] = method
                    end
                end
                --]]
                return setmetatable(userdata instance_mt) -- can't be done from Lua
            end
            --[[
            userdata to Nodeable before:
            getmetatable(userdata).__index == Nodeable

            userdata to Nodeable after:
            local override_mt = getmetatable(userdata)
            local proxy = override_mt.__index
            local SubGraph = getmetatable(proxy).__index
            getmetatable(SubGraph.__index) == Nodeable)
            --]]
            */

            // \note: all other metamethods ignored for now
            // \note: the the object is being constructed, and is assumed to never leave or re-enter Lua again
            AZ_Assert(lua_isuserdata(lua, -2) && !lua_islightuserdata(lua, -2), "Error in compiled lua file, 1st argument to OverrideNodeableMetatable is not userdata (Nodeable)");
            AZ_Assert(lua_istable(lua, -1), "Error in compiled lua file, 2nd argument to OverrideNodeableMetatable is not a Lua table");
                   
            [[maybe_unused]] auto userData = reinterpret_cast<AZ::LuaUserData*>(lua_touserdata(lua, -2));
            AZ_Assert(userData && userData->magicData == AZ_CRC_CE("AZLuaUserData"), "this isn't user data");
            // Lua: LuaUserData::nodeable, class_mt
            lua_newtable(lua);
            // Lua: LuaUserData::nodeable, class_mt, proxy
            lua_pushvalue(lua, -2);
            // Lua: LuaUserData::nodeable, class_mt, proxy, class_mt
            lua_setmetatable(lua, -2);
            // Lua: LuaUserData::nodeable, class_mt, proxy
            lua_newtable(lua);
            // Lua: LuaUserData::nodeable, class_mt, proxy, userdata_mt
            lua_pushvalue(lua, -2);
            // Lua: LuaUserData::nodeable, class_mt, proxy, userdata_mt, proxy
            lua_setfield(lua, -2, "__index");
            // Lua: LuaUserData::nodeable, class_mt, proxy, userdata_mt
            lua_pushvalue(lua, -2);
            // Lua: LuaUserData::nodeable, class_mt, proxy, userdata_mt, proxy
            lua_pushcclosure(lua, &ProxyNewIndexMethod, 1);
            // Lua: LuaUserData::nodeable, class_mt, proxy, userdata_mt, ProxyNewIndexMethod
            lua_setfield(lua, -2, "__newindex");
            // Lua: LuaUserData::nodeable, class_mt, proxy, userdata_mt
            lua_pushcfunction(lua, &DeleteNodeable);
            // Lua: LuaUserData::nodeable, class_mt, proxy, userdata_mt, DeleteNodeable
            lua_setfield(lua, -2, "__gc");
            // Lua: LuaUserData::nodeable, class_mt, proxy, userdata_mt
            lua_setmetatable(lua, -4);
            // Lua: LuaUserData::nodeable, class_mt, proxy
            lua_pop(lua, 2);
            // Lua: LuaUserData::nodeable            
            AZ_Assert(reinterpret_cast<AZ::LuaUserData*>(lua_touserdata(lua, -1)) == userData, "must leave original userdata at the top of the stack");
            return 1;
        }

        void PushActivationArgs(lua_State* lua, AZ::BehaviorValueParameter* arguments, size_t numArguments)
        {
            auto behaviorContext = AZ::ScriptContext::FromNativeContext(lua)->GetBoundContext();

            for (size_t i = 0; i < numArguments; ++i)
            {
                Execution::StackPush(lua, behaviorContext, arguments[i]);
            }
        }

        int GetRandomSwitchControlNumber(lua_State* lua)
        {
            lua_pushnumber(lua, MathNodeUtilities::GetRandom(lua_Number(0), lua_tonumber(lua, -1)));
            return 1;
        }

        void RegisterAPI(lua_State* lua)
        {
            using namespace ScriptCanvas::Grammar;
            using namespace ExecutionInterpretedAPICpp;

            lua_register(lua, k_InitializeNodeableOutKeys, &InitializeNodeableOutKeys);
            lua_register(lua, k_NodeableCallInterpretedOut, &CallExecutionOut);
            lua_register(lua, k_NodeableSetExecutionOutName, &SetExecutionOut);
            lua_register(lua, k_NodeableSetExecutionOutResultName, &SetExecutionOutResult);
            lua_register(lua, k_NodeableSetExecutionOutUserSubgraphName, &SetExecutionOutUserSubgraph);
            lua_register(lua, k_OverrideNodeableMetatableName, &OverrideNodeableMetatable);
            lua_register(lua, k_UnpackDependencyConstructionArgsFunctionName, &UnpackDependencyConstructionArgs);
            lua_register(lua, k_UnpackDependencyConstructionArgsLeafFunctionName, &UnpackDependencyConstructionArgsLeaf);

#if defined(_RELEASE)
            lua_pushboolean(lua, false);
            lua_setglobal(lua, k_InterpretedConfigurationPerformance);
            lua_pushboolean(lua, true);
            lua_setglobal(lua, k_InterpretedConfigurationRelease);
#else
            // all others default to debug configuration
            lua_pushboolean(lua, false);
            lua_setglobal(lua, k_InterpretedConfigurationPerformance);
            lua_pushboolean(lua, false);
            lua_setglobal(lua, k_InterpretedConfigurationRelease);
#endif

            lua_register(lua, k_GetRandomSwitchControlNumberName, &GetRandomSwitchControlNumber);

            RegisterTypeSafeEBusResultFunctions(lua);
            RegisterCloningAPI(lua);
            RegisterDebugAPI(lua);
            RegisterEBusHandlerAPI(lua);
            lua_gc(lua, LUA_GCCOLLECT, 0);
        }

        int CallExecutionOut(lua_State* lua)
        {
            // Lua: executionState, outKey, args...
            const int argsCount = lua_gettop(lua);
            AZ_Assert(argsCount >= 2, "CallExecutionOut: Error in compiled Lua file, not enough arguments");
            AZ_Assert(lua_isuserdata(lua, 1), "CallExecutionOut: Error in compiled lua file, 1st argument to SetExecutionOut is not userdata (Nodeable)");
            AZ_Assert(lua_isnumber(lua, 2), "CallExecutionOut: Error in compiled lua file, 2nd argument to SetExecutionOut is not a number");
            Nodeable* nodeable = AZ::ScriptValue<Nodeable*>::StackRead(lua, 1);
            size_t index = aznumeric_caster(lua_tointeger(lua, 2));
            nodeable->CallOut(index, nullptr, nullptr, argsCount - 2);
            // Lua: results...
            return lua_gettop(lua);
        }

        void InitializeInterpretedStatics(RuntimeData& runtimeData)
        {
            if (!runtimeData.m_areStaticsInitialized)
            {
                runtimeData.m_areStaticsInitialized = true;

                for (auto& dependency : runtimeData.m_requiredAssets)
                {
                    InitializeInterpretedStatics(dependency.Get()->GetData());
                }

#if defined(AZ_PROFILE_BUILD) || defined(AZ_DEBUG_BUILD)
                Execution::InitializeFromLuaStackFunctions(const_cast<Grammar::DebugSymbolMap&>(runtimeData.m_debugMap));
#endif
                AZ_WarningOnce("ScriptCanvas", !runtimeData.m_areStaticsInitialized, "ScriptCanvas runtime data already initalized");

                if (runtimeData.RequiresStaticInitialization())
                {
                    AZ::ScriptLoadResult result{};
                    AZ::ScriptSystemRequestBus::BroadcastResult(result, &AZ::ScriptSystemRequests::LoadAndGetNativeContext, runtimeData.m_script, AZ::k_scriptLoadBinary, AZ::ScriptContextIds::DefaultScriptContextId);
                    AZ_Assert(result.status == AZ::ScriptLoadResult::Status::Initial, "ExecutionStateInterpreted script asset was valid but failed to load.");
                    AZ_Assert(result.lua, "Must have a default script context and a lua_State");
                    AZ_Assert(lua_istable(result.lua, -1), "No run-time execution was available for this script");

                    auto lua = result.lua;
                    // Lua: table
                    lua_getfield(lua, -1, Grammar::k_InitializeStaticsName);
                    // Lua: table, ?
                    if (lua_isfunction(lua, -1))
                    {
                        // Lua: table, function
                        lua_pushvalue(lua, -2);
                        // Lua: table, function, table
                        for (auto& clonerSource : runtimeData.m_cloneSources)
                        {
                            lua_pushlightuserdata(lua, const_cast<void*>(reinterpret_cast<const void*>(&clonerSource)));
                        }
                        // Lua: table, function, table, cloners...
                        AZ::Internal::LuaSafeCall(lua, aznumeric_caster(runtimeData.m_cloneSources.size() + 1), 0);
                        // Lua: table
                        lua_pop(lua, 1);
                    }
                    else
                    {
                        // Lua: table, ?
                        lua_pop(lua, 2);
                    }
                }
            }
        }

        int InitializeNodeableOutKeys(lua_State* lua)
        {
            using namespace ExecutionInterpretedAPICpp;
            // Lua: usernodeable, keyCount
            [[maybe_unused]] const int argsCount = lua_gettop(lua);
            AZ_Assert(argsCount == 2, "InitializeNodeableOutKeys: Error in compiled Lua file, not enough arguments");
            AZ_Assert(lua_isuserdata(lua, 1), "InitializeNodeableOutKeys: Error in compiled lua file, 1st argument to SetExecutionOut is not userdata (Nodeable)");
            Nodeable* nodeable = AZ::ScriptValue<Nodeable*>::StackRead(lua, 1);
            AZ_Assert(lua_isnumber(lua, 2), "InitializeNodeableOutKeys: Error in compiled lua file, 2nd argument was not an integer");
            const size_t keyCount = aznumeric_caster(lua_tointeger(lua, 2));
            nodeable->InitializeExecutionOuts(keyCount);
            return 0;
        }

        int InterpretedSafeCall(lua_State* lua, int argCount, int returnValueCount)
        {
            const int handlerIndex = lua_gettop(lua) - argCount;
            lua_pushcfunction(lua, &ExecutionInterpretedAPICpp::ErrorHandler);
            lua_insert(lua, handlerIndex);
            int result = lua_pcall(lua, argCount, returnValueCount, handlerIndex);
            lua_remove(lua, handlerIndex);
            return result;
        }

        int SetExecutionOut(lua_State* lua)
        {
            // \note Return values could become necessary.
            // \see LY-99750

            AZ_Assert(lua_isuserdata(lua, -3), "Error in compiled lua file, 1st argument to SetExecutionOut is not userdata (Nodeable)");
            AZ_Assert(lua_isnumber(lua, -2), "Error in compiled lua file, 2nd argument to SetExecutionOut is not a number");
            AZ_Assert(lua_isfunction(lua, -1), "Error in compiled lua file, 3rd argument to SetExecutionOut is not a function (lambda need to get around atypically routed arguments)");
            Nodeable* nodeable = AZ::ScriptValue<Nodeable*>::StackRead(lua, -3);
            AZ_Assert(nodeable, "Failed to read nodeable");
            size_t index = aznumeric_caster(lua_tointeger(lua, -2));
            // Lua: nodeable, index, lambda

            lua_pushvalue(lua, -1);
            // Lua: nodeable, index, lambda, lambda

            nodeable->SetExecutionOut(index, OutInterpreted(lua));
            // Lua: nodeable, index, lambda

            // \todo clear these immediately after they are not needed with an explicit call written by the translator
            return 0;
        }

        int SetExecutionOutResult(lua_State* lua)
        {
            // \note Return values could become necessary.
            // \see LY-99750

            AZ_Assert(lua_isuserdata(lua, -3), "Error in compiled lua file, 1st argument to SetExecutionOutResult is not userdata (Nodeable)");
            AZ_Assert(lua_isnumber(lua, -2), "Error in compiled lua file, 2nd argument to SetExecutionOutResult is not a number");
            AZ_Assert(lua_isfunction(lua, -1), "Error in compiled lua file, 3rd argument to SetExecutionOutResult is not a function (lambda need to get around atypically routed arguments)");
            Nodeable* nodeable = AZ::ScriptValue<Nodeable*>::StackRead(lua, -3); // this won't be a BCO, because BCOs won't be necessary in the interpreted mode...most likely
            AZ_Assert(nodeable, "Failed to read nodeable");
            size_t index = aznumeric_caster(lua_tointeger(lua, -2));
            // Lua: nodeable, index, lambda

            lua_pushvalue(lua, -1);
            // Lua: nodeable, index, lambda, lambda

            nodeable->SetExecutionOut(index, OutInterpretedResult(lua));
            // Lua: nodeable, index, lambda

            // \todo clear these immediately after they are not needed with an explicit call written by the translator
            return 0;
        }

        int SetExecutionOutUserSubgraph(lua_State* lua)
        {
            AZ_Assert(lua_isuserdata(lua, -3), "Error in compiled lua file, 1st argument to SetExecutionOutUserSubgraph is not userdata (Nodeable)");
            AZ_Assert(lua_isnumber(lua, -2), "Error in compiled lua file, 2nd argument to SetExecutionOutUserSubgraph is not a number");
            AZ_Assert(lua_isfunction(lua, -1), "Error in compiled lua file, 3rd argument to SetExecutionOutUserSubgraph is not a function (lambda need to get around atypically routed arguments)");
            Nodeable* nodeable = AZ::ScriptValue<Nodeable*>::StackRead(lua, -3);
            AZ_Assert(nodeable, "Failed to read nodeable");
            size_t index = aznumeric_caster(lua_tointeger(lua, -2));
            // Lua: nodeable, index, lambda

            lua_pushvalue(lua, -1);
            // Lua: nodeable, index, lambda, lambda

            nodeable->SetExecutionOut(index, OutInterpretedUserSubgraph(lua));
            // Lua: nodeable, index, lambda

            // \todo clear these immediately after they are not needed with an explicit call written by the translator
            return 0;
        }

        //////////////////////////////////////////////////////////////////////////
        // \todo ScriptCanvas will probably need its own version of all of these functions
        //////////////////////////////////////////////////////////////////////////
        void StackPush(lua_State* lua, AZ::BehaviorContext* context, AZ::BehaviorValueParameter& argument)
        {
            using namespace ExecutionInterpretedAPICpp;

            if (auto valuePtr = GetAs<const char*>(argument))
            {
                auto realValue = reinterpret_cast<const char*>(valuePtr);
                lua_pushstring(lua, realValue);
            }
            else if (auto valuePtr2 = GetAs<AZStd::string>(argument))
            {
                lua_pushlstring(lua, valuePtr2->data(), valuePtr2->size());
            }
            else if (auto valuePtr3 = GetAs<AZStd::string_view>(argument))
            {
                lua_pushlstring(lua, valuePtr3->data(), valuePtr3->size());
            }
            else
            {
                // \todo determine whether or not to adjust this for return results
                AZ::StackPush(lua, context, argument);
            }
        }

        bool StackRead(lua_State* lua, AZ::BehaviorContext* context, int index, AZ::BehaviorValueParameter& param, AZ::StackVariableAllocator* allocator)
        {
            return AZ::StackRead(lua, index, context, param, allocator);
        }

        void InterpretedUnloadData(RuntimeData& runtimeData)
        {
            AZ::ScriptSystemRequestBus::Broadcast(&AZ::ScriptSystemRequests::ClearAssetReferences, runtimeData.m_script.GetId());
        }

        struct DependencyConstructionPack
        {
            ExecutionStateInterpreted* executionState;
            AZStd::vector<RuntimeDataOverrides>* dependencies;
            const size_t dependenciesIndex;
            RuntimeDataOverrides& runtimeOverrides;
        };

        DependencyConstructionPack UnpackDependencyConstructionArgsSanitize(lua_State* lua)
        {
            auto executionState = AZ::ScriptValue<ExecutionStateInterpreted*>::StackRead(lua, 1);
            AZ_Assert(executionState, "Error in compiled lua file, 1st argument to UnpackDependencyArgs is not an ExecutionStateInterpreted");
            AZ_Assert(lua_islightuserdata(lua, 2), "Error in compiled lua file, 2nd argument to UnpackDependencyArgs is not userdata (AZStd::vector<AZ::Data::Asset<RuntimeAsset>>*), but a :%s", lua_typename(lua, 2));
            auto dependentOverrides = reinterpret_cast<AZStd::vector<RuntimeDataOverrides>*>(lua_touserdata(lua, 2));
            AZ_Assert(lua_isinteger(lua, 3), "Error in compiled Lua file, 3rd argument to UnpackDependencyArgs is not a number");
            const size_t dependencyIndex = aznumeric_caster(lua_tointeger(lua, 3));
            return DependencyConstructionPack{ executionState, dependentOverrides, dependencyIndex, (*dependentOverrides)[dependencyIndex] };
        }

        int Unpack(lua_State* lua, DependencyConstructionPack& args)
        {
            ActivationInputArray storage;
            ActivationData data(args.runtimeOverrides, storage);
            ActivationInputRange range = Execution::Context::CreateActivateInputRange(data, args.executionState->GetEntityId());
            PushActivationArgs(lua, range.inputs, range.totalCount);
            return static_cast<int>(range.totalCount);
        }

        int UnpackDependencyConstructionArgs(lua_State* lua)
        {
            // Lua: executionState, dependent overrides, index into dependent overrides
            DependencyConstructionPack pack = UnpackDependencyConstructionArgsSanitize(lua);
            lua_pushlightuserdata(lua, const_cast<void*>(reinterpret_cast<const void*>(&pack.runtimeOverrides.m_dependencies)));
            return 1 + Unpack(lua, pack);
        }

        int UnpackDependencyConstructionArgsLeaf(lua_State* lua)
        {
            // Lua: executionState, dependentAssets, dependentAssetsIndex
            DependencyConstructionPack constructionArgs = UnpackDependencyConstructionArgsSanitize(lua);
            return Unpack(lua, constructionArgs);
        }
    }
}
