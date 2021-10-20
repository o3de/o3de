/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.AcceptMatch
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>

#include <Activity/AWSGameLiftAcceptMatchActivity.h>
#include <AWSGameLiftSessionConstants.h>

#include <aws/core/utils/Outcome.h>
#include <aws/gamelift/model/AcceptMatchRequest.h>

namespace AWSGameLift
{
    namespace AcceptMatchActivity
    {
        Aws::GameLift::Model::AcceptMatchRequest BuildAWSGameLiftAcceptMatchRequest(
            const AWSGameLiftAcceptMatchRequest& acceptMatchRequest)
        {
            Aws::GameLift::Model::AcceptMatchRequest request;
            request.SetAcceptanceType(acceptMatchRequest.m_acceptMatch ?
                Aws::GameLift::Model::AcceptanceType::ACCEPT : Aws::GameLift::Model::AcceptanceType::REJECT);

            Aws::Vector<Aws::String> playerIds;
            for (const AZStd::string& playerId : acceptMatchRequest.m_playerIds)
            {
                playerIds.emplace_back(playerId.c_str());
            }
            request.SetPlayerIds(playerIds);

            if (!acceptMatchRequest.m_ticketId.empty())
            {
                request.SetTicketId(acceptMatchRequest.m_ticketId.c_str());
            }

            AZ_TracePrintf(AWSGameLiftAcceptMatchActivityName, "Built AcceptMatchRequest with TicketId=%s", request.GetTicketId().c_str());

            return request;
        }

        void AcceptMatch(const Aws::GameLift::GameLiftClient& gameliftClient,
            const AWSGameLiftAcceptMatchRequest& AcceptMatchRequest)
        {
            AZ_TracePrintf(AWSGameLiftAcceptMatchActivityName, "Requesting AcceptMatch against Amazon GameLift service ...");

            Aws::GameLift::Model::AcceptMatchRequest request = BuildAWSGameLiftAcceptMatchRequest(AcceptMatchRequest);
            auto AcceptMatchOutcome = gameliftClient.AcceptMatch(request);

            if (AcceptMatchOutcome.IsSuccess())
            {
                AZ_TracePrintf(AWSGameLiftAcceptMatchActivityName, "AcceptMatch request against Amazon GameLift service is complete");
            }
            else
            {
                AZ_Error(AWSGameLiftAcceptMatchActivityName, false, AWSGameLiftErrorMessageTemplate,
                    AcceptMatchOutcome.GetError().GetExceptionName().c_str(), AcceptMatchOutcome.GetError().GetMessage().c_str());
            }
        }

        bool ValidateAcceptMatchRequest(const AzFramework::AcceptMatchRequest& AcceptMatchRequest)
        {
            auto gameliftAcceptMatchRequest = azrtti_cast<const AWSGameLiftAcceptMatchRequest*>(&AcceptMatchRequest);
            bool isValid = gameliftAcceptMatchRequest &&
                (gameliftAcceptMatchRequest->m_playerIds.size() > 0) &&
                (!gameliftAcceptMatchRequest->m_ticketId.empty());

            AZ_Error(AWSGameLiftAcceptMatchActivityName, isValid, AWSGameLiftAcceptMatchRequestInvalidErrorMessage);

            return isValid;
        }
    } // namespace AcceptMatchActivity
} // namespace AWSGameLift
