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

#include "API.h"

namespace ScriptCanvas
{
    namespace Debugger
    {
        const AzFramework::MsgSlotId k_serviceNotificationsMsgSlotId = AZ_CRC("ScriptCanvasDebugServiceNotifications", 0xfd4305e9);
        const AzFramework::MsgSlotId k_clientRequestsMsgSlotId = AZ_CRC("ScriptCanvasDebugClientRequests", 0x435d9a15);

        AZ::Outcome<void, AZStd::string> IsTargetConnectable(const AzFramework::TargetInfo& target)
        {
            const auto statusFlags = target.GetStatusFlags();
            
            if (!target.IsValid())
            {
                return AZ::Failure(AZStd::string("The target is invalid, it has never been seen"));
            }
            else if (!(statusFlags & AzFramework::TF_ONLINE))
            {
                return AZ::Failure(AZStd::string("The target is not online"));
            }
            else if (!(statusFlags & AzFramework::TF_DEBUGGABLE))
            {
                return AZ::Failure(AZStd::string("The target is not debuggable"));
            }
            return AZ::Success();
        }
    }
}