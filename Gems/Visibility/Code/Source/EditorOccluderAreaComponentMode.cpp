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

#include "Visibility_precompiled.h"
#include "EditorOccluderAreaComponentMode.h"

#include <AzToolsFramework/Manipulators/ManipulatorManager.h>

namespace Visibility
{
    AZ_CLASS_ALLOCATOR_IMPL(EditorOccluderAreaComponentMode, AZ::SystemAllocator, 0)

    EditorOccluderAreaComponentMode::EditorOccluderAreaComponentMode(
        const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType)
        : EditorBaseComponentMode(entityComponentIdPair, componentType)
    {
        CreateManipulators();
        
        AZ::TransformNotificationBus::Handler::BusConnect(entityComponentIdPair.GetEntityId());
        EditorOccluderAreaNotificationBus::Handler::BusConnect(entityComponentIdPair.GetEntityId());
    }

    EditorOccluderAreaComponentMode::~EditorOccluderAreaComponentMode()
    {
        EditorOccluderAreaNotificationBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        
        m_vertexSelection.Destroy();
    }

    void EditorOccluderAreaComponentMode::CreateManipulators()
    {
        using namespace AzToolsFramework;

        m_vertexSelection.Create(
            AZ::EntityComponentIdPair(GetEntityId(), GetComponentId()), g_mainManipulatorManagerId,
            AZStd::make_unique<NullHoverSelection>(),
            TranslationManipulators::Dimensions::Three,
            ConfigureTranslationManipulatorAppearance3d);

        m_vertexSelection.SetVertexPositionsUpdatedCallback([this]()
        {
            EditorOccluderAreaRequestBus::Event(
                GetEntityId(), &EditorOccluderAreaRequests::UpdateOccluderAreaObject);
        });
    }

    void EditorOccluderAreaComponentMode::OnTransformChanged(
        const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_vertexSelection.RefreshSpace(world);
    }

    void EditorOccluderAreaComponentMode::OnVerticesChangedInspector()
    {
        m_vertexSelection.RefreshLocal();
    }

    void EditorOccluderAreaComponentMode::Refresh()
    {
        // destroy and recreate manipulators when container is modified (vertices are added or removed)
        m_vertexSelection.Destroy();
        CreateManipulators();
    }

    AZStd::vector<AzToolsFramework::ActionOverride> EditorOccluderAreaComponentMode::PopulateActionsImpl()
    {
        return m_vertexSelection.ActionOverrides();
    }

    bool EditorOccluderAreaComponentMode::HandleMouseInteraction(
        const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        return m_vertexSelection.HandleMouse(mouseInteraction);
    }
} // namespace Visibility