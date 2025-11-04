/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "EditorBaseShapeComponent.h"
#include "TubeShapeComponent.h"

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/ComponentMode/ComponentModeDelegate.h>
#include <LmbrCentral/Shape/SplineComponentBus.h>
#include <LmbrCentral/Shape/EditorTubeShapeComponentBus.h>

namespace LmbrCentral
{
    /// Editor representation of a tube shape.
    class EditorTubeShapeComponent
        : public EditorBaseShapeComponent
        , private AzFramework::EntityDebugDisplayEventBus::Handler
        , private SplineComponentNotificationBus::Handler
        , private SplineAttributeNotificationBus::Handler
        , private EditorTubeShapeComponentRequestBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(EditorTubeShapeComponent, EditorTubeShapeComponentTypeId, EditorBaseShapeComponent);
        static void Reflect(AZ::ReflectContext* context);

        EditorTubeShapeComponent() = default;

        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;

    protected:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            EditorBaseShapeComponent::GetProvidedServices(provided);
            provided.push_back(AZ_CRC_CE("TubeShapeService"));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            EditorBaseShapeComponent::GetRequiredServices(required);
            required.push_back(AZ_CRC_CE("SplineService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
    private:
        AZ_DISABLE_COPY_MOVE(EditorTubeShapeComponent)

        // AzFramework::EntityDebugDisplayEventBus
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        // SplineComponentNotificationBus
        void OnSplineChanged() override;

        // SplineAttributeNotificationBus
        void OnAttributeAdded(size_t index) override;
        void OnAttributeRemoved(size_t index) override;
        void OnAttributesSet(size_t size) override;
        void OnAttributesCleared() override;

        // EditorTubeShapeComponentRequestBus
        void GenerateVertices() override;

        void ConfigurationChanged();

        TubeShape m_tubeShape; ///< Underlying tube shape.
        TubeShapeMeshConfig m_tubeShapeMeshConfig; ///< Configuration to control how the TubeShape should look.
        ShapeMesh m_tubeShapeMesh; ///< Buffer to hold index and vertex data for TubeShape when drawing.

        using ComponentModeDelegate = AzToolsFramework::ComponentModeFramework::ComponentModeDelegate;
        ComponentModeDelegate m_componentModeDelegate; ///< Responsible for detecting ComponentMode activation 
                                                       ///< and creating a concrete ComponentMode(s).
    };
} // namespace LmbrCentral
