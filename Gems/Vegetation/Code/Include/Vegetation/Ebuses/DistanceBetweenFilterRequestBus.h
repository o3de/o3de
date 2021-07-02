/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace Vegetation
{
    class DistanceBetweenFilterRequests
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

        virtual BoundMode GetBoundMode() const = 0;
        virtual void SetBoundMode(BoundMode boundMode) = 0;

        virtual float GetRadiusMin() const = 0;
        virtual void SetRadiusMin(float radiusMin) = 0;
    };

    using DistanceBetweenFilterRequestBus = AZ::EBus<DistanceBetweenFilterRequests>;
}
