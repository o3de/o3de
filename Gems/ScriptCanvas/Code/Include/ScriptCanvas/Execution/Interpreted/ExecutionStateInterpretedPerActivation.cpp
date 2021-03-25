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
    {
        const auto registryIndex = GetLuaRegistryIndex();
        AZ_Assert(registryIndex != LUA_NOREF, "ExecutionStateInterpretedPerActivation::Activate called but Init is was never called");
        auto& lua = m_luaState;
        // Lua:
        lua_rawgeti(lua, LUA_REGISTRYINDEX, registryIndex);
        // Lua: instance
        lua_getfield(lua, -1, Grammar::k_OnGraphStartFunctionName);
        // Lua: instance, graph_VM['k_OnGraphStartFunctionName']
        if (lua_isfunction(lua, -1))
        {
            // Lua: instance, graph_VM.k_OnGraphStartFunctionName
            lua_pushvalue(lua, -2);
            // Lua: instance, graph_VM.k_OnGraphStartFunctionName, instance
            const int result = Execution::InterpretedSafeCall(lua, 1, 0);
            // Lua: instance ?
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
        }
        else
        {
            // Lua: instance, nil
            lua_pop(lua, 2);
        }
        // Lua:
        m_deactivationRequired = true;
    }

    void ExecutionStateInterpretedPerActivation::Initialize()
    {
        auto lua = LoadLuaScript();
        // Lua: graph_VM
        lua_getfield(lua, -1, "new");
        // Lua: graph_VM, graph_VM['new']
        AZ::Internal::LuaClassToStack(lua, this, azrtti_typeid<ExecutionStateInterpretedPerActivation>(), AZ::ObjectToLua::ByReference, AZ::AcquisitionOnPush::None);
        // Lua: graph_VM, graph_VM['new'], userdata<ExecutionState>
        Execution::ActivationInputArray storage;
        Execution::ActivationData data(*m_component, storage);
        Execution::ActivationInputRange range = Execution::Context::CreateActivateInputRange(data);
        Execution::PushActivationArgs(lua, range.inputs, range.totalCount);
        // Lua: graph_VM, graph_VM['new'], userdata<ExecutionState>, args...
        AZ::Internal::LuaSafeCall(lua, aznumeric_caster(1 + range.totalCount), 1);
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
        AZ_Assert(registryIndex != LUA_NOREF, "ExecutionStateInterpretedPerActivation::Deactivate called but Initialize is was never called");
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

} 
