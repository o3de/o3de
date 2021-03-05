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
    class DitherGradientRequests
        : public AZ::ComponentBus
    {
    public:
        /**
         * Overrides the default AZ::EBusTraits handler policy to allow one
         * listener only.
         */
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual bool GetUseSystemPointsPerUnit() const = 0;
        virtual void SetUseSystemPointsPerUnit(bool value) = 0;

        virtual float GetPointsPerUnit() const = 0;
        virtual void SetPointsPerUnit(float points) = 0;

        virtual AZ::Vector3 GetPatternOffset() const = 0;
        virtual void SetPatternOffset(AZ::Vector3 offset) = 0;

        virtual AZ::u8 GetPatternType() const = 0;
        virtual void SetPatternType(AZ::u8 type) = 0;

        virtual GradientSampler& GetGradientSampler() = 0;
    };

    using DitherGradientRequestBus = AZ::EBus<DitherGradientRequests>;
}