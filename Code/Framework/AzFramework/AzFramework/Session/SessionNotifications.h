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
} // namespace AzFramework
