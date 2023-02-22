/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/PolygonPrism.h>
#include <AzCore/Math/VertexContainerInterface.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

namespace LmbrCentral
{
    /**
    * Type ID for the EditorPolygonPrismShapeComponent
    */
    inline constexpr AZ::TypeId EditorPolygonPrismShapeComponentTypeId{ "{5368F204-FE6D-45C0-9A4F-0F933D90A785}" };

    /// Services provided by the Polygon Prism Shape Component.
    class PolygonPrismShapeComponentRequests
        : public AZ::VariableVertices<AZ::Vector2>
    {
    public:
        /// Returns a reference to the underlying polygon prism.
        virtual AZ::PolygonPrismPtr GetPolygonPrism() = 0;

        /// Sets height of polygon shape.
        /// @param height The height of the polygon shape.
        virtual void SetHeight(float height) = 0;

    protected:
        ~PolygonPrismShapeComponentRequests() = default;
    };

    /// Bus to service the Polygon Prism Shape component event group.
    using PolygonPrismShapeComponentRequestBus = AZ::EBus<PolygonPrismShapeComponentRequests, AZ::ComponentBus>;

    /// Listener for polygon prism changes.
    class PolygonPrismShapeComponentNotification
        : public AZ::ComponentBus
        , public AZ::VertexContainerNotificationInterface<AZ::Vector2>
    {
    public:
        /// Called when a new vertex is added to polygon prism.
        void OnVertexAdded(size_t /*index*/) override {}

        /// Called when a vertex is removed from polygon prism.
        void OnVertexRemoved(size_t /*index*/) override {}

        /// Called when a vertex is updated.
        void OnVertexUpdated(size_t /*index*/) override {}

        /// Called when all vertices on the polygon prism are set.
        void OnVerticesSet(const AZStd::vector<AZ::Vector2>& /*vertices*/) override {}

        /// Called when all vertices from the polygon prism are removed.
        void OnVerticesCleared() override {}

    protected:
        ~PolygonPrismShapeComponentNotification() = default;
    };

    /// Bus to service the polygon prism shape component notification group.
    using PolygonPrismShapeComponentNotificationBus = AZ::EBus<PolygonPrismShapeComponentNotification>;

    class PolygonPrismShapeConfig
        : public ShapeComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(PolygonPrismShapeConfig, AZ::SystemAllocator)
        AZ_RTTI(PolygonPrismShapeConfig, "{84CAA5E4-45EB-4CFF-BC41-EE0FDC0F095C}", ShapeComponentConfig)

        static void Reflect(AZ::ReflectContext* context);

        PolygonPrismShapeConfig() = default;
    };

} // namespace LmbrCentral
