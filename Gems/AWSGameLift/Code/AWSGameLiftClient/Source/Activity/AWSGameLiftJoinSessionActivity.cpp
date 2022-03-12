/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

#include <Activity/AWSGameLiftJoinSessionActivity.h>
#include <AWSGameLiftSessionConstants.h>
#include <Request/IAWSGameLiftInternalRequests.h>

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

            AZ_TracePrintf(AWSGameLiftJoinSessionActivityName,
                "Built CreatePlayerSessionRequest with PlayerData=%s, PlayerId=%s and GameSessionId=%s",
                request.GetPlayerData().c_str(),
                request.GetPlayerId().c_str(),
                request.GetGameSessionId().c_str());

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
            sessionConnectionConfig.m_port = static_cast<uint16_t>(createPlayerSessionResult.GetPlayerSession().GetPort());

            AZ_TracePrintf(AWSGameLiftJoinSessionActivityName,
                "Built SessionConnectionConfig with IpAddress=%s, PlayerSessionId=%s and Port=%d",
                sessionConnectionConfig.m_ipAddress.c_str(),
                sessionConnectionConfig.m_playerSessionId.c_str(),
                sessionConnectionConfig.m_port);

            return sessionConnectionConfig;
        }

        Aws::GameLift::Model::CreatePlayerSessionOutcome CreatePlayerSession(
            const AWSGameLiftJoinSessionRequest& joinSessionRequest)
        {
            Aws::GameLift::Model::CreatePlayerSessionOutcome createPlayerSessionOutcome;

            auto gameliftClient = AZ::Interface<IAWSGameLiftInternalRequests>::Get()->GetGameLiftClient();
            if (!gameliftClient)
            {
                AZ_Error(AWSGameLiftJoinSessionActivityName, false, AWSGameLiftClientMissingErrorMessage);
                return createPlayerSessionOutcome;
            }

            AZ_TracePrintf(AWSGameLiftJoinSessionActivityName,
                "Requesting CreatePlayerSession for player %s against Amazon GameLift service ...",
                joinSessionRequest.m_playerId.c_str());

            Aws::GameLift::Model::CreatePlayerSessionRequest request =
                BuildAWSGameLiftCreatePlayerSessionRequest(joinSessionRequest);
            createPlayerSessionOutcome = gameliftClient->CreatePlayerSession(request);
            AZ_TracePrintf(AWSGameLiftJoinSessionActivityName,
                "CreatePlayerSession request for player %s against Amazon GameLift service is complete", joinSessionRequest.m_playerId.c_str());

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
                    AzFramework::SessionConnectionConfig sessionConnectionConfig =
                        BuildSessionConnectionConfig(createPlayerSessionOutcome);

                    AZ_TracePrintf(AWSGameLiftJoinSessionActivityName,
                        "Requesting and validating player session %s to connect to game session ...", sessionConnectionConfig.m_playerSessionId.c_str());
                    result = clientRequestHandler->RequestPlayerJoinSession(sessionConnectionConfig);
                    AZ_TracePrintf(AWSGameLiftJoinSessionActivityName, "Started connection process, and connection validation is in process.");
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
