/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ExecutionInterpretedCloningAPI.h"

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
        int CloneSourceObject(lua_State* lua)
        {
            AZ_Assert(lua_islightuserdata(lua, -1), "Error in compiled lua file, 1st argument to CloneSourceFunction is not userdata (CloneSource), but a :%s", lua_typename(lua, -1));
            const CloneSource* cloneSource = reinterpret_cast<const CloneSource*>(lua_touserdata(lua, -1));
            AZ_Assert(cloneSource, "Failed to read CloneSource");
            CloneSource::Result result = cloneSource->Clone();
            AZ_Assert(result.object, "CloneSource::Clone failed to create an object.");
            AZ_Assert(!result.typeId.IsNull(), "CloneSource::Clone failed to return the type of the object.");
            AZ::Internal::LuaClassToStack(lua, result.object, result.typeId, AZ::ObjectToLua::ByReference, AZ::AcquisitionOnPush::ScriptAcquire);
            return 1;
        }

        void RegisterCloningAPI(lua_State* lua)
        {
            using namespace ScriptCanvas::Grammar;

            lua_register(lua, k_CloneSourceFunctionName, &CloneSourceObject);
        }

    } 

} 
