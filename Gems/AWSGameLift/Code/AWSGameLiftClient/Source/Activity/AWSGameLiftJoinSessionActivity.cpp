/*
 * /*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 */
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>
#include <AzFramework/Session/ISessionHandlingRequests.h>

#include <Activity/AWSGameLiftJoinSessionActivity.h>
#include <AWSGameLiftSessionConstants.h>

namespace AWSGameLift
{
    namespace JoinSessionActivity
    {
        Aws::GameLift::Model::CreatePlayerSessionRequest BuildAWSGameLiftCreatePlayerSessionRequest(
            const AWSGameLiftJoinSessionRequest& joinSessionRequest)
        {
            Aws::GameLift::Model::CreatePlayerSessionRequest request;
            // Optional attributes
            if (!joinSessionRequest.m_playerData.empty())
            {
                request.SetPlayerData(joinSessionRequest.m_playerData.c_str());
            }
            // Required attributes
            request.SetPlayerId(joinSessionRequest.m_playerId.c_str());
            request.SetGameSessionId(joinSessionRequest.m_sessionId.c_str());
            return request;
        }

        AzFramework::SessionConnectionConfig BuildSessionConnectionConfig(
            const Aws::GameLift::Model::CreatePlayerSessionOutcome& createPlayerSessionOutcome)
        {
            AzFramework::SessionConnectionConfig sessionConnectionConfig;
            auto createPlayerSessionResult = createPlayerSessionOutcome.GetResult();
            // TODO: AWSNativeSDK needs to be updated to support this attribute, and it is a must have for TLS certificate enabled fleet
            //sessionConnectionConfig.m_dnsName = createPlayerSessionResult.GetPlayerSession().GetDnsName().c_str();
            sessionConnectionConfig.m_ipAddress = createPlayerSessionResult.GetPlayerSession().GetIpAddress().c_str();
            sessionConnectionConfig.m_playerSessionId = createPlayerSessionResult.GetPlayerSession().GetPlayerSessionId().c_str();
            sessionConnectionConfig.m_port = createPlayerSessionResult.GetPlayerSession().GetPort();
            return sessionConnectionConfig;
        }

        Aws::GameLift::Model::CreatePlayerSessionOutcome CreatePlayerSession(
            const Aws::GameLift::GameLiftClient& gameliftClient,
            const AWSGameLiftJoinSessionRequest& joinSessionRequest)
        {
            AZ_TracePrintf(AWSGameLiftJoinSessionActivityName,
                "Requesting CreatePlayerSession for player %s against Amazon GameLift service ...",
                joinSessionRequest.m_playerId.c_str());

            Aws::GameLift::Model::CreatePlayerSessionRequest request =
                BuildAWSGameLiftCreatePlayerSessionRequest(joinSessionRequest);
            auto createPlayerSessionOutcome = gameliftClient.CreatePlayerSession(request);

            if (!createPlayerSessionOutcome.IsSuccess())
            {
                AZ_Error(AWSGameLiftJoinSessionActivityName, false, AWSGameLiftErrorMessageTemplate,
                    createPlayerSessionOutcome.GetError().GetExceptionName().c_str(),
                    createPlayerSessionOutcome.GetError().GetMessage().c_str());
            }
            return createPlayerSessionOutcome;
        }

        bool RequestPlayerJoinSession(const Aws::GameLift::Model::CreatePlayerSessionOutcome& createPlayerSessionOutcome)
        {
            bool result = false;
            if (createPlayerSessionOutcome.IsSuccess())
            {
                auto clientRequestHandler = AZ::Interface<AzFramework::ISessionHandlingClientRequests>::Get();
                if (clientRequestHandler)
                {
                    AZ_TracePrintf(AWSGameLiftJoinSessionActivityName, "Requesting player to connect to game session ...");

                    AzFramework::SessionConnectionConfig sessionConnectionConfig =
                        BuildSessionConnectionConfig(createPlayerSessionOutcome);
                    result = clientRequestHandler->RequestPlayerJoinSession(sessionConnectionConfig);
                }
                else
                {
                    AZ_Error(AWSGameLiftJoinSessionActivityName, false, AWSGameLiftJoinSessionMissingRequestHandlerErrorMessage);
                }
            }
            return result;
        }

        bool ValidateJoinSessionRequest(const AzFramework::JoinSessionRequest& joinSessionRequest)
        {
            auto gameliftJoinSessionRequest = azrtti_cast<const AWSGameLiftJoinSessionRequest*>(&joinSessionRequest);

            if (gameliftJoinSessionRequest &&
                !gameliftJoinSessionRequest->m_playerId.empty() &&
                !gameliftJoinSessionRequest->m_sessionId.empty())
            {
                return true;
            }
            else
            {
                AZ_Error(AWSGameLiftJoinSessionActivityName, false, AWSGameLiftJoinSessionRequestInvalidErrorMessage);

                return false;
            }
        }
    } // namespace JoinSessionActivity
} // namespace AWSGameLift
