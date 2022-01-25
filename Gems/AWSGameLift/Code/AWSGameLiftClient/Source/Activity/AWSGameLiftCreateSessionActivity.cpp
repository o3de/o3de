/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

#include <Activity/AWSGameLiftActivityUtils.h>
#include <Activity/AWSGameLiftCreateSessionActivity.h>
#include <AWSGameLiftSessionConstants.h>
#include <Request/IAWSGameLiftInternalRequests.h>

#include <aws/core/utils/Outcome.h>
#include <aws/gamelift/model/CreateGameSessionRequest.h>

namespace AWSGameLift
{
    namespace CreateSessionActivity
    {
        Aws::GameLift::Model::CreateGameSessionRequest BuildAWSGameLiftCreateGameSessionRequest(
            const AWSGameLiftCreateSessionRequest& createSessionRequest)
        {
            Aws::GameLift::Model::CreateGameSessionRequest request;
            // Optional attributes
            if (!createSessionRequest.m_creatorId.empty())
            {
                request.SetCreatorId(createSessionRequest.m_creatorId.c_str());
            }
            if (!createSessionRequest.m_sessionName.empty())
            {
                request.SetName(createSessionRequest.m_sessionName.c_str());
            }
            if (!createSessionRequest.m_idempotencyToken.empty())
            {
                request.SetIdempotencyToken(createSessionRequest.m_idempotencyToken.c_str());
            }
            AZStd::string propertiesOutput = "";
            Aws::Vector<Aws::GameLift::Model::GameProperty> properties;
            AWSGameLiftActivityUtils::GetGameProperties(createSessionRequest.m_sessionProperties, properties, propertiesOutput);
            if (!properties.empty())
            {
                request.SetGameProperties(properties);
            }

            // Required attributes
            if (!createSessionRequest.m_aliasId.empty())
            {
                request.SetAliasId(createSessionRequest.m_aliasId.c_str());
            }
            if (!createSessionRequest.m_fleetId.empty())
            {
                request.SetFleetId(createSessionRequest.m_fleetId.c_str());
            }
            request.SetMaximumPlayerSessionCount(static_cast<int>(createSessionRequest.m_maxPlayer));

            AZ_TracePrintf(AWSGameLiftCreateSessionActivityName,
                "Built CreateGameSessionRequest with CreatorId=%s, Name=%s, IdempotencyToken=%s, GameProperties=%s, AliasId=%s, FleetId=%s and MaximumPlayerSessionCount=%d",
                request.GetCreatorId().c_str(),
                request.GetName().c_str(),
                request.GetIdempotencyToken().c_str(),
                AZStd::string::format("[%s]", propertiesOutput.c_str()).c_str(),
                request.GetAliasId().c_str(),
                request.GetFleetId().c_str(),
                request.GetMaximumPlayerSessionCount());

            return request;
        }

        AZStd::string CreateSession(const AWSGameLiftCreateSessionRequest& createSessionRequest)
        {
            AZStd::string result = "";

            auto gameliftClient = AZ::Interface<IAWSGameLiftInternalRequests>::Get()->GetGameLiftClient();
            if (!gameliftClient)
            {
                AZ_Error(AWSGameLiftCreateSessionActivityName, false, AWSGameLiftClientMissingErrorMessage);
                return result;
            }

            AZ_TracePrintf(AWSGameLiftCreateSessionActivityName, "Requesting CreateGameSession against Amazon GameLift service ...");

            Aws::GameLift::Model::CreateGameSessionRequest request = BuildAWSGameLiftCreateGameSessionRequest(createSessionRequest);
            auto createSessionOutcome = gameliftClient->CreateGameSession(request);
            AZ_TracePrintf(AWSGameLiftCreateSessionActivityName, "CreateGameSession request against Amazon GameLift service is complete");

            if (createSessionOutcome.IsSuccess())
            {
                result = AZStd::string(createSessionOutcome.GetResult().GetGameSession().GetGameSessionId().c_str());
            }
            else
            {
                AZ_Error(AWSGameLiftCreateSessionActivityName, false, AWSGameLiftErrorMessageTemplate,
                    createSessionOutcome.GetError().GetExceptionName().c_str(), createSessionOutcome.GetError().GetMessage().c_str());
            }
            return result;
        }

        bool ValidateCreateSessionRequest(const AzFramework::CreateSessionRequest& createSessionRequest)
        {
            auto gameliftCreateSessionRequest = azrtti_cast<const AWSGameLiftCreateSessionRequest*>(&createSessionRequest);

            return gameliftCreateSessionRequest &&
                (!gameliftCreateSessionRequest->m_aliasId.empty() || !gameliftCreateSessionRequest->m_fleetId.empty());
        }
    } // namespace CreateSessionActivity
} // namespace AWSGameLift
