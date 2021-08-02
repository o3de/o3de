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

namespace GradientSignal
{
    class LevelsGradientRequests
        : public AZ::ComponentBus
    {
    public:
        /**
         * Overrides the default AZ::EBusTraits handler policy to allow one
         * listener only.
         */
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual float GetInputMin() const = 0;
        virtual void SetInputMin(float value) = 0;

        virtual float GetInputMid() const = 0;
        virtual void SetInputMid(float value) = 0;

        virtual float GetInputMax() const = 0;
        virtual void SetInputMax(float value) = 0;

        virtual float GetOutputMin() const = 0;
        virtual void SetOutputMin(float value) = 0;

        virtual float GetOutputMax() const = 0;
        virtual void SetOutputMax(float value) = 0;

        virtual GradientSampler& GetGradientSampler() = 0;
    };

    using LevelsGradientRequestBus = AZ::EBus<LevelsGradientRequests>;
}
