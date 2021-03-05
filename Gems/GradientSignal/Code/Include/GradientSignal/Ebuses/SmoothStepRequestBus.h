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