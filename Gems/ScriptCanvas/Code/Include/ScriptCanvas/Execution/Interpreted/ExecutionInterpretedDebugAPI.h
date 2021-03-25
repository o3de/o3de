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

#pragma once

#include <AzCore/std/allocator.h>
#include <AzCore/std/allocator_static.h>
#include <AzCore/std/typetraits/internal/type_sequence_traits.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <ScriptCanvas/Core/NodeableOut.h>

struct lua_State;

namespace AZ
{
    class SerializeContext;
}

namespace ScriptCanvas
{
    namespace Execution
    {        
        // Lua: executionState
        int DebugIsTraced(lua_State* lua);

        // Lua: executionState, string
        int DebugRuntimeError(lua_State* lua);

        // Lua: executionState, executionKey, data...
        int DebugSignalIn(lua_State* lua);

        // Lua: executionState, AssetID, executionKey, data...
        int DebugSignalInSubgraph(lua_State* lua);

        // Lua: executionState, executionKey, data...
        int DebugSignalOut(lua_State* lua);

        // Lua: executionState, AssetID, executionKey, data...
        int DebugSignalOutSubgraph(lua_State* lua);

        // Lua: executionState, executionKey, data...
        int DebugSignalReturn(lua_State* /*lua*/);

        // Lua: executionState, AssetID, executionKey, data...
        int DebugSignalReturnSubgraph(lua_State* /*lua*/);

        // Lua: executionState, executionKey, datumKey, value
        int DebugVariableChange(lua_State* lua);

        // Lua: executionState, AssetID, executionKey, datumKey, value
        int DebugVariableChangeSubgraph(lua_State* lua);

        void RegisterDebugAPI(lua_State* lua);

    } 

} 
