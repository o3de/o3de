/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ExecutionStateInterpretedPerActivation.h"

#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Script/lua/lua.h>
#include <ScriptCanvas/Grammar/PrimitivesDeclarations.h>

#include "Execution/Interpreted/ExecutionInterpretedAPI.h"
#include "Execution/RuntimeComponent.h"

namespace ScriptCanvas
{
    ExecutionStateInterpretedPerActivation::ExecutionStateInterpretedPerActivation(const ExecutionStateConfig& config)
        : ExecutionStateInterpreted(config)
    {}

    ExecutionStateInterpretedPerActivation::~ExecutionStateInterpretedPerActivation()
    {
        if (m_deactivationRequired)
        {
            StopExecution();
            ReleaseExecutionStateUnchecked();
        }
        else
        {
            ReleaseExecutionState();
        }
    }

    void ExecutionStateInterpretedPerActivation::Execute()
    {}

    void ExecutionStateInterpretedPerActivation::Initialize()
    {
        auto lua = LoadLuaScript();
        // Lua: graph_VM
        lua_getfield(lua, -1, "new");
        // Lua: graph_VM, graph_VM['new']
        AZ::Internal::LuaClassToStack(lua, this, azrtti_typeid<ExecutionStateInterpretedPerActivation>(), AZ::ObjectToLua::ByReference, AZ::AcquisitionOnPush::None);
        // Lua: graph_VM, graph_VM['new'], userdata<ExecutionState>
        Execution::ActivationInputArray storage;
        Execution::ActivationData data(m_component->GetRuntimeDataOverrides(), storage);
        Execution::ActivationInputRange range = Execution::Context::CreateActivateInputRange(data, m_component->GetEntityId());

        if (range.requiresDependencyConstructionParameters)
        {
            lua_pushlightuserdata(lua, const_cast<void*>(reinterpret_cast<const void*>(&data.variableOverrides.m_dependencies)));
            // Lua: graph_VM, graph_VM['new'], userdata<ExecutionState>, runtimeDataOverrides
            Execution::PushActivationArgs(lua, range.inputs, range.totalCount);
            // Lua: graph_VM, graph_VM['new'], userdata<ExecutionState>, runtimeDataOverrides, args...
            AZ::Internal::LuaSafeCall(lua, aznumeric_caster(2 + range.totalCount), 1);
        }
        else
        {
            Execution::PushActivationArgs(lua, range.inputs, range.totalCount);
            // Lua: graph_VM, graph_VM['new'], userdata<ExecutionState>, args...
            AZ::Internal::LuaSafeCall(lua, aznumeric_caster(1 + range.totalCount), 1);
        }

        // Lua: graph_VM, instance
        ReferenceExecutionState();
        // Lua: graph_VM,
        lua_pop(lua, 1);
        // Lua:
        m_deactivationRequired = true;
    }

    void ExecutionStateInterpretedPerActivation::StopExecution()
    {
        const auto registryIndex = GetLuaRegistryIndex();
        AZ_Assert(registryIndex != LUA_NOREF, "ExecutionStateInterpretedPerActivation::StopExecution called but Initialize is was never called");
        auto& lua = m_luaState;
        // Lua:
        lua_rawgeti(lua, LUA_REGISTRYINDEX, registryIndex);
        // Lua: instance
        AZ::Internal::azlua_getfield(lua, -1, Grammar::k_DeactivateName);
        // Lua: instance, instance['Deactivate']
        AZ::Internal::azlua_pushvalue(lua, -2);
        // Lua: instance, instance['Deactivate'], instance
        AZ::Internal::LuaSafeCall(lua, 1, 0);
        // Lua: instance
        AZ::Internal::azlua_pop(lua, 1);
        // Lua:
        m_deactivationRequired = false;
    }

    void ExecutionStateInterpretedPerActivation::Reflect(AZ::ReflectContext* reflectContext)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflectContext))
        {
            behaviorContext->Class<ExecutionStateInterpretedPerActivation>()
                ;
        }
    }

    ExecutionStateInterpretedPerActivationOnGraphStart::ExecutionStateInterpretedPerActivationOnGraphStart(const ExecutionStateConfig& config)
        : ExecutionStateInterpretedPerActivation(config)
    {}

    void ExecutionStateInterpretedPerActivationOnGraphStart::Execute()
    {
        const auto registryIndex = GetLuaRegistryIndex();
        AZ_Assert(registryIndex != LUA_NOREF, "ExecutionStateInterpretedPerActivationOnGraphStart::Execute called but Initialize is was never called");
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

    void ExecutionStateInterpretedPerActivationOnGraphStart::Reflect(AZ::ReflectContext* reflectContext)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflectContext))
        {
            behaviorContext->Class<ExecutionStateInterpretedPerActivationOnGraphStart>()
                ;
        }
    }
}
