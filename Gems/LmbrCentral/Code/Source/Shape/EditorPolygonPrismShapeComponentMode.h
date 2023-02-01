/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/NonUniformScaleBus.h>
#include <AzToolsFramework/ComponentMode/EditorBaseComponentMode.h>
#include <AzToolsFramework/Manipulators/EditorVertexSelection.h>
#include <LmbrCentral/Shape/PolygonPrismShapeComponentBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

namespace AzToolsFramework
{
    class LinearManipulator;
}

namespace LmbrCentral
{
    /// The specific ComponentMode responsible for handling polygon prism editing.
    class EditorPolygonPrismShapeComponentMode
        : public AzToolsFramework::ComponentModeFramework::EditorBaseComponentMode
        , private AZ::TransformNotificationBus::Handler
        , private PolygonPrismShapeComponentNotificationBus::Handler
        , private ShapeComponentNotificationsBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL
        AZ_RTTI(EditorPolygonPrismShapeComponentMode, "{010CC49A-477A-4F1A-812F-60F7C4E420D5}", EditorBaseComponentMode)

        static void Reflect(AZ::ReflectContext* context);

        EditorPolygonPrismShapeComponentMode(
            const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType);
        ~EditorPolygonPrismShapeComponentMode();

    private:
        // EditorBaseComponentMode
        void Refresh() override;
        AZStd::vector<AzToolsFramework::ActionOverride> PopulateActionsImpl() override;
        bool HandleMouseInteraction(
            const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction) override;
        AZStd::string GetComponentModeName() const override;
        AZ::Uuid GetComponentModeType() const override;

        // Manipulator handling
        void CreateManipulators();
        void DestroyManipulators();
        void RefreshManipulators();
        void ContainerChanged();

        // PolygonPrismShapeComponentNotificationBus
        void OnVertexAdded(size_t index) override;
        void OnVertexRemoved(size_t index) override;
        void OnVerticesSet(const AZStd::vector<AZ::Vector2>& vertices) override;
        void OnVerticesCleared() override;

        // ShapeComponentNotificationsBus
        void OnShapeChanged(ShapeChangeReasons changeReason) override;

        // TransformNotificationBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        void OnNonUniformScaleChanged(const AZ::Vector3& scale);

        AZ::Transform m_currentTransform;
        AZ::Vector3 m_currentNonUniformScale;
        AzToolsFramework::EditorVertexSelectionVariable<AZ::Vector2> m_vertexSelection; ///< Handles all manipulator interactions with vertices (inserting and translating).
        AZStd::shared_ptr<AzToolsFramework::LinearManipulator> m_heightManipulator; ///< Manipulator to control the height of the polygon prism.
        AZ::NonUniformScaleChangedEvent::Handler m_nonUniformScaleChangedHandler; ///< Responds to changes in non-uniform scale.
    };
} // namespace LmbrCentral
