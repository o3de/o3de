/*
 * /*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 */
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AWSGameLiftSessionConstants.h>
#include <Activity/AWSGameLiftCreateSessionOnQueueActivity.h>

namespace AWSGameLift
{
    namespace CreateSessionOnQueueActivity
    {
        Aws::GameLift::Model::StartGameSessionPlacementRequest BuildAWSGameLiftStartGameSessionPlacementRequest(
            const AWSGameLiftCreateSessionOnQueueRequest& createSessionOnQueueRequest)
        {
            Aws::GameLift::Model::StartGameSessionPlacementRequest request;
            // Optional attributes
            if (!createSessionOnQueueRequest.m_sessionName.empty())
            {
                request.SetGameSessionName(createSessionOnQueueRequest.m_sessionName.c_str());
            }
            for (auto iter = createSessionOnQueueRequest.m_sessionProperties.begin();
                 iter != createSessionOnQueueRequest.m_sessionProperties.end(); iter++)
            {
                Aws::GameLift::Model::GameProperty sessionProperty;
                sessionProperty.SetKey(iter->first.c_str());
                sessionProperty.SetValue(iter->second.c_str());
                request.AddGameProperties(sessionProperty);
            }

            // Required attributes
            request.SetGameSessionQueueName(createSessionOnQueueRequest.m_queueName.c_str());
            request.SetMaximumPlayerSessionCount(createSessionOnQueueRequest.m_maxPlayer);
            request.SetPlacementId(createSessionOnQueueRequest.m_placementId.c_str());

            return request;
        }

        AZStd::string CreateSessionOnQueue(
            const Aws::GameLift::GameLiftClient& gameliftClient,
            const AWSGameLiftCreateSessionOnQueueRequest& createSessionOnQueueRequest)
        {
            AZ_TracePrintf(AWSGameLiftCreateSessionOnQueueActivityName,
                "Requesting StartGameSessionPlacement against Amazon GameLift service ...");

            AZStd::string result = "";
            Aws::GameLift::Model::StartGameSessionPlacementRequest request =
                BuildAWSGameLiftStartGameSessionPlacementRequest(createSessionOnQueueRequest);
            auto createSessionOnQueueOutcome = gameliftClient.StartGameSessionPlacement(request);

            if (createSessionOnQueueOutcome.IsSuccess())
            {
                result = AZStd::string(createSessionOnQueueOutcome.GetResult().GetGameSessionPlacement().GetPlacementId().c_str());
            }
            else
            {
                AZ_Error(AWSGameLiftCreateSessionOnQueueActivityName, false, AWSGameLiftErrorMessageTemplate,
                    createSessionOnQueueOutcome.GetError().GetExceptionName().c_str(),
                    createSessionOnQueueOutcome.GetError().GetMessage().c_str());
            }
            return result;
        }

        bool ValidateCreateSessionOnQueueRequest(const AzFramework::CreateSessionRequest& createSessionRequest)
        {
            auto gameliftCreateSessionOnQueueRequest =
                azrtti_cast<const AWSGameLiftCreateSessionOnQueueRequest*>(&createSessionRequest);

            return gameliftCreateSessionOnQueueRequest && gameliftCreateSessionOnQueueRequest->m_maxPlayer >= 0 &&
                !gameliftCreateSessionOnQueueRequest->m_queueName.empty() && !gameliftCreateSessionOnQueueRequest->m_placementId.empty();
        }
    } // namespace CreateSessionOnQueueActivity
} // namespace AWSGameLift
