/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/EBus/EBus.h>
#include <AzToolsFramework/AzToolsFrameworkAPI.h>

namespace AzToolsFramework
{
    //! Used to notify changes of state for Actions in the Action Manager.
    class ActionManagerNotifications : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        //////////////////////////////////////////////////////////////////////////

        //! Triggered when an action's enabled or checked state changes.
        virtual void OnActionStateChanged([[maybe_unused]] AZStd::string actionIdentifier) {}

    protected:
        ~ActionManagerNotifications() = default;
    };

    using ActionManagerNotificationBus = AZ::EBus<ActionManagerNotifications>;

} // namespace AzToolsFramework

AZ_DECLARE_EBUS_SINGLE_ADDRESS(AZTF_API, AzToolsFramework::ActionManagerNotifications)
