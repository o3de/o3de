/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace AZ
{
    class Transform;
}

namespace LmbrCentral
{
    /**
     * Broadcasts change notifications from the water ocean and water volume components
     */
    class WaterNotifications
        : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////
        // EBusTraits
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        ////////////////////////////////////////////////////////////////////////

        //! allows multiple threads to call
        using MutexType = AZStd::recursive_mutex;

        //! Notifies when the height of the ocean changes
        virtual void OceanHeightChanged([[maybe_unused]] float height) {}

        //! Notifies when a water volume is moved
        virtual void WaterVolumeTransformChanged([[maybe_unused]] AZ::EntityId entityId, [[maybe_unused]] const AZ::Transform& worldTransform) {}

        //! Notifies when a water volume shape is changed
        virtual void WaterVolumeShapeChanged([[maybe_unused]] AZ::EntityId entityId) {}
    };

    using WaterNotificationBus = AZ::EBus<WaterNotifications>;
}
