/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace GradientSignal
{
    class SmoothStepRequests
        : public AZ::ComponentBus
    {
    public:
        /**
         * Overrides the default AZ::EBusTraits handler policy to allow one
         * listener only.
         */
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual float GetFallOffRange() const = 0;
        virtual void SetFallOffRange(float range) = 0;

        virtual float GetFallOffStrength() const = 0;
        virtual void SetFallOffStrength(float strength) = 0;

        virtual float GetFallOffMidpoint() const = 0;
        virtual void SetFallOffMidpoint(float midpoint) = 0;
    };

    using SmoothStepRequestBus = AZ::EBus<SmoothStepRequests>;
}
