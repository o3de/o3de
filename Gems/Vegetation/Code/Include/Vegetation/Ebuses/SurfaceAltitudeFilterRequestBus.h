/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace Vegetation
{
    class SurfaceAltitudeFilterRequests
        : public AZ::ComponentBus
    {
    public:
        /**
         * Overrides the default AZ::EBusTraits handler policy to allow one
         * listener only.
         */
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual bool GetAllowOverrides() const = 0;
        virtual void SetAllowOverrides(bool value) = 0;

        virtual AZ::EntityId GetShapeEntityId() const = 0;
        virtual void SetShapeEntityId(AZ::EntityId shapeEntityId) = 0;

        virtual float GetAltitudeMin() const = 0;
        virtual void SetAltitudeMin(float altitudeMin) = 0;

        virtual float GetAltitudeMax() const = 0;
        virtual void SetAltitudeMax(float altitudeMax) = 0;
    };

    using SurfaceAltitudeFilterRequestBus = AZ::EBus<SurfaceAltitudeFilterRequests>;
}
