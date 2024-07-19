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
        const AZ::u64 k_serviceNotificationsMsgSlotId = AZ_CRC_CE("ScriptCanvasDebugServiceNotifications");
        const AZ::u64 k_clientRequestsMsgSlotId = AZ_CRC_CE("ScriptCanvasDebugClientRequests");

        AZ::Outcome<void, AZStd::string> IsTargetConnectable(const AzFramework::RemoteToolsEndpointInfo& target)
        {
            if (!target.IsValid())
            {
                return AZ::Failure(AZStd::string("The target is invalid, it has never been seen"));
            }
            return AZ::Success();
        }
    }
}
