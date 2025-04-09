/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Material/LuaScriptUtilities.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ
{
    namespace RPI
    {
        void LuaScriptUtilities::Reflect(AZ::BehaviorContext* behaviorContext)
        {
            behaviorContext->Method("Error", &Error);
            behaviorContext->Method("Warning", &Warning);
            behaviorContext->Method("Print", &Print);
        }

        void LuaScriptUtilities::Error([[maybe_unused]] const AZStd::string& message)
        {
            AZ_Error(DebugName, false, "Lua script: %s", message.c_str());
        }

        void LuaScriptUtilities::Warning([[maybe_unused]] const AZStd::string& message)
        {
            AZ_Warning(DebugName, false, "Lua script: %s", message.c_str());
        }

        void LuaScriptUtilities::Print([[maybe_unused]] const AZStd::string& message)
        {
            AZ_TracePrintf(DebugName, "Lua script: %s\n", message.c_str());
        }

    } // namespace RPI
} // namespace AZ
