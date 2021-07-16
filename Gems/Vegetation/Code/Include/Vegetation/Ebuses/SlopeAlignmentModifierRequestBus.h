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
    class SlopeAlignmentModifierRequests
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

        virtual float GetRangeMin() const = 0;
        virtual void SetRangeMin(float rangeMin) = 0;

        virtual float GetRangeMax() const = 0;
        virtual void SetRangeMax(float rangeMax) = 0;

        virtual GradientSignal::GradientSampler& GetGradientSampler() = 0;
    };

    using SlopeAlignmentModifierRequestBus = AZ::EBus<SlopeAlignmentModifierRequests>;
}
