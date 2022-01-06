/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

#include <AWSGameLiftSessionConstants.h>
#include <Activity/AWSGameLiftActivityUtils.h>
#include <Activity/AWSGameLiftCreateSessionOnQueueActivity.h>
#include <Request/IAWSGameLiftInternalRequests.h>

#include <aws/core/utils/Outcome.h>
#include <aws/gamelift/model/StartGameSessionPlacementRequest.h>

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
            AZStd::string propertiesOutput = "";
            Aws::Vector<Aws::GameLift::Model::GameProperty> properties;
            AWSGameLiftActivityUtils::GetGameProperties(createSessionOnQueueRequest.m_sessionProperties, properties, propertiesOutput);
            if (!properties.empty())
            {
                request.SetGameProperties(properties);
            }

            // Required attributes
            request.SetGameSessionQueueName(createSessionOnQueueRequest.m_queueName.c_str());
            request.SetMaximumPlayerSessionCount(static_cast<int>(createSessionOnQueueRequest.m_maxPlayer));
            request.SetPlacementId(createSessionOnQueueRequest.m_placementId.c_str());

            AZ_TracePrintf(AWSGameLiftCreateSessionOnQueueActivityName,
                "Built StartGameSessionPlacementRequest with GameSessionName=%s, GameProperties=%s, GameSessionQueueName=%s, MaximumPlayerSessionCount=%d and PlacementId=%s",
                request.GetGameSessionName().c_str(),
                AZStd::string::format("[%s]", propertiesOutput.c_str()).c_str(),
                request.GetGameSessionQueueName().c_str(),
                request.GetMaximumPlayerSessionCount(),
                request.GetPlacementId().c_str());

            return request;
        }

        AZStd::string CreateSessionOnQueue(const AWSGameLiftCreateSessionOnQueueRequest& createSessionOnQueueRequest)
        {
            AZStd::string result = "";

            auto gameliftClient = AZ::Interface<IAWSGameLiftInternalRequests>::Get()->GetGameLiftClient();
            if (!gameliftClient)
            {
                AZ_Error(AWSGameLiftCreateSessionOnQueueActivityName, false, AWSGameLiftClientMissingErrorMessage);
                return result;
            }

            AZ_TracePrintf(AWSGameLiftCreateSessionOnQueueActivityName,
                "Requesting StartGameSessionPlacement against Amazon GameLift service ...");

            Aws::GameLift::Model::StartGameSessionPlacementRequest request =
                BuildAWSGameLiftStartGameSessionPlacementRequest(createSessionOnQueueRequest);
            auto createSessionOnQueueOutcome = gameliftClient->StartGameSessionPlacement(request);
            AZ_TracePrintf(AWSGameLiftCreateSessionOnQueueActivityName,
                "StartGameSessionPlacement request against Amazon GameLift service is complete.");

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

            return gameliftCreateSessionOnQueueRequest &&
                !gameliftCreateSessionOnQueueRequest->m_queueName.empty() && !gameliftCreateSessionOnQueueRequest->m_placementId.empty();
        }
    } // namespace CreateSessionOnQueueActivity
} // namespace AWSGameLift
