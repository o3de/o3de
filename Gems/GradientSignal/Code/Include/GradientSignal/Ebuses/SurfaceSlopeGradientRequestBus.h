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
    class SurfaceSlopeGradientRequests
        : public AZ::ComponentBus
    {
    public:
        /**
         * Overrides the default AZ::EBusTraits handler policy to allow one
         * listener only.
         */
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual float GetSlopeMin() const = 0;
        virtual void SetSlopeMin(float slopeMin) = 0;

        virtual float GetSlopeMax() const = 0;
        virtual void SetSlopeMax(float slopeMax) = 0;

        virtual size_t GetNumTags() const = 0;
        virtual AZ::Crc32 GetTag(int tagIndex) const = 0;
        virtual void RemoveTag(int tagIndex) = 0;
        virtual void AddTag(AZStd::string tag) = 0;

        virtual AZ::u8 GetRampType() const = 0;
        virtual void SetRampType(AZ::u8 rampType) = 0;
    };

    using SurfaceSlopeGradientRequestBus = AZ::EBus<SurfaceSlopeGradientRequests>;
}
