/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorPickEntitySelection.h"

#include <AzToolsFramework/API/ViewportEditorModeTrackerInterface.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <QApplication>

namespace AzToolsFramework
{
    AZ_CLASS_ALLOCATOR_IMPL(EditorPickEntitySelectionHelper, AZ::SystemAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(EditorPickEntitySelection, AZ::SystemAllocator)

    EditorPickEntitySelectionHelper::EditorPickEntitySelectionHelper(
        const EditorVisibleEntityDataCacheInterface* entityDataCache, ViewportEditorModeTrackerInterface* viewportEditorModeTracker)
        : m_editorHelpers(AZStd::make_unique<EditorHelpers>(entityDataCache))
        , m_viewportEditorModeTracker(viewportEditorModeTracker)
    {
        m_viewportEditorModeTracker->ActivateMode({ GetEntityContextId() }, ViewportEditorMode::Pick);
    }

    EditorPickEntitySelectionHelper::~EditorPickEntitySelectionHelper()
    {
        if (m_hoveredEntityId.IsValid())
        {
            ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetEntityHighlighted, m_hoveredEntityId, false);
        }

        m_viewportEditorModeTracker->DeactivateMode({ GetEntityContextId() }, ViewportEditorMode::Pick);
    }

    // note: m_cachedEntityIdUnderCursor is the authoritative entityId we get each frame by querying
    // HandleMouseInteraction on EditorHelpers, m_hoveredEntityId is what was under the cursor
    // the previous frame. We need to be able to notify the entity the hover/mouse just left and
    // that it should no longer be highlighted, or that a hover just started, so it should be
    // highlighted - m_hoveredEntityId is updated based on the change in m_cachedEntityIdUnderCursor.
    void EditorPickEntitySelectionHelper::HighlightSelectedEntity()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        auto mouseButtons = ViewportInteraction::BuildMouseButtons(QGuiApplication::mouseButtons());

        const bool invalidMouseButtonHeld = mouseButtons.Middle() || mouseButtons.Right();

        // we were hovering, but our new entity id has changed (or we're using the mouse to perform
        // an action unrelated to selection - e.g. moving the camera). In this case we should remove
        // the highlight as it's no longer valid, and clear the hovered entity id
        if ((m_hoveredEntityId.IsValid() && m_hoveredEntityId != m_cachedEntityIdUnderCursor) || invalidMouseButtonHeld)
        {
            if (m_hoveredEntityId.IsValid())
            {
                ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetEntityHighlighted, m_hoveredEntityId, false);

                m_hoveredEntityId.SetInvalid();
            }
        }

        // if we are using the mouse in a selection context and the entity id under the cursor is valid,
        // set it to be highlighted and update the hovered entity id
        if (!invalidMouseButtonHeld)
        {
            if (m_cachedEntityIdUnderCursor.IsValid() && m_cachedEntityIdUnderCursor != m_hoveredEntityId)
            {
                ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetEntityHighlighted, m_cachedEntityIdUnderCursor, true);

                m_hoveredEntityId = m_cachedEntityIdUnderCursor;
            }
        }
    }

    void EditorPickEntitySelectionHelper::DisplayEditorHelpers(
        const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        const AzFramework::CameraState cameraState = GetCameraState(viewportInfo.m_viewportId);
        m_editorHelpers->DisplayHelpers(viewportInfo, cameraState, debugDisplay, [](AZ::EntityId) { return true; });
    }

    bool EditorPickEntitySelectionHelper::HandleMouseViewportInteraction(
        const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        const int viewportId = mouseInteraction.m_mouseInteraction.m_interactionId.m_viewportId;

        const AzFramework::CameraState cameraState = GetCameraState(viewportId);

        m_cachedEntityIdUnderCursor = m_editorHelpers->FindEntityIdUnderCursor(cameraState, mouseInteraction).ContainerAncestorEntityId();

        // when left clicking, if we successfully clicked an entity, assign that
        // to the entity field selected in the entity inspector (RPE)
        if (mouseInteraction.m_mouseInteraction.m_mouseButtons.Left() &&
            mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::Down)
        {
            if (m_cachedEntityIdUnderCursor.IsValid())
            {
                // if we clicked on a valid entity id, actually try to set it
                EditorPickModeRequestBus::Broadcast(&EditorPickModeRequests::PickModeSelectEntity, m_cachedEntityIdUnderCursor);
            }

            // after a click, always stop pick mode, whether we set an entity or not
            EditorPickModeRequestBus::Broadcast(&EditorPickModeRequests::StopEntityPickMode);
        }

        return false;
    }

    EditorPickEntitySelection::EditorPickEntitySelection(
        const EditorVisibleEntityDataCacheInterface* entityDataCache, ViewportEditorModeTrackerInterface* viewportEditorModeTracker)
        : m_pickEntitySelectionHelper(entityDataCache, viewportEditorModeTracker)
    {
    }

    bool EditorPickEntitySelection::InternalHandleMouseViewportInteraction(
        const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        return m_pickEntitySelectionHelper.HandleMouseViewportInteraction(mouseInteraction);
    }

    void EditorPickEntitySelection::DisplayViewportSelection(
        const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        m_pickEntitySelectionHelper.DisplayEditorHelpers(viewportInfo, debugDisplay);
        m_pickEntitySelectionHelper.HighlightSelectedEntity();
    }
} // namespace AzToolsFramework
