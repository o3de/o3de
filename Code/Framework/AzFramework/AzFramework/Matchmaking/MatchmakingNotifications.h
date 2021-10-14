/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>

namespace AzFramework
{
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

        // OnAcceptMatchAsyncComplete is fired once AcceptMatchAsync completes
        virtual void OnAcceptMatchAsyncComplete() = 0;

        // OnStartMatchmakingAsyncComplete is fired once StartMatchmakingAsync completes
        // @param matchmakingTicketId The unique identifier for the matchmaking ticket
        virtual void OnStartMatchmakingAsyncComplete(const AZStd::string& matchmakingTicketId) = 0;

        // OnStopMatchmakingAsyncComplete is fired once StopMatchmakingAsync completes
        virtual void OnStopMatchmakingAsyncComplete() = 0;
    };
    using MatchmakingAsyncRequestNotificationBus = AZ::EBus<MatchmakingAsyncRequestNotifications>;

    //! MatchmakingNotifications
    //! The matchmaking notifications to listen for performing required operations
    class MatchAcceptanceNotifications
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

        // OnMatchAcceptance is fired when DescribeMatchmaking ticket status is REQUIRES_ACCEPTANCE
        virtual void OnMatchAcceptance() = 0;
    };
    using MatchAcceptanceNotificationBus = AZ::EBus<MatchAcceptanceNotifications>;
} // namespace AzFramework
