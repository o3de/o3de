/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>
#include <AzCore/std/bind/bind.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzFramework/Session/ISessionHandlingRequests.h>
#include <AzFramework/Matchmaking/MatchmakingNotifications.h>

#include <AWSGameLiftClientLocalTicketTracker.h>
#include <AWSGameLiftSessionConstants.h>
#include <Request/IAWSGameLiftInternalRequests.h>

#include <aws/core/utils/Outcome.h>
#include <aws/gamelift/GameLiftClient.h>
#include <aws/gamelift/model/DescribeMatchmakingRequest.h>

namespace AWSGameLift
{
    AWSGameLiftClientLocalTicketTracker::AWSGameLiftClientLocalTicketTracker()
        : m_status(TicketTrackerStatus::Idle)
        , m_pollingPeriodInMS(AWSGameLiftClientDefaultPollingPeriodInMS)
    {
    }

    void AWSGameLiftClientLocalTicketTracker::ActivateTracker()
    {
        AZ::Interface<IAWSGameLiftMatchmakingEventRequests>::Register(this);
        AWSGameLiftMatchmakingEventRequestBus::Handler::BusConnect();
    }

    void AWSGameLiftClientLocalTicketTracker::DeactivateTracker()
    {
        AWSGameLiftMatchmakingEventRequestBus::Handler::BusDisconnect();
        AZ::Interface<IAWSGameLiftMatchmakingEventRequests>::Unregister(this);
        StopPolling();
    }

    void AWSGameLiftClientLocalTicketTracker::StartPolling(
        const AZStd::string& ticketId, const AZStd::string& playerId)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_trackerMutex);
        if (m_status == TicketTrackerStatus::Running)
        {
            AZ_TracePrintf(AWSGameLiftClientLocalTicketTrackerName, "Matchmaking ticket tracker is running.");
            return;
        }

        // Make sure thread and wait event are both in clean state before starting new one
        m_waitEvent.release();
        if (m_trackerThread.joinable())
        {
            m_trackerThread.join();
        }
        m_waitEvent.acquire();

        m_status = TicketTrackerStatus::Running;
        m_trackerThread = AZStd::thread(AZStd::bind(
            &AWSGameLiftClientLocalTicketTracker::ProcessPolling, this, ticketId, playerId));
    }

    void AWSGameLiftClientLocalTicketTracker::StopPolling()
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_trackerMutex);
        m_status = TicketTrackerStatus::Idle;
        m_waitEvent.release();
        if (m_trackerThread.joinable())
        {
            m_trackerThread.join();
        }
    }

    void AWSGameLiftClientLocalTicketTracker::ProcessPolling(
        const AZStd::string& ticketId, const AZStd::string& playerId)
    {
        while (m_status == TicketTrackerStatus::Running)
        {
            auto gameliftClient = AZ::Interface<IAWSGameLiftInternalRequests>::Get()->GetGameLiftClient();
            if (gameliftClient)
            {
                Aws::GameLift::Model::DescribeMatchmakingRequest request;
                request.AddTicketIds(ticketId.c_str());

                auto describeMatchmakingOutcome = gameliftClient->DescribeMatchmaking(request);
                if (describeMatchmakingOutcome.IsSuccess())
                {
                    if (describeMatchmakingOutcome.GetResult().GetTicketList().size() == 1)
                    {
                        auto ticket = describeMatchmakingOutcome.GetResult().GetTicketList().front();
                        if (ticket.GetStatus() == Aws::GameLift::Model::MatchmakingConfigurationStatus::COMPLETED)
                        {
                            AZ_TracePrintf(AWSGameLiftClientLocalTicketTrackerName,
                                "Matchmaking ticket %s is complete.", ticket.GetTicketId().c_str());
                            RequestPlayerJoinMatch(ticket, playerId);
                            AzFramework::MatchmakingNotificationBus::Broadcast(&AzFramework::MatchmakingNotifications::OnMatchComplete);
                            m_status = TicketTrackerStatus::Idle;
                            return;
                        }
                        else if (ticket.GetStatus() == Aws::GameLift::Model::MatchmakingConfigurationStatus::TIMED_OUT ||
                            ticket.GetStatus() == Aws::GameLift::Model::MatchmakingConfigurationStatus::FAILED ||
                            ticket.GetStatus() == Aws::GameLift::Model::MatchmakingConfigurationStatus::CANCELLED)
                        {
                            AZ_Warning(AWSGameLiftClientLocalTicketTrackerName, false, "Matchmaking ticket %s is not complete, %s",
                                ticket.GetTicketId().c_str(), ticket.GetStatusMessage().c_str());
                            AzFramework::MatchmakingNotificationBus::Broadcast(&AzFramework::MatchmakingNotifications::OnMatchFailure);
                            m_status = TicketTrackerStatus::Idle;
                            return;
                        }
                        else if (ticket.GetStatus() == Aws::GameLift::Model::MatchmakingConfigurationStatus::REQUIRES_ACCEPTANCE)
                        {
                            AZ_TracePrintf(AWSGameLiftClientLocalTicketTrackerName, "Matchmaking ticket %s is pending on acceptance, %s.",
                                ticket.GetTicketId().c_str(), ticket.GetStatusMessage().c_str());
                            AzFramework::MatchmakingNotificationBus::Broadcast(&AzFramework::MatchmakingNotifications::OnMatchAcceptance);
                        }
                        else
                        {
                            AZ_TracePrintf(AWSGameLiftClientLocalTicketTrackerName, "Matchmaking ticket %s is processing, %s.",
                                ticket.GetTicketId().c_str(), ticket.GetStatusMessage().c_str());
                        }
                    }
                    else
                    {
                        AZ_Error(AWSGameLiftClientLocalTicketTrackerName, false, "Unable to find expected ticket with id %s", ticketId.c_str());
                        AzFramework::MatchmakingNotificationBus::Broadcast(&AzFramework::MatchmakingNotifications::OnMatchError);
                    }
                }
                else
                {
                    AZ_Error(AWSGameLiftClientLocalTicketTrackerName, false, AWSGameLiftErrorMessageTemplate,
                        describeMatchmakingOutcome.GetError().GetExceptionName().c_str(),
                        describeMatchmakingOutcome.GetError().GetMessage().c_str());
                    AzFramework::MatchmakingNotificationBus::Broadcast(&AzFramework::MatchmakingNotifications::OnMatchError);
                }
            }
            else
            {
                AZ_Error(AWSGameLiftClientLocalTicketTrackerName, false, AWSGameLiftClientMissingErrorMessage);
                AzFramework::MatchmakingNotificationBus::Broadcast(&AzFramework::MatchmakingNotifications::OnMatchError);
            }
            m_waitEvent.try_acquire_for(AZStd::chrono::milliseconds(m_pollingPeriodInMS));
        }
    }

    void AWSGameLiftClientLocalTicketTracker::RequestPlayerJoinMatch(
        const Aws::GameLift::Model::MatchmakingTicket& ticket, const AZStd::string& playerId)
    {
        auto connectionInfo = ticket.GetGameSessionConnectionInfo();
        AzFramework::SessionConnectionConfig sessionConnectionConfig;
        sessionConnectionConfig.m_ipAddress = connectionInfo.GetIpAddress().c_str();
        for (auto matchedPlayer : connectionInfo.GetMatchedPlayerSessions())
        {
            if (playerId.compare(matchedPlayer.GetPlayerId().c_str()) == 0)
            {
                sessionConnectionConfig.m_playerSessionId = matchedPlayer.GetPlayerSessionId().c_str();
                break;
            }
        }
        sessionConnectionConfig.m_port = static_cast<uint16_t>(connectionInfo.GetPort());

        if (!sessionConnectionConfig.m_playerSessionId.empty())
        {
            AZ_TracePrintf(AWSGameLiftClientLocalTicketTrackerName,
                "Requesting and validating player session %s to connect to the match ...",
                sessionConnectionConfig.m_playerSessionId.c_str());
            bool result =
                AZ::Interface<AzFramework::ISessionHandlingClientRequests>::Get()->RequestPlayerJoinSession(sessionConnectionConfig);
            if (result)
            {
                AZ_TracePrintf(AWSGameLiftClientLocalTicketTrackerName,
                    "Started connection process, and connection validation is in process.");
            }
            else
            {
                AZ_Error(AWSGameLiftClientLocalTicketTrackerName, false,
                    "Failed to start connection process.");
            }
        }
        else
        {
            AZ_Error(AWSGameLiftClientLocalTicketTrackerName, false,
                "Player session id is missing for player % to join the match.", playerId.c_str());
        }
    }
}
