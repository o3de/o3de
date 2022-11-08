/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace AzFramework
{
    //! Provides an interface to notify changes to ComponentModeDelegate state.
    class ComponentModeDelegateNotifications : public AZ::EBusTraits
    {
    public:
        //! Notification that a component has connected to the ComponentModeDelegate.
        virtual void OnComponentModeDelegateConnect(const AZ::EntityComponentIdPair&)
        {
        }

        //! Notification that a component has disconnected from the ComponentModeDelegate.
        virtual void OnComponentModeDelegateDisconnect(const AZ::EntityComponentIdPair&)
        {
        }
    };

    using ComponentModeDelegateNotificationBus = AZ::EBus<ComponentModeDelegateNotifications>;
} // namespace AzFramework
