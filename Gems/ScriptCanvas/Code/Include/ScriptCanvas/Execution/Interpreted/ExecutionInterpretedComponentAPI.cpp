/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ExecutionInterpretedComponentAPI.h"

#include <AzCore/Script/lua/lua.h>
#include <ScriptCanvas/Execution/RuntimeComponent.h>
#include <ScriptCanvas/Execution/ExecutionState.h>

#include "ExecutionInterpretedDebugAPI.h"
#include "ExecutionInterpretedEBusAPI.h"
#include "ExecutionInterpretedOut.h"

namespace ScriptCanvas
{
    namespace Execution
    {
        // \todo do a sanity check at compile(?) usage time for any graph NOT being used properly (that is, not by a component)
        int GetSelfEntityId(lua_State* lua)
        {
            // Lua: executionState
            auto executionState = AZ::ScriptValue<ExecutionState*>::StackRead(lua, 1);
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
