/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

        EditorSplineComponentMode(
            const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType);
        ~EditorSplineComponentMode();

    private:
        // EditorBaseComponentMode
        void Refresh() override;
        AZStd::vector<AzToolsFramework::ActionOverride> PopulateActionsImpl() override;
        bool HandleMouseInteraction(
            const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction) override;
        AZStd::string GetComponentModeName() const override;

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
