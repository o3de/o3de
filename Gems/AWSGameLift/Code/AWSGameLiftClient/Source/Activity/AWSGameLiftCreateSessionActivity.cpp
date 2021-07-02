/*
 * /*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 */
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Activity/AWSGameLiftCreateSessionActivity.h>
#include <AWSGameLiftSessionConstants.h>

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
            for (auto iter = createSessionRequest.m_sessionProperties.begin();
                 iter != createSessionRequest.m_sessionProperties.end(); iter++)
            {
                Aws::GameLift::Model::GameProperty sessionProperty;
                sessionProperty.SetKey(iter->first.c_str());
                sessionProperty.SetValue(iter->second.c_str());
                request.AddGameProperties(sessionProperty);
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
            request.SetMaximumPlayerSessionCount(createSessionRequest.m_maxPlayer);

            return request;
        }

        AZStd::string CreateSession(
            const Aws::GameLift::GameLiftClient& gameliftClient,
            const AWSGameLiftCreateSessionRequest& createSessionRequest)
        {
            AZ_TracePrintf(AWSGameLiftCreateSessionActivityName, "Requesting CreateGameSession against Amazon GameLift service ...");

            AZStd::string result = "";
            Aws::GameLift::Model::CreateGameSessionRequest request = BuildAWSGameLiftCreateGameSessionRequest(createSessionRequest);
            auto createSessionOutcome = gameliftClient.CreateGameSession(request);

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

            return gameliftCreateSessionRequest && gameliftCreateSessionRequest->m_maxPlayer >= 0 &&
                (!gameliftCreateSessionRequest->m_aliasId.empty() || !gameliftCreateSessionRequest->m_fleetId.empty());
        }
    } // namespace CreateSessionActivity
} // namespace AWSGameLift
