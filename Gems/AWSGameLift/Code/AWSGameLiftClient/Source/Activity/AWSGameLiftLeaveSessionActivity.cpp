/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzFramework/Session/ISessionHandlingRequests.h>

#include <Activity/AWSGameLiftLeaveSessionActivity.h>

namespace AWSGameLift
{
    namespace LeaveSessionActivity
    {
        void LeaveSession()
        {
            auto clientRequestHandler = AZ::Interface<AzFramework::ISessionHandlingClientRequests>::Get();
            if (clientRequestHandler)
            {
                AZ_TracePrintf(AWSGameLiftLeaveSessionActivityName, "Requesting player to leave the current session ...");
                clientRequestHandler->RequestPlayerLeaveSession();
                AZ_TracePrintf(AWSGameLiftLeaveSessionActivityName, "Started disconnect process, and player clean up is in process.");
            }
            else
            {
                AZ_Error(AWSGameLiftLeaveSessionActivityName, false, AWSGameLiftLeaveSessionMissingRequestHandlerErrorMessage);
            }
        }

    } // namespace LeaveSessionActivity
} // namespace AWSGameLift
