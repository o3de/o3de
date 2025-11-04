/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Script/lua/lua.h>
#include <ScriptCanvas/Execution/Interpreted/ExecutionInterpretedAPI.h>
#include <ScriptCanvas/Execution/Interpreted/ExecutionStateInterpretedAPI.h>
#include <ScriptCanvas/Execution/Interpreted/ExecutionStateInterpretedPerActivation.h>
#include <ScriptCanvas/Grammar/PrimitivesDeclarations.h>

namespace ScriptCanvas
{
    ExecutionStateInterpretedPerActivation::ExecutionStateInterpretedPerActivation(ExecutionStateConfig& config)
        : ExecutionStateInterpreted(config)
        , m_deactivationRequired(false)
        , m_luaRegistryIndex(LUA_NOREF)
    {}

    ExecutionStateInterpretedPerActivation::~ExecutionStateInterpretedPerActivation()
    {
        if (m_deactivationRequired)
        {
            StopExecution();
            ReleaseInterpretedInstanceUnchecked();
        }
        else
        {
            ReleaseInterpretedInstance();
        }
    }

    void ExecutionStateInterpretedPerActivation::ClearLuaRegistryIndex()
    {
        m_luaRegistryIndex = LUA_NOREF;
    }

    void ExecutionStateInterpretedPerActivation::Execute()
    {}

    int ExecutionStateInterpretedPerActivation::GetLuaRegistryIndex() const
    {
        return m_luaRegistryIndex;
    }

    void ExecutionStateInterpretedPerActivation::Initialize()
    {
        auto lua = LoadLuaScript();
        // Lua: graph_VM
        lua_getfield(lua, -1, "new");
        // Lua: graph_VM, graph_VM['new']
        Execution::ExecutionStatePush(lua, this);
        // Lua: graph_VM, graph_VM['new'], executionState
        Execution::ActivationInputArray storage;
        Execution::ActivationData data(GetRuntimeDataOverrides(), storage);
        Execution::ActivationInputRange range = Execution::Context::CreateActivateInputRange(data);

        if (range.requiresDependencyConstructionParameters)
        {
            SC_RUNTIME_CHECK_RETURN(!data.variableOverrides.m_dependencies.empty()
                , "ExecutionStateInterpretedPerActivation::Initialize dependencies are empty or null, check the processing of this asset");
            lua_pushlightuserdata(lua, const_cast<void*>(reinterpret_cast<const void*>(&data.variableOverrides.m_dependencies)));
            // Lua: graph_VM, graph_VM['new'], executionState, runtimeDataOverrides
            Execution::PushActivationArgs(lua, range.inputs, range.totalCount);
            // Lua: graph_VM, graph_VM['new'], executionState, runtimeDataOverrides, args...
            AZ::Internal::LuaSafeCall(lua, aznumeric_caster(2 + range.totalCount), 1);
        }
        else
        {
            Execution::PushActivationArgs(lua, range.inputs, range.totalCount);
            // Lua: graph_VM, graph_VM['new'], executionState, args...
            AZ::Internal::LuaSafeCall(lua, aznumeric_caster(1 + range.totalCount), 1);
        }

        // Lua: graph_VM, instance
        ReferenceInterpretedInstance();
        // Lua: graph_VM,
        lua_pop(lua, 1);
        // Lua:
        m_deactivationRequired = true;
    }

    void ExecutionStateInterpretedPerActivation::StopExecution()
    {
        if (m_deactivationRequired)
        {
            const auto registryIndex = GetLuaRegistryIndex();
            SC_RUNTIME_CHECK_RETURN(
                registryIndex != LUA_NOREF,
                "ExecutionStateInterpretedPerActivation::StopExecution called but Initialize is was never called");
            auto& lua = m_luaState;
            // Lua:
            lua_rawgeti(lua, LUA_REGISTRYINDEX, registryIndex);
            // Lua: instance
            lua_getfield(lua, -1, Grammar::k_DeactivateName);
            // Lua: instance, instance['Deactivate']
            lua_pushvalue(lua, -2);
            // Lua: instance, instance['Deactivate'], instance
            AZ::Internal::LuaSafeCall(lua, 1, 0);
            // Lua: instance
            lua_getfield(lua, -1, "Destruct");
            // Lua: instance, instance['Destruct']
            lua_pushvalue(lua, -2);
            // Lua: instance, instance['Destruct'], instance
            AZ::Internal::LuaSafeCall(lua, 1, 0);
            // Lua: instance
            AZ::Internal::azlua_pop(lua, 1);
            // Lua:
            m_deactivationRequired = false;
        }
    }

    void ExecutionStateInterpretedPerActivation::ReleaseInterpretedInstance()
    {
        if (m_luaRegistryIndex != LUA_NOREF)
        {
            ReleaseInterpretedInstanceUnchecked();
        }
    }

    void ExecutionStateInterpretedPerActivation::ReleaseInterpretedInstanceUnchecked()
    {
        luaL_unref(m_luaState, LUA_REGISTRYINDEX, m_luaRegistryIndex);
        m_luaRegistryIndex = LUA_NOREF;
    }

    void ExecutionStateInterpretedPerActivation::ReferenceInterpretedInstance()
    {
        SC_RUNTIME_CHECK(m_luaRegistryIndex == LUA_NOREF, "ExecutionStateInterpreted already in the Lua registry and risks double deletion");
        // Lua: instance
        m_luaRegistryIndex = luaL_ref(m_luaState, LUA_REGISTRYINDEX);
        SC_RUNTIME_CHECK(m_luaRegistryIndex != LUA_REFNIL, "ExecutionStateInterpreted was nil when trying to gain a reference");
        SC_RUNTIME_CHECK(m_luaRegistryIndex != LUA_NOREF, "ExecutionStateInterpreted failed to gain a reference");
    }

    ExecutionStateInterpretedPerActivationOnGraphStart::ExecutionStateInterpretedPerActivationOnGraphStart(ExecutionStateConfig& config)
        : ExecutionStateInterpretedPerActivation(config)
    {}

    void ExecutionStateInterpretedPerActivationOnGraphStart::Execute()
    {
        const auto registryIndex = GetLuaRegistryIndex();
        SC_RUNTIME_CHECK_RETURN(registryIndex != LUA_NOREF,
            "ExecutionStateInterpretedPerActivationOnGraphStart::Execute called but Initialize is was never called");
        auto& lua = m_luaState;
        // Lua:
        lua_rawgeti(lua, LUA_REGISTRYINDEX, registryIndex);
        // Lua: instance
        lua_getfield(lua, -1, Grammar::k_OnGraphStartFunctionName);
        // Lua: instance, graph_VM.k_OnGraphStartFunctionName
        lua_pushvalue(lua, -2);
        // Lua: instance, graph_VM.k_OnGraphStartFunctionName, instance
        const int result = Execution::InterpretedSafeCall(lua, 1, 0);
        // Lua: instance, ?
        if (result == LUA_OK)
        {
            // Lua: instance
            lua_pop(lua, 1);
        }
        else
        {
            // Lua: instance, error
            lua_pop(lua, 2);
        }
        // Lua:
        m_deactivationRequired = true;
    }
}
