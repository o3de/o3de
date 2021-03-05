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
#include "EditorVisAreaComponentMode.h"

#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Manipulators/LineHoverSelection.h>

namespace Visibility
{
    AZ_CLASS_ALLOCATOR_IMPL(EditorVisAreaComponentMode, AZ::SystemAllocator, 0)

    EditorVisAreaComponentMode::EditorVisAreaComponentMode(
        const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType)
        : EditorBaseComponentMode(entityComponentIdPair, componentType)
    {
        CreateManipulators();

        AZ::TransformNotificationBus::Handler::BusConnect(entityComponentIdPair.GetEntityId());
        EditorVisAreaComponentNotificationBus::Handler::BusConnect(entityComponentIdPair.GetEntityId());
    }

    EditorVisAreaComponentMode::~EditorVisAreaComponentMode()
    {
        EditorVisAreaComponentNotificationBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();

        m_vertexSelection.Destroy();
    }

    void EditorVisAreaComponentMode::CreateManipulators()
    {
        using namespace AzToolsFramework;

        m_vertexSelection.Create(
            GetEntityComponentIdPair(), g_mainManipulatorManagerId,
            AZStd::make_unique<LineSegmentHoverSelection<AZ::Vector3>>(GetEntityComponentIdPair(), g_mainManipulatorManagerId),
            TranslationManipulators::Dimensions::Three,
            ConfigureTranslationManipulatorAppearance3d);

        m_vertexSelection.SetVertexPositionsUpdatedCallback([this]()
        {
            EditorVisAreaComponentRequestBus::Event(
                GetEntityId(), &EditorVisAreaComponentRequests::UpdateVisAreaObject);
        });
    }

    void EditorVisAreaComponentMode::OnTransformChanged(
        const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_vertexSelection.RefreshSpace(world);
    }

    void EditorVisAreaComponentMode::OnVertexAdded(size_t index)
    {
        Refresh();

        AZ::Vector3 vertex;
        bool found = false;
        AZ::FixedVerticesRequestBus<AZ::Vector3>::EventResult(
            found, GetEntityId(), &AZ::FixedVerticesRequestBus<AZ::Vector3>::Handler::GetVertex,
            index, vertex);

        if (found)
        {
            m_vertexSelection.CreateTranslationManipulator(
                GetEntityComponentIdPair(), AzToolsFramework::g_mainManipulatorManagerId,
                vertex, index);
        }
    }

    void EditorVisAreaComponentMode::OnVertexRemoved(size_t /*index*/)
    {
        Refresh();
    }

    void EditorVisAreaComponentMode::OnVerticesSet(const AZStd::vector<AZ::Vector3>& /*vertices*/)
    {
        Refresh();
    }

    void EditorVisAreaComponentMode::OnVerticesCleared()
    {
        Refresh();
    }

    void EditorVisAreaComponentMode::Refresh()
    {
        // destroy and recreate manipulators when container is modified (vertices are added or removed)
        m_vertexSelection.Destroy();
        CreateManipulators();
    }

    AZStd::vector<AzToolsFramework::ActionOverride> EditorVisAreaComponentMode::PopulateActionsImpl()
    {
        return m_vertexSelection.ActionOverrides();
    }

    bool EditorVisAreaComponentMode::HandleMouseInteraction(
        const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        return m_vertexSelection.HandleMouse(mouseInteraction);
    }

    AZStd::string EditorVisAreaComponentMode::GetComponentModeName() const
    {
        return "Vis Area Edit Mode";
    }

} // namespace Visibility
