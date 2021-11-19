/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

#include <AWSGameLiftPlayer.h>

namespace AWSGameLift
{
    //! IAWSGameLiftServerRequests
    //! Server interfaces to expose Amazon GameLift Server SDK
    class IAWSGameLiftServerRequests
    {
    public:
        AZ_RTTI(IAWSGameLiftServerRequests, "{D76CD98D-4C37-4C25-82C4-4E8772706D70}");

        IAWSGameLiftServerRequests() = default;
        virtual ~IAWSGameLiftServerRequests() = default;

        //! Notify GameLift that the server process is ready to host a game session.
        //! @return True if the ProcessReady notification is sent to GameLift successfully, false otherwise
        virtual bool NotifyGameLiftProcessReady() = 0;

        //! Sends a request to find new players for open slots in a game session created with FlexMatch.
        //! @param  ticketId Unique identifier for match backfill request ticket
        //! @param  players A set of data representing all players who are currently in the game session,
        //!         if not provided, system will use lazy loaded game session data which is not guaranteed to
        //!         be accurate (no latency data either)
        //! @return True if StartMatchBackfill succeeds, false otherwise
        virtual bool StartMatchBackfill(const AZStd::string& ticketId, const AZStd::vector<AWSGameLiftPlayer>& players) = 0;

        //! Cancels an active match backfill request that was created with StartMatchBackfill
        //! @param  ticketId Unique identifier of the backfill request ticket to be canceled
        //! @return True if StopMatchBackfill succeeds, false otherwise
        virtual bool StopMatchBackfill(const AZStd::string& ticketId) = 0;
    };

    // IAWSGameLiftServerRequests EBus wrapper
    class AWSGameLiftServerRequests
        : public AZ::EBusTraits
    {
    public:
        using MutexType = AZStd::recursive_mutex;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    };
    using AWSGameLiftServerRequestBus = AZ::EBus<IAWSGameLiftServerRequests, AWSGameLiftServerRequests>;
} // namespace AWSGameLift
