/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/Entity.h>

namespace AZ
{
    class Aabb;
}

namespace SurfaceData
{
    /**
    * the EBus is used to send notification information about surfaces
    */
    class SurfaceDataSystemNotifications
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

        virtual void OnSurfaceChanged(const AZ::EntityId& entityId, const AZ::Aabb& oldBounds, const AZ::Aabb& newBounds) = 0;
    };

    typedef AZ::EBus<SurfaceDataSystemNotifications> SurfaceDataSystemNotificationBus;
}
