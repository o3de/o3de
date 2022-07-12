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
    enum class FalloffType : AZ::u8
    {
        Inner = 0,
        Outer,
        InnerOuter,
    };

    class ShapeAreaFalloffGradientRequests
        : public AZ::ComponentBus
    {
    public:
        /**
         * Overrides the default AZ::EBusTraits handler policy to allow one
         * listener only.
         */
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual AZ::EntityId GetShapeEntityId() const = 0;
        virtual void SetShapeEntityId(AZ::EntityId entityId) = 0;

        virtual float GetFalloffWidth() const = 0;
        virtual void SetFalloffWidth(float falloffWidth) = 0;

        virtual FalloffType GetFalloffType() const = 0;
        virtual void SetFalloffType(FalloffType type) = 0;

        virtual bool Get3dFalloff() const = 0;
        virtual void Set3dFalloff(bool is3dFalloff) = 0;
    };

    using ShapeAreaFalloffGradientRequestBus = AZ::EBus<ShapeAreaFalloffGradientRequests>;
}
