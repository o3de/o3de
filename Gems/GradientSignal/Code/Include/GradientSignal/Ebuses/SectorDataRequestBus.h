/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Vector3.h>

namespace GradientSignal
{
    /**
    * Handles queries regarding sector info
    */
    class SectorDataRequests
        : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////
        // EBusTraits
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        using MutexType = AZStd::recursive_mutex;
        ////////////////////////////////////////////////////////////////////////

        virtual ~SectorDataRequests() = default;

        // configured data
        virtual void GetPointsPerMeter(float& value) const = 0;
    };

    using SectorDataRequestBus = AZ::EBus<SectorDataRequests>;

    /**
    * Notifies about changes to sector data configuration
    */
    class SectorDataNotifications
        : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////
        // EBusTraits
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        using MutexType = AZStd::recursive_mutex;
        ////////////////////////////////////////////////////////////////////////

        virtual void OnSectorDataConfigurationUpdated() const = 0;
    };

    using SectorDataNotificationBus = AZ::EBus<SectorDataNotifications>;

} // namespace GradientSignal

