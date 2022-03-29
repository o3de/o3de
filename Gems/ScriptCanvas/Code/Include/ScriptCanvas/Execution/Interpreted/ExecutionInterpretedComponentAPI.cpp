/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ExecutionInterpretedComponentAPI.h"

#include <AzCore/Math/Crc.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Script/ScriptSystemBus.h>
#include <AzCore/Script/lua/lua.h>
#include <ScriptCanvas/Core/Nodeable.h>
#include <ScriptCanvas/Core/NodeableOut.h>
#include <ScriptCanvas/Execution/ExecutionState.h>
#include <ScriptCanvas/Execution/ExecutionObjectCloning.h>
#include <ScriptCanvas/Grammar/PrimitivesDeclarations.h>
#include <ScriptCanvas/Libraries/Math/MathNodeUtilities.h>
#include <ScriptCanvas/Utils/BehaviorContextUtils.h>

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
            auto reference = AZStd::any_cast<const Reference>(&executionState->GetUserData());
            auto entityId = reference->As<AZ::EntityId>();
            AZ::ScriptValue<AZ::EntityId>::StackPush(lua, *entityId);
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
