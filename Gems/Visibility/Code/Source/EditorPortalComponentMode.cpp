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
#include "EditorPortalComponentMode.h"

#include <AzToolsFramework/Manipulators/ManipulatorManager.h>

namespace Visibility
{
    AZ_CLASS_ALLOCATOR_IMPL(EditorPortalComponentMode, AZ::SystemAllocator, 0)

    EditorPortalComponentMode::EditorPortalComponentMode(
        const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType)
        : EditorBaseComponentMode(entityComponentIdPair, componentType)
    {
        CreateManipulators();

        AZ::TransformNotificationBus::Handler::BusConnect(entityComponentIdPair.GetEntityId());
        EditorPortalNotificationBus::Handler::BusConnect(entityComponentIdPair.GetEntityId());
    }

    EditorPortalComponentMode::~EditorPortalComponentMode()
    {
        EditorPortalNotificationBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();

        m_vertexSelection.Destroy();
    }

    void EditorPortalComponentMode::CreateManipulators()
    {
        using namespace AzToolsFramework;

        m_vertexSelection.Create(
            AZ::EntityComponentIdPair(GetEntityId(), GetComponentId()), g_mainManipulatorManagerId,
            AZStd::make_unique<NullHoverSelection>(),
            TranslationManipulators::Dimensions::Three,
            ConfigureTranslationManipulatorAppearance3d);

        m_vertexSelection.SetVertexPositionsUpdatedCallback([this]()
        {
            EditorPortalRequestBus::Event(
                GetEntityId(), &EditorPortalRequests::UpdatePortalObject);
        });
    }

    void EditorPortalComponentMode::OnTransformChanged(
        const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_vertexSelection.RefreshSpace(world);
    }

    void EditorPortalComponentMode::OnVerticesChangedInspector()
    {
        m_vertexSelection.RefreshLocal();
    }

    void EditorPortalComponentMode::Refresh()
    {
        // destroy and recreate manipulators when container is modified (vertices are added or removed)
        m_vertexSelection.Destroy();
        CreateManipulators();
    }

    AZStd::vector<AzToolsFramework::ActionOverride> EditorPortalComponentMode::PopulateActionsImpl()
    {
        return m_vertexSelection.ActionOverrides();
    }

    bool EditorPortalComponentMode::HandleMouseInteraction(
        const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        return m_vertexSelection.HandleMouse(mouseInteraction);
    }
} // namespace Visibility