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

#include <AzCore/Component/EntityId.h>
#include <ScriptCanvas/Core/Core.h>
#include <AzCore/EBus/EBus.h>
#include <AzFramework/TargetManagement/TargetManagementAPI.h>
#include <AzCore/Outcome/Outcome.h>

/**
 * Runtime systems that provide debug information are inherently slow. Debugging such debug systems themselves makes them even slower.
 * Keep Debug debugging entries confined here, and do not enable them in source control.
 */ 
#if defined(SCRIPT_CANVAS_DEBUG_DEBUGGER)

#define SCRIPT_CANVAS_DEBUGGER_TRACE_SERVER(message, ...) AZ_TracePrintf("ScriptCanvas Debugger Server", message, __VA_ARGS__);
#define SCRIPT_CANVAS_DEBUGGER_TRACE_CLIENT(message, ...) AZ_TracePrintf("ScriptCanvas Debugger Client", message, __VA_ARGS__)

#else

#define SCRIPT_CANVAS_DEBUGGER_TRACE_SERVER(message, ...)
#define SCRIPT_CANVAS_DEBUGGER_TRACE_CLIENT(message, ...)

#endif//defined(SCRIPT_CANVAS_DEBUG_DEBUGGER)


namespace AZ
{
    class ReflectContext;
}

namespace ScriptCanvas
{
    namespace Debugger
    {
        extern const AzFramework::MsgSlotId k_serviceNotificationsMsgSlotId;
        extern const AzFramework::MsgSlotId k_clientRequestsMsgSlotId;

        AZ::Outcome<void, AZStd::string> IsTargetConnectable(const AzFramework::TargetInfo& target);
        void ReflectArguments(AZ::ReflectContext* context);
        void ReflectNotifications(AZ::ReflectContext* context);
        void ReflectRequests(AZ::ReflectContext* context);
    }
}