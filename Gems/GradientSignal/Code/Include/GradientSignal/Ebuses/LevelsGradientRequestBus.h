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