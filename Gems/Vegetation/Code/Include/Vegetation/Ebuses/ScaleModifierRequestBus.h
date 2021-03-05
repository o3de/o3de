/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <GradientSignal/GradientSampler.h>

namespace Vegetation
{
    class ScaleModifierRequests
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

    using ScaleModifierRequestBus = AZ::EBus<ScaleModifierRequests>;
}