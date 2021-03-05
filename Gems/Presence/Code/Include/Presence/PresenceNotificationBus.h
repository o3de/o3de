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
#include <AzFramework/Input/User/LocalUserId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>

namespace Presence
{
    ////////////////////////////////////////////////////////////////////////////////////////
    //! EBUS interface used to listen for presence request events
    class PresenceNotifications
        : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: presence notifications can be handled by multiple listeners
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: presence notifications are addressed to a single address
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Override to be notified when presence has been successfully set. Unsuccessful attempts
        //! are logged.
        //! \param[in] the local user ID for the user whose presence has been set.
        virtual void OnPresenceSet(const AzFramework::LocalUserId& localUserId) = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Override to be notified when presence has been successfully queried. Unsuccessful attempts
        //! are logged.
        //! \param[in] struct representing presence details populated by query request
        virtual void OnPresenceQueried(const PresenceDetails& presenceDetails) = 0;
    };
    using PresenceNotificationBus = AZ::EBus<PresenceNotifications>;
} // namespace Presence