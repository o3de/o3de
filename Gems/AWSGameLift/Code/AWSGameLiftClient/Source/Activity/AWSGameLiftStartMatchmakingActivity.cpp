/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Interface/Interface.h>

#include <Activity/AWSGameLiftActivityUtils.h>
#include <Activity/AWSGameLiftStartMatchmakingActivity.h>
#include <AWSGameLiftSessionConstants.h>
#include <Request/IAWSGameLiftMatchmakingInternalRequests.h>

namespace AWSGameLift
{
    namespace StartMatchmakingActivity
    {
        Aws::GameLift::Model::StartMatchmakingRequest BuildAWSGameLiftStartMatchmakingRequest(
            const AWSGameLiftStartMatchmakingRequest& startMatchmakingRequest)
        {
            Aws::GameLift::Model::StartMatchmakingRequest request;
            if (!startMatchmakingRequest.m_configurationName.empty())
            {
                request.SetConfigurationName(startMatchmakingRequest.m_configurationName.c_str());
            }

            Aws::Vector<Aws::GameLift::Model::Player> players;
            for (const AWSGameLiftPlayerInformation& playerInfo : startMatchmakingRequest.m_players)
            {
                Aws::GameLift::Model::Player player;
                // Optional attributes
                player.SetPlayerId(playerInfo.m_playerId.c_str());
                player.SetTeam(playerInfo.m_team.c_str());

                Aws::Map<Aws::String, int> regionToLatencyMap;
                AWSGameLiftActivityUtils::GetRegionToLatencyMap(playerInfo.m_latencyInMs, regionToLatencyMap);
                player.SetLatencyInMs(AZStd::move(regionToLatencyMap));

                Aws::Map<Aws::String, Aws::GameLift::Model::AttributeValue> playerAttributes;
                AWSGameLiftActivityUtils::GetPlayerAttributes(playerInfo.m_playerAttributes, playerAttributes);
                player.SetPlayerAttributes(AZStd::move(playerAttributes));
            }
            request.SetPlayers(players);

            // Optional attributes
            if (!startMatchmakingRequest.m_ticketId.empty())
            {
                request.SetTicketId(startMatchmakingRequest.m_ticketId.c_str());
            }

            AZ_TracePrintf(AWSGameLiftStartMatchmakingActivityName,
                "Built StartMatchmakingRequest with TicketId=%s, ConfigurationName=%s and PlayersCount=%d",
                request.GetTicketId().c_str(),
                request.GetConfigurationName().c_str(),
                request.GetPlayers().size());

            return request;
        }

        AZStd::string StartMatchmaking(
            const Aws::GameLift::GameLiftClient& gameliftClient,
            const AWSGameLiftStartMatchmakingRequest& startMatchmakingRequest)
        {
            AZ_TracePrintf(AWSGameLiftStartMatchmakingActivityName, "Requesting StartMatchmaking against Amazon GameLift service ...");

            AZStd::string result = "";
            Aws::GameLift::Model::StartMatchmakingRequest request = BuildAWSGameLiftStartMatchmakingRequest(startMatchmakingRequest);
            auto startMatchmakingOutcome = gameliftClient.StartMatchmaking(request);
            AZ_TracePrintf(AWSGameLiftStartMatchmakingActivityName, "StartMatchmaking request against Amazon GameLift service is complete");

            if (startMatchmakingOutcome.IsSuccess())
            {
                result = AZStd::string(startMatchmakingOutcome.GetResult().GetMatchmakingTicket().GetTicketId().c_str());

                // Start the local ticket tracker for polling matchmaking ticket based on the given ticket id and player id
                AZ_Warning(AWSGameLiftStartMatchmakingActivityName, startMatchmakingRequest.m_players.size() == 1,
                    AWSGameLiftStartMatchmakingMultiplePlayersWarningMessage);

                AZ::Interface<IAWSGameLiftMatchmakingInternalRequests>::Get()->StartPolling(
                    result, startMatchmakingRequest.m_players[0].m_playerId);
            }
            else
            {
                AZ_Error(AWSGameLiftStartMatchmakingActivityName, false, AWSGameLiftErrorMessageTemplate,
                    startMatchmakingOutcome.GetError().GetExceptionName().c_str(), startMatchmakingOutcome.GetError().GetMessage().c_str());
            }

            return result;
        }

        bool ValidateStartMatchmakingRequest(const AzFramework::StartMatchmakingRequest& startMatchmakingRequest)
        {
            auto gameliftStartMatchmakingRequest = azrtti_cast<const AWSGameLiftStartMatchmakingRequest*>(&startMatchmakingRequest);
            bool isValid = gameliftStartMatchmakingRequest &&
                (!gameliftStartMatchmakingRequest->m_configurationName.empty()) &&
                gameliftStartMatchmakingRequest->m_players.size() > 0;

            if (isValid)
            {
                for (const AWSGameLiftPlayerInformation& playerInfo : gameliftStartMatchmakingRequest->m_players)
                {
                    isValid &= AWSGameLiftActivityUtils::ValidatePlayerAttributes(playerInfo.m_playerAttributes);
                }
            }

            AZ_Error(AWSGameLiftStartMatchmakingActivityName, isValid, AWSGameLiftStartMatchmakingRequestInvalidErrorMessage);

            return isValid;
        }
    } // namespace StartMatchmakingActivity
} // namespace AWSGameLift
