/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/string/string.h>
#include <AzFramework/Matchmaking/MatchmakingRequests.h>

namespace AzFramework
{
    //! IMatchmakingRequests
    //! Pure virtual session interface class to abstract the details of session handling from application code.
    class IMatchmakingRequests
    {
    public:
        AZ_RTTI(IMatchmakingRequests, "{BC0B74DA-A448-4F40-9B50-9D73142829D5}");

        IMatchmakingRequests() = default;
        virtual ~IMatchmakingRequests() = default;

        //! Registers a player's acceptance or rejection of a proposed matchmaking.
        //! @param  acceptMatchRequest The request of AcceptMatch operation
        virtual void AcceptMatch(const AcceptMatchRequest& acceptMatchRequest) = 0;

        //! Create a game match for a group of players.
        //! @param  startMatchmakingRequest The request of StartMatchmaking operation
        //! @return A unique identifier for a matchmaking ticket
        virtual AZStd::string StartMatchmaking(const StartMatchmakingRequest& startMatchmakingRequest) = 0;

        //! Cancels a matchmaking ticket that is currently being processed.
        //! @param  stopMatchmakingRequest The request of StopMatchmaking operation
        virtual void StopMatchmaking(const StopMatchmakingRequest& stopMatchmakingRequest) = 0;
    };

    //! IMatchmakingAsyncRequests
    //! Async version of IMatchmakingRequests
    class IMatchmakingAsyncRequests
    {
    public:
        AZ_RTTI(IMatchmakingAsyncRequests, "{53513480-2D02-493C-B44E-96AA27F42429}");

        IMatchmakingAsyncRequests() = default;
        virtual ~IMatchmakingAsyncRequests() = default;

        //! AcceptMatch Async
        //! @param  acceptMatchRequest The request of AcceptMatch operation
        virtual void AcceptMatchAsync(const AcceptMatchRequest& acceptMatchRequest) = 0;

        //! StartMatchmaking Async
        //! @param  startMatchmakingRequest The request of StartMatchmaking operation
        virtual void StartMatchmakingAsync(const StartMatchmakingRequest& startMatchmakingRequest) = 0;

        //! StopMatchmaking Async
        //! @param  stopMatchmakingRequest The request of StopMatchmaking operation
        virtual void StopMatchmakingAsync(const StopMatchmakingRequest& stopMatchmakingRequest) = 0;
    };

    //! MatchmakingAsyncRequestNotifications
    //! The notifications correspond to matchmaking async requests
    class MatchmakingAsyncRequestNotifications
        : public AZ::EBusTraits
    {
    public:
        // Safeguard handler for multi-threaded use case
        using MutexType = AZStd::recursive_mutex;

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        //! OnAcceptMatchAsyncComplete is fired once AcceptMatchAsync completes
        virtual void OnAcceptMatchAsyncComplete() = 0;

        //! OnStartMatchmakingAsyncComplete is fired once StartMatchmakingAsync completes
        //! @param matchmakingTicketId The unique identifier for the matchmaking ticket
        virtual void OnStartMatchmakingAsyncComplete(const AZStd::string& matchmakingTicketId) = 0;

        //! OnStopMatchmakingAsyncComplete is fired once StopMatchmakingAsync completes
        virtual void OnStopMatchmakingAsyncComplete() = 0;
    };
    using MatchmakingAsyncRequestNotificationBus = AZ::EBus<MatchmakingAsyncRequestNotifications>;
} // namespace AzFramework
