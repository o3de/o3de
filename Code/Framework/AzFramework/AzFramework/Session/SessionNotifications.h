/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

namespace AzFramework
{
    struct SessionConfig;

    //! SessionNotifications
    //! The session notifications to listen for performing required operations
    class SessionNotifications
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

        // OnSessionHealthCheck is fired in health check process
        // @return The result of all OnSessionHealthCheck
        virtual bool OnSessionHealthCheck() = 0;

        // OnCreateSessionBegin is fired at the beginning of session creation
        // @param  sessionConfig The properties to describe a session
        // @return The result of all OnCreateSessionBegin notifications
        virtual bool OnCreateSessionBegin(const SessionConfig& sessionConfig) = 0;

        // OnDestroySessionBegin is fired at the beginning of session termination
        // @return The result of all OnDestroySessionBegin notifications
        virtual bool OnDestroySessionBegin() = 0;
    };
    using SessionNotificationBus = AZ::EBus<SessionNotifications>;
} // namespace AzFramework
