/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <GradientSignal/GradientSampler.h>

namespace Vegetation
{
    class RotationModifierRequests
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

        virtual AZ::Vector3 GetRangeMin() const = 0;
        virtual void SetRangeMin(AZ::Vector3 rangeMin) = 0;

        virtual AZ::Vector3 GetRangeMax() const = 0;
        virtual void SetRangeMax(AZ::Vector3 rangeMax) = 0;

        virtual GradientSignal::GradientSampler& GetGradientSamplerX() = 0;
        virtual GradientSignal::GradientSampler& GetGradientSamplerY() = 0;
        virtual GradientSignal::GradientSampler& GetGradientSamplerZ() = 0;
    };

    using RotationModifierRequestBus = AZ::EBus<RotationModifierRequests>;
}
