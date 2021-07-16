/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <GradientSignal/GradientSampler.h>

namespace Vegetation
{
    class DistributionFilterRequests
        : public AZ::ComponentBus
    {
    public:
        /**
         * Overrides the default AZ::EBusTraits handler policy to allow one
         * listener only.
         */
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual float GetThresholdMin() const = 0;
        virtual void SetThresholdMin(float thresholdMin) = 0;

        virtual float GetThresholdMax() const = 0;
        virtual void SetThresholdMax(float thresholdMax) = 0;

        virtual GradientSignal::GradientSampler& GetGradientSampler() = 0;
    };

    using DistributionFilterRequestBus = AZ::EBus<DistributionFilterRequests>;
}
