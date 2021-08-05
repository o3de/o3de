/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/ComponentBus.h>

namespace Vegetation
{
    class ShapeIntersectionFilterRequests
        : public AZ::ComponentBus
    {
    public:
        /**
         * Overrides the default AZ::EBusTraits handler policy to allow one
         * listener only.
         */
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual AZ::EntityId GetShapeEntityId() const = 0;
        virtual void SetShapeEntityId(AZ::EntityId shapeEntityId) = 0;
    };

    using ShapeIntersectionFilterRequestBus = AZ::EBus<ShapeIntersectionFilterRequests>;
}
