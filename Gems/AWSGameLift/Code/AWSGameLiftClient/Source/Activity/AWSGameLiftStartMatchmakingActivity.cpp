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
#include <Activity/AWSGameLiftStartMatchmakingActivity.h>
#include <AWSGameLiftPlayer.h>
#include <AWSGameLiftSessionConstants.h>
#include <Request/IAWSGameLiftInternalRequests.h>

#include <aws/core/utils/Outcome.h>
#include <aws/gamelift/model/StartMatchmakingRequest.h>

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
            for (const AWSGameLiftPlayer& playerInfo : startMatchmakingRequest.m_players)
            {
                Aws::GameLift::Model::Player player;
                if (!playerInfo.m_playerId.empty())
                {
                    player.SetPlayerId(playerInfo.m_playerId.c_str());
                }

                // Optional attributes
                if (!playerInfo.m_team.empty())
                {
                    player.SetTeam(playerInfo.m_team.c_str());
                }
                if (playerInfo.m_latencyInMs.size() > 0)
                {
                    Aws::Map<Aws::String, int> regionToLatencyMap;
                    AWSGameLiftActivityUtils::ConvertRegionToLatencyMap(playerInfo.m_latencyInMs, regionToLatencyMap);
                    player.SetLatencyInMs(AZStd::move(regionToLatencyMap));
                }
                if (playerInfo.m_playerAttributes.size() > 0)
                {
                    Aws::Map<Aws::String, Aws::GameLift::Model::AttributeValue> playerAttributes;
                    AWSGameLiftActivityUtils::ConvertPlayerAttributes(playerInfo.m_playerAttributes, playerAttributes);
                    player.SetPlayerAttributes(AZStd::move(playerAttributes));
                }
                players.emplace_back(player);
            }
            if (startMatchmakingRequest.m_players.size() > 0)
            {
                request.SetPlayers(players);
            }

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
            const AWSGameLiftStartMatchmakingRequest& startMatchmakingRequest)
        {
            AZStd::string result = "";

            auto gameliftClient = AZ::Interface<IAWSGameLiftInternalRequests>::Get()->GetGameLiftClient();
            if (!gameliftClient)
            {
                AZ_Error(AWSGameLiftStartMatchmakingActivityName, false, AWSGameLiftClientMissingErrorMessage);
                return result;
            }

            AZ_TracePrintf(AWSGameLiftStartMatchmakingActivityName, "Requesting StartMatchmaking against Amazon GameLift service ...");

            Aws::GameLift::Model::StartMatchmakingRequest request = BuildAWSGameLiftStartMatchmakingRequest(startMatchmakingRequest);
            auto startMatchmakingOutcome = gameliftClient->StartMatchmaking(request);
            if (startMatchmakingOutcome.IsSuccess())
            {
                result = AZStd::string(startMatchmakingOutcome.GetResult().GetMatchmakingTicket().GetTicketId().c_str());

                AZ_TracePrintf(AWSGameLiftStartMatchmakingActivityName, "StartMatchmaking request against Amazon GameLift service is complete");
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
                for (const AWSGameLiftPlayer& playerInfo : gameliftStartMatchmakingRequest->m_players)
                {
                    isValid &= !playerInfo.m_playerId.empty();
                    isValid &= AWSGameLiftActivityUtils::ValidatePlayerAttributes(playerInfo.m_playerAttributes);
                }
            }

            AZ_Error(AWSGameLiftStartMatchmakingActivityName, isValid, AWSGameLiftStartMatchmakingRequestInvalidErrorMessage);

            return isValid;
        }
    } // namespace StartMatchmakingActivity
} // namespace AWSGameLift
