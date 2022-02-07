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
    //! MatchmakingNotifications
    //! The matchmaking notifications to listen for performing required operations
    //! based on matchmaking ticket event
    class MatchmakingNotifications
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

        // OnMatchAcceptance is fired when match is found and pending on acceptance
        // Use this notification to accept found match
        virtual void OnMatchAcceptance() = 0;

        // OnMatchComplete is fired when match is complete
        virtual void OnMatchComplete() = 0;

        // OnMatchError is fired when match is processed with error
        virtual void OnMatchError() = 0;

        // OnMatchFailure is fired when match is failed to complete
        virtual void OnMatchFailure() = 0;
    };
    using MatchmakingNotificationBus = AZ::EBus<MatchmakingNotifications>;
} // namespace AzFramework
