/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#if defined(BUILD_GAMELIFT_CLIENT)

#include <GameLift/Session/GameLiftMatchmaking.h>
#include <GameLift/Session/GameLiftClientService.h>

#include <aws/core/utils/Outcome.h>

// To avoid the warning below
// Semaphore.h(50): warning C4251: 'Aws::Utils::Threading::Semaphore::m_mutex': class 'std::mutex' needs to have dll-interface to be used by clients of class 'Aws::Utils::Threading::Semaphore'
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option")
#include <aws/gamelift/model/StartMatchmakingRequest.h>
#include <aws/gamelift/model/DescribeMatchmakingRequest.h>
#include <aws/gamelift/model/DescribeGameSessionDetailsRequest.h>
#include <aws/gamelift/model/MatchmakingTicket.h>
#include <aws/gamelift/model/Player.h>
AZ_POP_DISABLE_WARNING

namespace GridMate
{
    GameLiftMatchmaking::GameLiftMatchmaking(GameLiftClientService* service, const AZStd::shared_ptr<GameLiftRequestInterfaceContext> context
    , const Aws::String& matchmakingConfigName)
        : GameLiftSearch(service, context)
        , m_matchmakingConfigName(matchmakingConfigName)
    {
    }

    bool GameLiftMatchmaking::Initialize()
    {
        Aws::GameLift::Model::StartMatchmakingRequest request;
        Aws::GameLift::Model::Player player;
        player.SetPlayerId(m_context->m_playerId.c_str());
        request.AddPlayers(player);
        request.SetConfigurationName(m_matchmakingConfigName);
        m_startMatchmakingOutcomeCallable = m_context->m_gameLiftClient.lock()->StartMatchmakingCallable(request);

        m_isDone = false;
        return true;
    }

    void GameLiftMatchmaking::SearchDone()
    {
        m_isDone = true;
    }

    void GameLiftMatchmaking::WaitForStartMatchmakingResult()
    {
        // Poll with 0 delay to see if callable is ready
        if (m_startMatchmakingOutcomeCallable.valid()
            && m_startMatchmakingOutcomeCallable.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
        {
            auto result = m_startMatchmakingOutcomeCallable.get();
            if (result.IsSuccess())
            {
                m_matchmakingTicket = result.GetResult().GetMatchmakingTicket();
                m_startDescribeMatchmakingTime = AZStd::chrono::system_clock::now();
            }
            else
            {
                AZ_TracePrintf("GameLift", "Matchmaking request failed with error: %s\n", result.GetError().GetMessage().c_str());
                SearchDone();
                return;
            }
        }
    }

    // GameLift recommends using Cloudwatch and SNS events instead of polling to avoid TPS limits.
    // Documentation: https://docs.aws.amazon.com/gamelift/latest/developerguide/match-notification.html
    void GameLiftMatchmaking::PollWithDelayDescribeMatchmaking()
    {
        GridMate::TimeStamp now = AZStd::chrono::system_clock::now();
        float timeElapsed = AZStd::chrono::duration<float>(now - m_startDescribeMatchmakingTime).count();
        if (timeElapsed > m_pollDescribeMatchmakingDelay && !m_describeMatchmakingOutcomeCallable.valid() && !m_matchmakingTicket.GetTicketId().empty())
        {
            m_startDescribeMatchmakingTime = AZStd::chrono::system_clock::now();
            Aws::GameLift::Model::DescribeMatchmakingRequest request;
            request.AddTicketIds(m_matchmakingTicket.GetTicketId());
            m_describeMatchmakingOutcomeCallable = m_context->m_gameLiftClient.lock()->DescribeMatchmakingCallable(request);
        }
    }

    void GameLiftMatchmaking::WaitForDescribeMatchmakingResult()
    {
        if (m_describeMatchmakingOutcomeCallable.valid()
            && m_describeMatchmakingOutcomeCallable.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
        {
            auto result = m_describeMatchmakingOutcomeCallable.get();
            if (result.IsSuccess())
            {
                for (auto ticket : result.GetResult().GetTicketList())
                {
                    if (ticket.GetTicketId() == m_matchmakingTicket.GetTicketId())
                    {
                        if (ticket.GetStatus() == Aws::GameLift::Model::MatchmakingConfigurationStatus::COMPLETED)
                        {
                            m_gameSessionConnectionInfo = ticket.GetGameSessionConnectionInfo();
                        }
                        else if (ticket.GetStatus() == Aws::GameLift::Model::MatchmakingConfigurationStatus::TIMED_OUT
                            || ticket.GetStatus() == Aws::GameLift::Model::MatchmakingConfigurationStatus::FAILED
                            || ticket.GetStatus() == Aws::GameLift::Model::MatchmakingConfigurationStatus::CANCELLED)
                        {
                            AZ_TracePrintf("GameLift", "Matchmaking request did not complete ticketId:%s status:%s message:%s", m_matchmakingTicket.GetTicketId().c_str(),
                                ticket.GetStatusReason().c_str(), ticket.GetStatusMessage().c_str());
                            SearchDone();
                        }
                        else
                        {
                            AZ_TracePrintf("GameLift", "Matchmaking request waiting to complete ticketId:%s status:%s message:%s", m_matchmakingTicket.GetTicketId().c_str(),
                                ticket.GetStatusReason().c_str(), ticket.GetStatusMessage().c_str());
                        }
                        break;
                    }
                }
            }
            else
            {
                AZ_TracePrintf("GameLift", "Matchmaking request failed with error: %s\n", result.GetError().GetMessage().c_str());
                SearchDone();
                return;
            }
        }

        // Game session connection found. End search and add to results.
        if (m_gameSessionConnectionInfo.GameSessionArnHasBeenSet())
        {
            GameLiftSearchInfo info;
            info.m_sessionId = m_gameSessionConnectionInfo.GetGameSessionArn().c_str();
            info.m_port = m_gameSessionConnectionInfo.GetPort();
            info.m_ipAddress = m_gameSessionConnectionInfo.GetIpAddress().c_str();
            for (auto playerSession : m_gameSessionConnectionInfo.GetMatchedPlayerSessions())
            {
                if (m_context->m_playerId.compare(playerSession.GetPlayerId().c_str()) == 0)
                {
                    info.m_playerSessionId = playerSession.GetPlayerSessionId().c_str();
                }
            }
            m_results.push_back(info);
            SearchDone();
        }
    }

    void GameLiftMatchmaking::Update()
    {
        if (m_isDone)
        {
            return;
        }

        WaitForStartMatchmakingResult();
        PollWithDelayDescribeMatchmaking();
        WaitForDescribeMatchmakingResult();
    }
} // namespace GridMate

#endif // BUILD_GAMELIFT_CLIENT
