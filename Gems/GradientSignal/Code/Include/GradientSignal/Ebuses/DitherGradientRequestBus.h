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
