/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace Physics
{
    //! Broadcasts notifications when heightfield data changes - heightfield providers implement HeightfieldRequests bus.
    class HeightfieldProviderNotifications : public AZ::ComponentBus
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        virtual void OnHeightfieldDataChanged(
            [[maybe_unused]] const AZ::Aabb& dirtyRegion)
        {
        }

        virtual void RefreshHeightfield()
        {
        }

    protected:
        ~HeightfieldProviderNotifications() = default;
    };

    using HeightfieldProviderNotificationBus = AZ::EBus<HeightfieldProviderNotifications>;

    class HeightfieldProviderRequests : public AZ::ComponentBus
    {
    public:
        virtual AZ::Vector2 GetHeightfieldGridSpacing() = 0;
        virtual void GetHeights(AZStd::vector<int16_t>& heights) = 0;
    };

    using HeightfieldProviderRequestsBus = AZ::EBus<HeightfieldProviderRequests>;
} // namespace AzFramework
