/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <GradientSignal/GradientSampler.h>

namespace GradientSignal
{
    class GradientSurfaceDataRequests
        : public AZ::ComponentBus
    {
    public:
        /**
         * Overrides the default AZ::EBusTraits handler policy to allow one
         * listener only.
         */
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual void SetThresholdMin(float thresholdMin) = 0;
        virtual float GetThresholdMin() const = 0;
        virtual void SetThresholdMax(float thresholdMax) = 0;
        virtual float GetThresholdMax() const = 0;

        virtual size_t GetNumTags() const = 0;
        virtual AZ::Crc32 GetTag(int tagIndex) const = 0;
        virtual void RemoveTag(int tagIndex) = 0;
        virtual void AddTag(AZStd::string tag) = 0;

        virtual AZ::EntityId GetShapeConstraintEntityId() const = 0;
        virtual void SetShapeConstraintEntityId(AZ::EntityId entityId) = 0;
    };

    using GradientSurfaceDataRequestBus = AZ::EBus<GradientSurfaceDataRequests>;
}
