/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Aabb.h>

namespace AZ
{
    /**
    * the EBus is used to request information about potential vegetation surfaces
    */
    class HeightmapUpdateNotification
        : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////
        // EBusTraits
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        ////////////////////////////////////////////////////////////////////////

        // Occurs when the terrain height map is modified.
        virtual void HeightmapModified(const AZ::Aabb& bounds) = 0;
    };

    typedef AZ::EBus<HeightmapUpdateNotification> HeightmapUpdateNotificationBus;
}
