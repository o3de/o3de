/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Component/ComponentBus.h>

namespace AzToolsFramework
{
    class PaintBrushNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityComponentIdPair;

        virtual void OnIntensityChanged([[maybe_unused]] float radius) { }
        virtual void OnOpacityChanged([[maybe_unused]] float radius) { }
        virtual void OnPaint([[maybe_unused]] const AZ::Aabb& dirtyArea) { }
        virtual void OnRadiusChanged([[maybe_unused]] float radius) { }
        virtual void OnWorldSpaceChanged([[maybe_unused]] AZ::Transform result) { }
    };

    using PaintBrushNotificationBus = AZ::EBus<PaintBrushNotifications>;
} // namespace AzToolsFramework