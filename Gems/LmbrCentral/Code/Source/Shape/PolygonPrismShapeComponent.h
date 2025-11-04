/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Math/PolygonPrism.h>

#include "PolygonPrismShape.h"
#include "Rendering/EntityDebugDisplayComponent.h"

namespace LmbrCentral
{
    /// Component interface for Polygon Prism.
    /// Formal Definition: A polygonal prism is a 3-dimensional prism made from two translated polygons connected by rectangles.
    /// Here the representation is defined by one polygon (internally represented as a vertex container - list of vertices) and a height (extrusion) property.
    /// All points lie on the local plane Z = 0.
    class PolygonPrismShapeComponent
        : public AZ::Component
    {
    public:
        friend class EditorPolygonPrismShapeComponent;

        AZ_COMPONENT(PolygonPrismShapeComponent, "{AD882674-1D5D-4E40-B079-449B47D2492C}");

        PolygonPrismShapeComponent() = default;

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

    protected:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("ShapeService"));
            provided.push_back(AZ_CRC_CE("PolygonPrismShapeService"));
            provided.push_back(AZ_CRC_CE("VariableVertexContainerService"));
            provided.push_back(AZ_CRC_CE("FixedVertexContainerService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("ShapeService"));
            incompatible.push_back(AZ_CRC_CE("PolygonPrismShapeService"));
            incompatible.push_back(AZ_CRC_CE("VariableVertexContainerService"));
            incompatible.push_back(AZ_CRC_CE("FixedVertexContainerService"));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(AZ_CRC_CE("NonUniformScaleService"));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("TransformService"));
        }

        static void Reflect(AZ::ReflectContext* context);

    private:
        PolygonPrismShape m_polygonPrismShape; ///< Stores configuration of a Polygon Prism Shape for this component
    };

    /// Concrete EntityDebugDisplay implementation for PolygonPrismShape.
    class PolygonPrismShapeDebugDisplayComponent
        : public EntityDebugDisplayComponent
        , public ShapeComponentNotificationsBus::Handler
    {
    public:
        AZ_COMPONENT(PolygonPrismShapeDebugDisplayComponent, "{FBDABBAB-F754-4637-BF26-9AB89F3AF626}")

        PolygonPrismShapeDebugDisplayComponent() = default;
        explicit PolygonPrismShapeDebugDisplayComponent(const AZ::PolygonPrism& polygonPrism)
            : m_polygonPrism(polygonPrism) {}

        void SetShapeConfig(const PolygonPrismShapeConfig& shapeConfig);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        static void Reflect(AZ::ReflectContext* context);

        void Draw(AzFramework::DebugDisplayRequests& debugDisplay) override;

        // ShapeComponentNotificationsBus
        void OnShapeChanged(ShapeChangeReasons changeReason) override;

    private:
        AZ_DISABLE_COPY_MOVE(PolygonPrismShapeDebugDisplayComponent)

        void GenerateVertices();

        AZ::PolygonPrism m_polygonPrism; ///< Stores configuration data for PolygonPrism shape.
        PolygonPrismMesh m_polygonPrismMesh; ///< Buffer to store triangles of top and bottom of Polygon Prism.

        PolygonPrismShapeConfig m_polygonShapeConfig;
    };
} // namespace LmbrCentral
