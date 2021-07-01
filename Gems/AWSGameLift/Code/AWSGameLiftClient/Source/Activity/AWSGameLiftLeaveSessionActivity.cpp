/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Activity/AWSGameLiftLeaveSessionActivity.h>

#include <AzCore/Interface/Interface.h>
#include <AzFramework/Session/ISessionHandlingRequests.h>

namespace AWSGameLift
{
    namespace LeaveSessionActivity
    {
        void LeaveSession()
        {
            auto clientRequestHandler = AZ::Interface<AzFramework::ISessionHandlingClientRequests>::Get();
            if (clientRequestHandler)
            {
                AZ_TracePrintf(AWSGameLiftLeaveSessionActivityName, "Requesting to leave the current session...");

                clientRequestHandler->RequestPlayerLeaveSession();
            }
            else
            {
                AZ_Error(AWSGameLiftLeaveSessionActivityName, false, AWSGameLiftLeaveSessionMissingRequestHandlerErrorMessage);
            }
        }

    } // namespace LeaveSessionActivity
} // namespace AWSGameLift
