/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Obb.h>
#include <AzCore/Component/NonUniformScaleBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include "BoxShape.h"

namespace AzFramework
{
    class DebugDisplayRequests;
}

namespace LmbrCentral
{
    struct ShapeDrawParams;

    class AxisAlignedBoxShape
        : public BoxShape
    {
    public:
        AZ_CLASS_ALLOCATOR(AxisAlignedBoxShape, AZ::SystemAllocator)
        AZ_RTTI(AxisAlignedBoxShape, "{CFDC96C5-287A-4033-8D7D-BA9331C13F25}", BoxShape)

        AxisAlignedBoxShape();

        static void Reflect(AZ::ReflectContext* context);

        void Activate(AZ::EntityId entityId) override;

        // AZ::TransformNotificationBus::Handler
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // BoxShapeComponentRequestBus::Handler overrides ...
        bool IsTypeAxisAligned() const override;
    };
} // namespace LmbrCentral
