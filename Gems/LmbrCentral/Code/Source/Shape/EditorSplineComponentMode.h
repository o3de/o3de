/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TransformBus.h>
#include <AzToolsFramework/ComponentMode/EditorBaseComponentMode.h>
#include <AzToolsFramework/Manipulators/EditorVertexSelection.h>
#include <LmbrCentral/Shape/EditorSplineComponentBus.h>
#include <LmbrCentral/Shape/SplineComponentBus.h>

namespace LmbrCentral
{
     /// The specific ComponentMode responsible for handling Spline Component editing.
    class EditorSplineComponentMode
        : public AzToolsFramework::ComponentModeFramework::EditorBaseComponentMode
        , private SplineComponentNotificationBus::Handler
        , private AZ::TransformNotificationBus::Handler
        , private EditorSplineComponentNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL
        AZ_RTTI(EditorSplineComponentMode, "{B4D50765-501D-45FF-B934-198386A806E6}", EditorBaseComponentMode)

        EditorSplineComponentMode(
            const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType);
        ~EditorSplineComponentMode();

        static void Reflect(AZ::ReflectContext* context);

        static void RegisterActions();
        static void BindActionsToModes();
        static void BindActionsToMenus();

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
        void ContainerChanged();

        // SplineComponentNotificationBus
        void OnSplineChanged() override;
        void OnVertexAdded(size_t index) override;
        void OnVertexRemoved(size_t index) override;
        void OnVertexUpdated(size_t index) override;
        void OnVerticesSet(const AZStd::vector<AZ::Vector3>& vertices) override;
        void OnVerticesCleared() override;

        // EditorSplineComponentNotificationBus
        void OnSplineTypeChanged() override;

        // TransformNotificationBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        AzToolsFramework::EditorVertexSelectionVariable<AZ::Vector3> m_vertexSelection; ///< Handles all manipulator interactions with vertices (inserting and translating).
    };
} // namespace LmbrCentral
