/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "EditorBaseShapeComponent.h"
#include "PolygonPrismShape.h"

#include <AzCore/Math/VertexContainerInterface.h>
#include <AzToolsFramework/ComponentMode/ComponentModeDelegate.h>
#include <LmbrCentral/Shape/EditorPolygonPrismShapeComponentBus.h>

namespace LmbrCentral
{
     /// Editor representation for the PolygonPrismShapeComponent.
     /// Visualizes the Polygon Prism in the editor - an extruded polygon.
    class EditorPolygonPrismShapeComponent
        : public EditorBaseShapeComponent
        , private AzFramework::EntityDebugDisplayEventBus::Handler
        , private EditorPolygonPrismShapeComponentRequestsBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(EditorPolygonPrismShapeComponent, EditorPolygonPrismShapeComponentTypeId, EditorBaseShapeComponent);
        static void Reflect(AZ::ReflectContext* context);

        EditorPolygonPrismShapeComponent() = default;

    private:
        AZ_DISABLE_COPY_MOVE(EditorPolygonPrismShapeComponent)

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;

        // AzFramework::EntityDebugDisplayEventBus
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        // EditorPolygonPrismShapeComponentRequestsBus
        void GenerateVertices() override;
        
        PolygonPrismShape m_polygonPrismShape; ///< Stores configuration data of a polygon prism for this component.
        PolygonPrismMesh m_polygonPrismMesh; ///< Buffer to store triangles of top and bottom of Polygon Prism.
        PolygonPrismShapeConfig m_polygonShapeConfig; ///< Configuration for polygon prism shape.

        using ComponentModeDelegate = AzToolsFramework::ComponentModeFramework::ComponentModeDelegate;
        ComponentModeDelegate m_componentModeDelegate; ///< Responsible for detecting ComponentMode activation
                                                       ///< and creating a concrete ComponentMode.
    };
} // namespace LmbrCentral
