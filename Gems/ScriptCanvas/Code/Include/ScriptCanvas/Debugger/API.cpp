/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
