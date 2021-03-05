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

#include "EditorPortalComponentBus.h"

namespace Visibility
{
    class EditorPortalComponentMode
        : public AzToolsFramework::ComponentModeFramework::EditorBaseComponentMode
        , private AZ::TransformNotificationBus::Handler
        , private EditorPortalNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        EditorPortalComponentMode(
            const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType);
        ~EditorPortalComponentMode();

    private:
        // EditorBaseComponentMode
        void Refresh() override;
        AZStd::vector<AzToolsFramework::ActionOverride> PopulateActionsImpl() override;
        bool HandleMouseInteraction(
            const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction) override;

        // Manipulator handling
        void CreateManipulators();

        // TransformNotificationBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // EditorPortalNotificationBus
        void OnVerticesChangedInspector() override;

        AzToolsFramework::EditorVertexSelectionFixed<AZ::Vector3> m_vertexSelection; ///< Handles all manipulator interactions with vertices.
    };
} // namespace Visibility