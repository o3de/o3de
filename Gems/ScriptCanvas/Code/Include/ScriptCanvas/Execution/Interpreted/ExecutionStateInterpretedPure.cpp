/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ExecutionStateInterpretedPure.h"

#include <AzCore/Script/ScriptContext.h>
#include <ScriptCanvas/Grammar/PrimitivesDeclarations.h>
#include <ScriptCanvas/Execution/Interpreted/ExecutionStateInterpretedAPI.h>

#include "Execution/Interpreted/ExecutionInterpretedAPI.h"
#include "Execution/RuntimeComponent.h"

namespace ScriptCanvas
{
    ExecutionStateInterpretedPure::ExecutionStateInterpretedPure(ExecutionStateConfig& config)
        : ExecutionStateInterpreted(config)
    {}

    void ExecutionStateInterpretedPure::Execute()
    {}

    void ExecutionStateInterpretedPure::Initialize()
    {}

    bool ExecutionStateInterpretedPure::IsPure() const
    {
        return true;
    }

    void ExecutionStateInterpretedPure::StopExecution()
    {}

    ExecutionStateInterpretedPureOnGraphStart::ExecutionStateInterpretedPureOnGraphStart(ExecutionStateConfig& config)
        : ExecutionStateInterpretedPure(config)
    {}

    // #functions2 dependency - ctor - args adjust for (pure only) on graph start args
    void ExecutionStateInterpretedPureOnGraphStart::Execute()
    {
        // execute the script in a single call
        auto lua = LoadLuaScript();
        // Lua: graph_VM
        AZ::Internal::azlua_getfield(lua, -1, Grammar::k_OnGraphStartFunctionName);
        // Lua: graph_VM, graph_VM['k_OnGraphStartFunctionName']
        Execution::ExecutionStatePush(lua, this);
        // Lua: graph_VM, graph_VM['k_OnGraphStartFunctionName'], executionState
        Execution::ActivationInputArray storage;
        Execution::ActivationData data(GetRuntimeDataOverrides(), storage);
        Execution::ActivationInputRange range = Execution::Context::CreateActivateInputRange(data);
        Execution::PushActivationArgs(lua, range.inputs, range.totalCount);
        // Lua: graph_VM, graph_VM['k_OnGraphStartFunctionName'], executionState, args...
        const int result = Execution::InterpretedSafeCall(lua, aznumeric_caster(1 + range.totalCount), 0);
        // Lua: graph_VM, ?
        if (result == LUA_OK)
        {
            // Lua: graph_VM
            lua_pop(lua, 1);
        }
        else
        {
            // Lua: graph_VM, error
            lua_pop(lua, 2);
        }
    }
}
