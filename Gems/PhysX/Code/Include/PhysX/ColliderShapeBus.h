/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace PhysX
{
    /// Messages serviced by a PhysX collider component's shape.
    /// A PhysX collider component shape represents the 3D geometry of a collider.
    class ColliderShapeRequests
        : public AZ::ComponentBus
    {
    public:
        /// Gets the world space AABB of the collider's shape.
        virtual AZ::Aabb GetColliderShapeAabb() = 0;

        /// Checks if this collider shape is a trigger.
        virtual bool IsTrigger() = 0;
    };
    using ColliderShapeRequestBus = AZ::EBus<ColliderShapeRequests>;
}
