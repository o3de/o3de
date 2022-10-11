/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Script/lua/lua.h>
#include <ScriptCanvas/Execution/RuntimeComponent.h>
#include <ScriptCanvas/Execution/ExecutionState.h>
#include <ScriptCanvas/Execution/Interpreted/ExecutionStateInterpretedAPI.h>
#include "ExecutionInterpretedDebugAPI.h"
#include "ExecutionInterpretedEBusAPI.h"
#include "ExecutionInterpretedOut.h"

#include "ExecutionInterpretedComponentAPI.h"

namespace ScriptCanvas
{
    namespace Execution
    {
        int GetSelfEntityId(lua_State* lua)
        {
            // Lua: executionState
            auto executionState = ExecutionStateRead(lua, 1);
            auto reference = AZStd::any_cast<const RuntimeComponentUserData>(&executionState->GetUserData());
            AZ::ScriptValue<AZ::EntityId>::StackPush(lua, reference->entity);
            // Lua: executionState, entityId
            return 1;
        }

        void RegisterComponentAPI(lua_State* lua)
        {
            using namespace ScriptCanvas::Grammar;

            lua_register(lua, k_GetSelfEntityId, &GetSelfEntityId);
        }
    }
} 
