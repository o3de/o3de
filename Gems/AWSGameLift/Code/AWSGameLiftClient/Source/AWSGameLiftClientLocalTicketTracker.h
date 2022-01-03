/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/thread.h>

#include <Request/AWSGameLiftMatchmakingRequestBus.h>

#include <aws/gamelift/model/MatchmakingTicket.h>

namespace AWSGameLift
{
    enum TicketTrackerStatus
    {
        Idle,
        Running
    };

    //! AWSGameLiftClientLocalTicketTracker
    //! GameLift client ticket tracker to describe submitted matchmaking ticket periodically,
    //! and join player to the match once matchmaking ticket is complete.
    //! For use in production, please see GameLifts guidance about matchmaking at volume.
    //! The continuous polling approach here is only suitable for low volume matchmaking and is meant to aid with development only
    class AWSGameLiftClientLocalTicketTracker
        : public AWSGameLiftMatchmakingEventRequestBus::Handler
    {
    public:
        static constexpr const char AWSGameLiftClientLocalTicketTrackerName[] = "AWSGameLiftClientLocalTicketTracker";
        // Set ticket polling period to 10 seconds
        // https://docs.aws.amazon.com/gamelift/latest/flexmatchguide/match-client.html#match-client-track
        static constexpr const uint64_t AWSGameLiftClientDefaultPollingPeriodInMS = 10000;

        AWSGameLiftClientLocalTicketTracker();
        virtual ~AWSGameLiftClientLocalTicketTracker() = default;

        virtual void ActivateTracker();
        virtual void DeactivateTracker();

        // AWSGameLiftMatchmakingEventRequestBus interface implementation
        void StartPolling(const AZStd::string& ticketId, const AZStd::string& playerId) override;
        void StopPolling() override;

    protected:
        // For testing friendly access
        uint64_t m_pollingPeriodInMS;
        TicketTrackerStatus m_status;

    private:
        void ProcessPolling(const AZStd::string& ticketId, const AZStd::string& playerId);
        void RequestPlayerJoinMatch(const Aws::GameLift::Model::MatchmakingTicket& ticket, const AZStd::string& playerId);

        AZStd::mutex m_trackerMutex;
        AZStd::thread m_trackerThread;
        AZStd::binary_semaphore m_waitEvent;
    };
} // namespace AWSGameLift
