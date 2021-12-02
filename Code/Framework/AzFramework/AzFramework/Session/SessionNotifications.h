/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        // Use this notification to perform any custom health check
        // @return True if OnSessionHealthCheck succeeds, false otherwise
        virtual bool OnSessionHealthCheck() = 0;

        // OnCreateSessionBegin is fired at the beginning of session creation process
        // Use this notification to perform any necessary configuration or initialization before
        // creating session
        // @param  sessionConfig The properties to describe a session
        // @return True if OnCreateSessionBegin succeeds, false otherwise
        virtual bool OnCreateSessionBegin(const SessionConfig& sessionConfig) = 0;

        // OnCreateSessionEnd is fired at the end of session creation process
        // Use this notification to perform any follow-up operation after session is created and active
        virtual void OnCreateSessionEnd() = 0;

        // OnDestroySessionBegin is fired at the beginning of session termination process
        // Use this notification to perform any cleanup operation before destroying session,
        // like gracefully disconnect players, cleanup data, etc.
        // @return True if OnDestroySessionBegin succeeds, false otherwise
        virtual bool OnDestroySessionBegin() = 0;

        // OnDestroySessionEnd is fired at the end of session termination process
        // Use this notification to perform any follow-up operation after session is destroyed,
        // like shutdown application process, etc.
        virtual void OnDestroySessionEnd() = 0;

        // OnUpdateSessionBegin is fired at the beginning of session update process
        // Use this notification to perform any configuration or initialization to handle
        // the session settings changing 
        // @param sessionConfig The properties to describe a session
        // @param updateReason The reason for session update
        virtual void OnUpdateSessionBegin(const SessionConfig& sessionConfig, const AZStd::string& updateReason) = 0;

        // OnUpdateSessionBegin is fired at the end of session update process
        // Use this notification to perform any follow-up operations after session is updated
        virtual void OnUpdateSessionEnd() = 0;
    };
    using SessionNotificationBus = AZ::EBus<SessionNotifications>;
} // namespace AzFramework
