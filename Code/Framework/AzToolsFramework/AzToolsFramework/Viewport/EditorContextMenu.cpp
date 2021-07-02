/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorContextMenu.h"

#include "AzToolsFramework/Viewport/ViewportMessages.h"
#include "Editor/EditorContextMenuBus.h"

namespace AzToolsFramework
{
    void EditorContextMenuUpdate(EditorContextMenu& contextMenu, const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        // could potentially show the context menu
        if (mouseInteraction.m_mouseInteraction.m_mouseButtons.Right() &&
            mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::Down)
        {
            contextMenu.m_shouldOpen = true;
            contextMenu.m_clickPoint =
                ViewportInteraction::QPointFromScreenPoint(mouseInteraction.m_mouseInteraction.m_mousePick.m_screenCoordinates);
        }

        // disable shouldOpen if right clicking an moving the mouse
        if (mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::Move)
        {
            const QPoint currentScreenCoords =
                ViewportInteraction::QPointFromScreenPoint(mouseInteraction.m_mouseInteraction.m_mousePick.m_screenCoordinates);

            contextMenu.m_shouldOpen = contextMenu.m_shouldOpen && (currentScreenCoords - contextMenu.m_clickPoint).manhattanLength() < 2;
        }

        // do show the context menu
        if (mouseInteraction.m_mouseInteraction.m_mouseButtons.Right() &&
            mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::Up)
        {
            if (contextMenu.m_shouldOpen)
            {
                QWidget* parent = nullptr;
                ViewportInteraction::MainEditorViewportInteractionRequestBus::EventResult(
                    parent, mouseInteraction.m_mouseInteraction.m_interactionId.m_viewportId,
                    &ViewportInteraction::MainEditorViewportInteractionRequestBus::Events::GetWidgetForViewportContextMenu);

                contextMenu.m_menu = new QMenu(parent);
                contextMenu.m_menu->setAttribute(Qt::WA_DeleteOnClose);
                contextMenu.m_menu->setParent(parent);

                // Populate global context menu.
                const int contextMenuFlag = 0;
                AzToolsFramework::EditorContextMenuBus::Broadcast(&AzToolsFramework::EditorContextMenuEvents::PopulateEditorGlobalContextMenu, contextMenu.m_menu.data(),
                    AzFramework::Vector2FromScreenPoint(mouseInteraction.m_mouseInteraction.m_mousePick.m_screenCoordinates),
                    contextMenuFlag);

                if (!contextMenu.m_menu->isEmpty())
                {
                    // Use popup instead of exec; this avoids blocking input event processing while the menu dialog is active
                    contextMenu.m_menu->popup(QCursor::pos());
                }
            }
        }
    }
} // namespace AzToolsFramework
