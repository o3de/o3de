/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorSplineComponentMode.h"

#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/Manipulators/SplineHoverSelection.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>

namespace LmbrCentral
{
    AZ_CLASS_ALLOCATOR_IMPL(EditorSplineComponentMode, AZ::SystemAllocator)

    EditorSplineComponentMode::EditorSplineComponentMode(
        const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid componentType)
        : EditorBaseComponentMode(entityComponentIdPair, componentType)
        , m_vertexSelection(entityComponentIdPair)
    {
        AZ::TransformNotificationBus::Handler::BusConnect(entityComponentIdPair.GetEntityId());
        SplineComponentNotificationBus::Handler::BusConnect(entityComponentIdPair.GetEntityId());
        EditorSplineComponentNotificationBus::Handler::BusConnect(entityComponentIdPair.GetEntityId());

        CreateManipulators();
    }

    EditorSplineComponentMode::~EditorSplineComponentMode()
    {
        EditorSplineComponentNotificationBus::Handler::BusDisconnect();
        SplineComponentNotificationBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();

        m_vertexSelection.Destroy();
    }

    void EditorSplineComponentMode::Reflect(AZ::ReflectContext* context)
    {
        AzToolsFramework::ComponentModeFramework::ReflectEditorBaseComponentModeDescendant<EditorSplineComponentMode>(context);
    }

    void EditorSplineComponentMode::RegisterActions()
    {
        AzToolsFramework::EditorVertexSelectionActionManagement::RegisterEditorVertexSelectionActions();
    }

    void EditorSplineComponentMode::BindActionsToModes()
    {
        auto actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
        AZ_Assert(actionManagerInterface, "EditorSplineComponentMode - could not get ActionManagerInterface on RegisterActions.");

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        
        AZStd::string modeIdentifier = AZStd::string::format(
            "o3de.context.mode.%s", serializeContext->FindClassData(azrtti_typeid<EditorSplineComponentMode>())->m_name);

        actionManagerInterface->AssignModeToAction(modeIdentifier, "o3de.action.vertexSelection.duplicate");
        actionManagerInterface->AssignModeToAction(modeIdentifier, "o3de.action.vertexSelection.delete");
        actionManagerInterface->AssignModeToAction(modeIdentifier, "o3de.action.vertexSelection.clearSelection");
    }

    void EditorSplineComponentMode::BindActionsToMenus()
    {
        AzToolsFramework::EditorVertexSelectionActionManagement::BindEditorVertexSelectionActionsToMenus();
    }

    void EditorSplineComponentMode::Refresh()
    {
        ContainerChanged();
    }

    AZStd::vector<AzToolsFramework::ActionOverride> EditorSplineComponentMode::PopulateActionsImpl()
    {
        return m_vertexSelection.ActionOverrides();
    }

    bool EditorSplineComponentMode::HandleMouseInteraction(
        const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        return m_vertexSelection.HandleMouse(mouseInteraction);
    }

    AZStd::string EditorSplineComponentMode::GetComponentModeName() const
    {
        return "Spline Edit Mode";
    }

    AZ::Uuid EditorSplineComponentMode::GetComponentModeType() const
    {
        return azrtti_typeid<EditorSplineComponentMode>();
    }

    void EditorSplineComponentMode::CreateManipulators()
    {
        using namespace AzToolsFramework;

        bool empty = false;
        SplineComponentRequestBus::EventResult(
            empty, GetEntityId(), &SplineComponentRequests::Empty);

        // if we have no vertices, do not attempt to create any manipulators
        if (empty)
        {
            return;
        }

        AZ::SplinePtr spline = nullptr;
        SplineComponentRequestBus::EventResult(
            spline, GetEntityId(), &SplineComponentRequests::GetSpline);

        m_vertexSelection.Create(
            GetEntityComponentIdPair(), g_mainManipulatorManagerId,
            AZStd::make_unique<SplineHoverSelection>(
                GetEntityComponentIdPair(), g_mainManipulatorManagerId, spline),
            TranslationManipulators::Dimensions::Three, ConfigureTranslationManipulatorAppearance3d);
    }

    void EditorSplineComponentMode::ContainerChanged()
    {
        // destroy and recreate manipulators when container is modified (vertices are added or removed)
        m_vertexSelection.Destroy();
        CreateManipulators();
    }

    void EditorSplineComponentMode::OnSplineChanged()
    {
        m_vertexSelection.RefreshLocal();
    }

    void EditorSplineComponentMode::OnVertexAdded(size_t index)
    {
        ContainerChanged();

        AZ::SplinePtr spline = nullptr;
        SplineComponentRequestBus::EventResult(
            spline, GetEntityId(), &SplineComponentRequests::GetSpline);

        m_vertexSelection.CreateTranslationManipulator(
            GetEntityComponentIdPair(), AzToolsFramework::g_mainManipulatorManagerId,
            spline->m_vertexContainer.GetVertices()[index], index);
    }

    void EditorSplineComponentMode::OnVertexRemoved(size_t /*index*/)
    {
        ContainerChanged();
    }

    void EditorSplineComponentMode::OnVertexUpdated(size_t /*index*/)
    {
        m_vertexSelection.RefreshLocal();
    }

    void EditorSplineComponentMode::OnVerticesSet(const AZStd::vector<AZ::Vector3>& /*vertices*/)
    {
        ContainerChanged();
    }

    void EditorSplineComponentMode::OnVerticesCleared()
    {
        ContainerChanged();
    }

    void EditorSplineComponentMode::OnTransformChanged(
        const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        // update the space manipulators are in after the entity has moved
        m_vertexSelection.RefreshSpace(world);
    }

    void EditorSplineComponentMode::OnSplineTypeChanged()
    {
        ContainerChanged();
    }
} // namespace LmbrCentral
