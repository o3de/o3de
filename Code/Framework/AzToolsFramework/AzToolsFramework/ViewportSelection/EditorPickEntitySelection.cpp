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
    AZ_CLASS_ALLOCATOR_IMPL(EditorPickEntitySelection, AZ::SystemAllocator, 0)

    EditorPickEntitySelection::EditorPickEntitySelection(
        const EditorVisibleEntityDataCache* entityDataCache, ViewportEditorModeTrackerInterface* viewportEditorModeTracker)
        : m_editorHelpers(AZStd::make_unique<EditorHelpers>(entityDataCache))
        , m_viewportEditorModeTracker(viewportEditorModeTracker)
    {
        m_viewportEditorModeTracker->ActivateMode({ GetEntityContextId() }, ViewportEditorMode::Pick);
    }

    EditorPickEntitySelection::~EditorPickEntitySelection()
    {
        if (m_hoveredEntityId.IsValid())
        {
            ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetEntityHighlighted, m_hoveredEntityId, false);
        }

        m_viewportEditorModeTracker->DeactivateMode({ GetEntityContextId() }, ViewportEditorMode::Pick);
    }

    // note: entityIdUnderCursor is the authoritative entityId we get each frame by querying
    // HandleMouseInteraction on EditorHelpers, hoveredEntityId is what was under the cursor
    // the previous frame. We need to be able to notify the entity the hover/mouse just left and
    // that it should no longer be highlighted, or that a hover just started, so it should be
    // highlighted - hoveredEntityId is an in/out param that is updated based on the change in
    // entityIdUnderCursor.
    static void HandleAccents(
        const AZ::EntityId entityIdUnderCursor, AZ::EntityId& hoveredEntityId, const ViewportInteraction::MouseButtons mouseButtons)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        const bool invalidMouseButtonHeld = mouseButtons.Middle() || mouseButtons.Right();

        // we were hovering, but our new entity id has changed (or we're using the mouse to perform
        // an action unrelated to selection - e.g. moving the camera). In this case we should remove
        // the highlight as it's no longer valid, and clear the hovered entity id
        if ((hoveredEntityId.IsValid() && hoveredEntityId != entityIdUnderCursor) || invalidMouseButtonHeld)
        {
            if (hoveredEntityId.IsValid())
            {
                ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetEntityHighlighted, hoveredEntityId, false);

                hoveredEntityId.SetInvalid();
            }
        }

        // if we are using the mouse in a selection context and the entity id under the cursor is valid,
        // set it to be highlighted and update the hovered entity id
        if (!invalidMouseButtonHeld)
        {
            if (entityIdUnderCursor.IsValid() && entityIdUnderCursor != hoveredEntityId)
            {
                ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetEntityHighlighted, entityIdUnderCursor, true);

                hoveredEntityId = entityIdUnderCursor;
            }
        }
    }

    bool EditorPickEntitySelection::InternalHandleMouseViewportInteraction(
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

    void EditorPickEntitySelection::DisplayViewportSelection(
        const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        const AzFramework::CameraState cameraState = GetCameraState(viewportInfo.m_viewportId);

        m_editorHelpers->DisplayHelpers(
            viewportInfo, cameraState, debugDisplay,
            [](AZ::EntityId)
            {
                return true;
            });

        HandleAccents(
            m_cachedEntityIdUnderCursor, m_hoveredEntityId, ViewportInteraction::BuildMouseButtons(QGuiApplication::mouseButtons()));
    }
} // namespace AzToolsFramework
