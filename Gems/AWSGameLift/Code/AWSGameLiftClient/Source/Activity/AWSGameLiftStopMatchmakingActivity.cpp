/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.StopMatchmaking
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

#include <Activity/AWSGameLiftStopMatchmakingActivity.h>
#include <AWSGameLiftSessionConstants.h>
#include <Request/IAWSGameLiftInternalRequests.h>

#include <aws/core/utils/Outcome.h>
#include <aws/gamelift/model/StopMatchmakingRequest.h>

namespace AWSGameLift
{
    namespace StopMatchmakingActivity
    {
        Aws::GameLift::Model::StopMatchmakingRequest BuildAWSGameLiftStopMatchmakingRequest(
            const AWSGameLiftStopMatchmakingRequest& stopMatchmakingRequest)
        {
            Aws::GameLift::Model::StopMatchmakingRequest request;
            if (!stopMatchmakingRequest.m_ticketId.empty())
            {
                request.SetTicketId(stopMatchmakingRequest.m_ticketId.c_str());
            }

            AZ_TracePrintf(AWSGameLiftStopMatchmakingActivityName, "Built StopMatchmakingRequest with TicketId=%s", request.GetTicketId().c_str());

            return request;
        }

        void StopMatchmaking(const AWSGameLiftStopMatchmakingRequest& stopMatchmakingRequest)
        {
            auto gameliftClient = AZ::Interface<IAWSGameLiftInternalRequests>::Get()->GetGameLiftClient();
            if (!gameliftClient)
            {
                AZ_Error(AWSGameLiftStopMatchmakingActivityName, false, AWSGameLiftClientMissingErrorMessage);
                return;
            }

            AZ_TracePrintf(AWSGameLiftStopMatchmakingActivityName, "Requesting StopMatchmaking against Amazon GameLift service ...");

            Aws::GameLift::Model::StopMatchmakingRequest request = BuildAWSGameLiftStopMatchmakingRequest(stopMatchmakingRequest);
            auto stopMatchmakingOutcome = gameliftClient->StopMatchmaking(request);

            if (stopMatchmakingOutcome.IsSuccess())
            {
                AZ_TracePrintf(AWSGameLiftStopMatchmakingActivityName, "StopMatchmaking request against Amazon GameLift service is complete");
            }
            else
            {
                AZ_Error(AWSGameLiftStopMatchmakingActivityName, false, AWSGameLiftErrorMessageTemplate,
                    stopMatchmakingOutcome.GetError().GetExceptionName().c_str(), stopMatchmakingOutcome.GetError().GetMessage().c_str());
            }
        }

        bool ValidateStopMatchmakingRequest(const AzFramework::StopMatchmakingRequest& StopMatchmakingRequest)
        {
            auto gameliftStopMatchmakingRequest = azrtti_cast<const AWSGameLiftStopMatchmakingRequest*>(&StopMatchmakingRequest);
            bool isValid = gameliftStopMatchmakingRequest && (!gameliftStopMatchmakingRequest->m_ticketId.empty());

            AZ_Error(AWSGameLiftStopMatchmakingActivityName, isValid, AWSGameLiftStopMatchmakingRequestInvalidErrorMessage);

            return isValid;
        }
    } // namespace StopMatchmakingActivity
} // namespace AWSGameLift
